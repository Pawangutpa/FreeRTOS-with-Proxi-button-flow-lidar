#include <Arduino.h>
#include "drivers/can_driver.h"
#include "config/board_config.h"
#include "rtos/rtos_queues.h"
#include "common/data_types.h"
#include "driver/twai.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/spray_config.h"

/* ==========================================================
   FILTER CONFIGURATION
   ========================================================== */

#define MEDIAN_WINDOW 3
#define VALID_M 5
#define VALID_N 3

/* ---------- Median Buffers ---------- */
static uint16_t medBufLeft[MEDIAN_WINDOW]  = {0};
static uint16_t medBufRight[MEDIAN_WINDOW] = {0};

static int medIdxLeft  = 0;
static int medIdxRight = 0;

/* ---------- Valid Window Buffers ---------- */
static bool validWinLeft[VALID_M]  = {false};
static bool validWinRight[VALID_M] = {false};

static int winIdxLeft  = 0;
static int winIdxRight = 0;

/* ==========================================================
   MEDIAN OF 3 FUNCTION
   ========================================================== */

static uint16_t Median3(uint16_t a, uint16_t b, uint16_t c)
{
    if (a > b) { uint16_t t = a; a = b; b = t; }
    if (b > c) { uint16_t t = b; b = c; c = t; }
    if (a > b) { uint16_t t = a; a = b; b = t; }
    return b;
}

/* ==========================================================
   CAN DRIVER INITIALIZATION
   ========================================================== */

void CAN_Driver_Init(void)
{
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
        Serial.println("CAN Driver Installed");
    }
    else
    {
        Serial.println("CAN Driver Install Failed");
        return;
    }

    if (twai_start() == ESP_OK)
        Serial.println("CAN Driver Started");
    else
        Serial.println("CAN Start Failed");
}

/* ==========================================================
   LIDAR TASK
   ========================================================== */

void LidarTask(void* pvParameters)
{
    twai_message_t rx_msg;
    lidar_data_t lidar_data;

    memset(&lidar_data, 0, sizeof(lidar_data));

    Serial.println("LidarTask Started");

    while (true)
    {
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) == ESP_OK)
        {
            if (rx_msg.data_length_code >= 4)
            {
                uint16_t distance =
                    rx_msg.data[0] | (rx_msg.data[1] << 8);

                uint16_t strength =
                    rx_msg.data[2] | (rx_msg.data[3] << 8);

                /* =====================================================
                   LEFT SENSOR  (CAN ID 0x101)
                   ===================================================== */
                if (rx_msg.identifier == 0x101)
                {
                    lidar_data.left_raw_distance_cm = distance;
                    lidar_data.left_raw_strength    = strength;

                    /* ---- Median Update ---- */
                    medBufLeft[medIdxLeft] = distance;
                    medIdxLeft = (medIdxLeft + 1) % MEDIAN_WINDOW;

                    uint16_t median =
                        Median3(
                            medBufLeft[0],
                            medBufLeft[1],
                            medBufLeft[2]);

                    /* ---- Frame Validation ---- */
                    bool frame_valid =
                        (strength > SPRAY_MIN_STRENGTH) &&
                        (distance >= SPRAY_MIN_DISTANCE_CM) &&
                        (distance <= SPRAY_MAX_DISTANCE_CM);

                    /* ---- N-of-M Window Update ---- */
                    validWinLeft[winIdxLeft] = frame_valid;
                    winIdxLeft = (winIdxLeft + 1) % VALID_M;

                    int count = 0;
                    for (int i = 0; i < VALID_M; i++)
                        if (validWinLeft[i]) count++;

                    if (count >= VALID_N)
                    {
                        lidar_data.left_distance_cm = median;
                        lidar_data.left_is_valid = true;
                    }
                    else
                    {
                        lidar_data.left_is_valid = false;
                    }
                }

                /* =====================================================
                   RIGHT SENSOR (CAN ID 0x102)
                   ===================================================== */
                else if (rx_msg.identifier == 0x102)
                {
                    lidar_data.right_raw_distance_cm = distance;
                    lidar_data.right_raw_strength    = strength;

                    /* ---- Median Update ---- */
                    medBufRight[medIdxRight] = distance;
                    medIdxRight = (medIdxRight + 1) % MEDIAN_WINDOW;

                    uint16_t median =
                        Median3(
                            medBufRight[0],
                            medBufRight[1],
                            medBufRight[2]);

                    /* ---- Frame Validation ---- */
                    bool frame_valid =
                        (strength > SPRAY_MIN_STRENGTH) &&
                        (distance >= SPRAY_MIN_DISTANCE_CM) &&
                        (distance <= SPRAY_MAX_DISTANCE_CM);

                    /* ---- N-of-M Window Update ---- */
                    validWinRight[winIdxRight] = frame_valid;
                    winIdxRight = (winIdxRight + 1) % VALID_M;

                    int count = 0;
                    for (int i = 0; i < VALID_M; i++)
                        if (validWinRight[i]) count++;

                    if (count >= VALID_N)
                    {
                        lidar_data.right_distance_cm = median;
                        lidar_data.right_is_valid = true;
                    }
                    else
                    {
                        lidar_data.right_is_valid = false;
                    }
                }

                /* ---- Send Latest Data To Queue ---- */
                if (g_lidarQueue != NULL)
                {
                    xQueueOverwrite(g_lidarQueue, &lidar_data);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ==========================================================
   FAN COMMAND (SAME AS NON-RTOS VERSION)
   ========================================================== */

void CAN_SendFanCommand(bool on)
{
    twai_message_t msg = {0};

    msg.identifier = 0x200;          // SAME CAN ID
    msg.data_length_code = 1;
    msg.data[0] = on ? 1 : 0;        // 1 = ON, 0 = OFF

    esp_err_t result =
        twai_transmit(&msg, pdMS_TO_TICKS(10));

    if (result != ESP_OK)
    {
        Serial.println("Fan CAN TX Failed");
    }
}