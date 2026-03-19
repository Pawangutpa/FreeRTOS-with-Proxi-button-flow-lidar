#include "log/system_logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rtos/rtos_queues.h"

/* ==========================================================
   STATE TO STRING
   ========================================================== */

const char* GetSystemStateString(system_state_t state)
{
    switch (state)
    {
    case SYS_STATE_INIT: return "INIT";
    case SYS_STATE_CAN_INIT: return "CAN_INIT";
    case SYS_STATE_CAN_STARTED: return "CAN_STARTED";
    case SYS_STATE_LIDAR_WAIT: return "LIDAR_WAIT";
    case SYS_STATE_RUNNING: return "RUNNING";
    case SYS_STATE_CAN_START_FAILED: return "CAN_FAIL";
    case SYS_STATE_QUEUE_ERROR: return "QUEUE_ERR";
    case SYS_STATE_FAN_TX_ERROR: return "FAN_TX_ERR";
    default: return "UNKNOWN";
    }
}

const char* GetButtonStateString(BUTTON_state_t state)
{
    switch (state)
    {
    case BUTTON_STATE_INIT: return "INIT";
    case BUTTON_STATE_AUTONOMOUS: return "AUTO";
    case BUTTON_STATE_MANUAL: return "MANUAL";
    case BUTTON_STATE_MANUAL_LEFT: return "LEFT";
    case BUTTON_STATE_MANUAL_RIGHT: return "RIGHT";
    case BUTTON_STATE_MANUAL_BOTH: return "BOTH";
    case BUTTON_STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

const char* GetProxiStateString(PROXI_state_t state)
{
    switch (state)
    {
    case PROXI_STATE_INIT: return "INIT";
    case PROXI_STATE_IDLE: return "IDLE";
    case PROXI_STATE_MOVING: return "MOVING";
    case PROXI_STATE_SPRAYING: return "SPRAYING";
    case PROXI_STATE_FLOW_SENSOR_FAULT: return "FLOW_FAULT";
    case PROXI_STATE_PTO_SENSOR_FAULT: return "PTO_FAULT";
    case PROXI_STATE_NUT_LEFT_FAULT: return "LEFT_FAULT";
    case PROXI_STATE_NUT_RIGHT_FAULT: return "RIGHT_FAULT";
    case PROXI_STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

/* ==========================================================
   LOGGER TASK
   ========================================================== */

extern volatile system_state_t g_systemState;
extern volatile BUTTON_state_t g_buttonState;
extern volatile PROXI_state_t g_proxiState;

void SystemLoggerTask(void* pvParameters)
{
    // system_state_t lastSys = (system_state_t)-1;
    // BUTTON_state_t lastBtn = (BUTTON_state_t)-1;
    // PROXI_state_t lastProxi = (PROXI_state_t)-1;

    lidar_data_t lidar = { 0 };
    proxi_data_t proxi = { 0 };

    while (true)
    {
        /* Get latest sensor data (non-blocking) */
 xQueuePeek(g_lidarQueue, &lidar, 0);
xQueuePeek(g_proxiQueue, &proxi, 0);

        /* Print only when state changes OR always (your choice) */
        // if (lastSys != g_systemState ||
        //     lastBtn != g_buttonState ||
        //     lastProxi != g_proxiState)
        // {
        //     lastSys = g_systemState;
        //     lastBtn = g_buttonState;
        //     lastProxi = g_proxiState;

            Serial.printf(
    "SYS CAN:%s | BTN:%s | PROXI:%s | "
    "L CAN:%d R CAN:%d | ",

    GetSystemStateString(g_systemState),
    GetButtonStateString(g_buttonState),
    GetProxiStateString(g_proxiState),

    lidar.left_distance_cm,
    lidar.right_distance_cm

    
);
        //}

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}