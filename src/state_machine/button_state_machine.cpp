#include "state_machine/button_state_machine.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

volatile BUTTON_state_t g_buttonState = BUTTON_STATE_INIT;

/* ==========================================================
   SET STATE
   ========================================================== */

void SetButtonState(BUTTON_state_t newState)
{
    g_buttonState = newState;
}

/* ==========================================================
   STATE MACHINE TASK
   ========================================================== */

void StateMachineTaskButton(void *pvParameters)
{
   // BUTTON_state_t lastState = (BUTTON_state_t)-1;

    while (true)
    {
        // if (lastState != g_buttonState)
        // {
        //     lastState = g_buttonState;

            switch (g_buttonState)
            {
                case BUTTON_STATE_INIT:
                    //Serial.println("STATE: INIT");
                    break;

                case BUTTON_STATE_AUTONOMOUS:
                    //Serial.println("STATE: AUTONOMOUS MODE");
                    break;

                case BUTTON_STATE_MANUAL:
                   // Serial.println("STATE: MANUAL MODE");
                    break;

                case BUTTON_STATE_MANUAL_LEFT:
                    //Serial.println("STATE: MANUAL LEFT SPRAY");
                    break;

                case BUTTON_STATE_MANUAL_RIGHT:
                    //Serial.println("STATE: MANUAL RIGHT SPRAY");
                    break;

                case BUTTON_STATE_MANUAL_BOTH:
                    //Serial.println("STATE: MANUAL BOTH SPRAY");
                    break;

                case BUTTON_STATE_ERROR:
                    //Serial.println("STATE: ERROR");
                    break;

                default:
                    //Serial.println("STATE: UNKNOWN");
                    break;
            //}
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}