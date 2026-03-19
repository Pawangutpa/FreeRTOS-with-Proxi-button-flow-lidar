#include <string.h>
#include <Arduino.h>
#include "drivers/can_driver.h"
#include "config/board_config.h"
#include "rtos/rtos_queues.h"
#include "common/data_types.h"
#include "driver/twai.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/spray_config.h"
#include "state_machine/can_state_machine.h"

/* ==========================================================
   FILTER CONFIGURATION
   ========================================================== */
#define MEDIAN_WINDOW 3
#define VALID_M 5
#define VALID_N 3

/* ==========================================================
   MEDIAN FILTER BUFFERS
   ========================================================== */
static uint16_t medBufLeft[MEDIAN_WINDOW]  = {0};
static uint16_t medBufRight[MEDIAN_WINDOW] = {0};
static uint8_t medIdxLeft  = 0;
static uint8_t medIdxRight = 0;

/* ==========================================================
   VALID WINDOW BUFFERS
   ========================================================== */
static bool validWinLeft[VALID_M]  = {false};
static bool validWinRight[VALID_M] = {false};
static uint8_t winIdxLeft  = 0;
static uint8_t winIdxRight = 0;

/* ==========================================================
   MEDIAN OF 3 FILTER
   ========================================================== */
static inline uint16_t Median3(uint16_t a, uint16_t b, uint16_t c)
{
    if (a > b) { uint16_t t = a; a = b; b = t; }
    if (b > c) { uint16_t t = b; b = c; c = t; }
    if (a > b) { uint16_t t = a; a = b; b = t; }
    return b;
}

/* ==========================================================
   SENSOR PROCESSING FUNCTION
   ========================================================== */
static void ProcessLidarSensor(
    uint16_t distance,
    uint16_t strength,
    uint16_t* raw_distance,
    uint16_t* raw_strength,
    uint16_t* filtered_distance,
    bool* is_valid,
    uint16_t* medBuf,
    uint8_t* medIdx,
    bool* validWin,
    uint8_t* winIdx)
{
    /* ---------- Store raw values ---------- */
    *raw_distance = distance;
    *raw_strength = strength;

    /* ---------- Median Filter ---------- */
    medBuf[*medIdx] = distance;
    *medIdx = (*medIdx + 1) % MEDIAN_WINDOW;

    uint16_t median =
        Median3(
            medBuf[0],
            medBuf[1],
            medBuf[2]);

    /* ---------- Frame Validation ---------- */
    bool frame_valid =
        (strength >= SPRAY_MIN_STRENGTH) &&
        (distance >= SPRAY_MIN_DISTANCE_CM) &&
        (distance <= SPRAY_MAX_DISTANCE_CM);

    /* ---------- N-of-M Filtering ---------- */
    validWin[*winIdx] = frame_valid;
    *winIdx = (*winIdx + 1) % VALID_M;

    uint8_t count = 0;
    for (uint8_t i = 0; i < VALID_M; i++)
    {
        if (validWin[i])
            count++;
    }

    if (count >= VALID_N)
    {
        *filtered_distance = median;
        *is_valid = true;
    }
    else
    {
        *is_valid = false;
    }
}

/* ==========================================================
   CAN DRIVER INITIALIZATION
   ========================================================== */
void CAN_Driver_Init(void)
{
    SetSystemState(SYS_STATE_CAN_INIT);

    twai_general_config_t general_config =
        TWAI_GENERAL_CONFIG_DEFAULT(
            (gpio_num_t)CAN_TX_PIN,
            (gpio_num_t)CAN_RX_PIN,
            TWAI_MODE_NORMAL);

    twai_timing_config_t timing_config =
        TWAI_TIMING_CONFIG_500KBITS();

    twai_filter_config_t filter_config =
        TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(
            &general_config,
            &timing_config,
            &filter_config) == ESP_OK)
    {
        SetSystemState(SYS_STATE_LIDAR_WAIT);
    }
    else
    {
        system_state_t lastState = SYS_STATE_INIT;
        return;
    }

    if (twai_start() == ESP_OK)
        SetSystemState(SYS_STATE_CAN_STARTED);
    else
        SetSystemState(SYS_STATE_CAN_START_FAILED);
}

/* ==========================================================
   LIDAR TASK
   ========================================================== */
void LidarTask(void* pvParameters)
{
    twai_message_t rx_msg;
    lidar_data_t lidar_data;

    memset(&lidar_data, 0, sizeof(lidar_data));

    SetSystemState(SYS_STATE_RUNNING);

    while (true)
    {
        /* Wait for CAN message */
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) == ESP_OK)
        {
            if (rx_msg.data_length_code >= 4)
            {
                uint16_t distance =
                    rx_msg.data[0] |
                    (rx_msg.data[1] << 8);

                uint16_t strength =
                    rx_msg.data[2] |
                    (rx_msg.data[3] << 8);

                /* =====================================================
                   LEFT SENSOR
                   ===================================================== */
                if (rx_msg.identifier == CAN_ID_LIDAR_LEFT)
                {
                    ProcessLidarSensor(
                        distance,
                        strength,
                        &lidar_data.left_raw_distance_cm,
                        &lidar_data.left_raw_strength,
                        &lidar_data.left_distance_cm,
                        &lidar_data.left_is_valid,
                        medBufLeft,
                        &medIdxLeft,
                        validWinLeft,
                        &winIdxLeft);
                }

                /* =====================================================
                   RIGHT SENSOR
                   ===================================================== */
                else if (rx_msg.identifier == CAN_ID_LIDAR_RIGHT)
                {
                    ProcessLidarSensor(
                        distance,
                        strength,
                        &lidar_data.right_raw_distance_cm,
                        &lidar_data.right_raw_strength,
                        &lidar_data.right_distance_cm,
                        &lidar_data.right_is_valid,
                        medBufRight,
                        &medIdxRight,
                        validWinRight,
                        &winIdxRight);
                }
                /* =====================================================
                   SEND DATA TO RTOS QUEUE
                   ===================================================== */
                if (g_lidarQueue != NULL)
                {
                    BaseType_t ok =
                        xQueueOverwrite(
                            g_lidarQueue,
                            &lidar_data);
                    if (ok != pdPASS)
                    {
                        SetSystemState(SYS_STATE_QUEUE_ERROR);
                    }
                }
            }
        }
        /* Small RTOS yield */
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

/* ==========================================================
   FAN CONTROL COMMAND
   ========================================================== */
void CAN_SendFanCommand(bool on)
{
    twai_message_t msg = {0};

    msg.identifier = CAN_ID_FAN_CMD;
    msg.data_length_code = 1;
    msg.data[0] = on ? 1 : 0;

    esp_err_t result =
        twai_transmit(&msg, pdMS_TO_TICKS(10));

    if (result != ESP_OK)
    {
        SetSystemState(SYS_STATE_FAN_TX_ERROR);
    }
}