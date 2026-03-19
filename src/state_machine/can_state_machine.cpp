#include "state_machine/can_state_machine.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

volatile system_state_t g_systemState = SYS_STATE_INIT;

void SetSystemState(system_state_t newState)
{
    g_systemState = newState;
}

void StateMachineTask(void* pvParameters)
{
    //system_state_t lastState = (system_state_t)-1;

    while (true)
    {
        // if (lastState != g_systemState)
        // {
        //     lastState = g_systemState;

            switch (g_systemState)
            {
            case SYS_STATE_INIT:
                //Serial.println("STATE: INIT");
                break;

            case SYS_STATE_CAN_INIT:
                //Serial.println("STATE: CAN INIT");
                break;

            case SYS_STATE_CAN_STARTED:
                //Serial.println("STATE: CAN STARTED");
                break;

            case SYS_STATE_LIDAR_WAIT:
                //Serial.println("STATE: WAIT LIDAR");
                break;

            case SYS_STATE_RUNNING:
                //Serial.println("STATE: RUNNING");
                break;

            case SYS_STATE_CAN_START_FAILED:
                //Serial.println("ERROR: CAN START FAILED");
                break;

            case SYS_STATE_QUEUE_ERROR:
                //Serial.println("ERROR: LIDAR QUEUE FAILED");
                break;

            case SYS_STATE_FAN_TX_ERROR:
                //Serial.println("ERROR: FAN CAN TX FAILED");
                break;
            default:
                //Serial.println("UNKNOWN STATE");
                break;
            }
        //}

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}