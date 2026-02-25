#include <Arduino.h>
#include "drivers/button_driver.h"
#include "rtos/rtos_events.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/board_config.h"

/* ==============================
   DRIVER INIT
   ============================== */
void Button_Driver_Init(void)
{
    pinMode(BUTTON_AUTONOMOUS_PIN, INPUT_PULLUP);
    pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BOTH_PIN, INPUT_PULLUP);

    Serial.println("Button Driver Started");
}

/* ==============================
   BUTTON TASK
   ============================== */
void ButtonTask(void *pvParameters)
{
    bool lastAutonomousState = false;

    while (true)
    {
        /* -------------------------
           READ BUTTON STATES
        ------------------------- */

        bool autonomousPressed =
            (digitalRead(BUTTON_AUTONOMOUS_PIN) == LOW);

        bool leftPressed =
            (digitalRead(BUTTON_LEFT_PIN) == LOW);

        bool rightPressed =
            (digitalRead(BUTTON_RIGHT_PIN) == LOW);

        bool bothPressed =
            (digitalRead(BUTTON_BOTH_PIN) == LOW);

        /* -------------------------
           MODE SELECTION
        ------------------------- */

        if (autonomousPressed != lastAutonomousState)
        {
            if (autonomousPressed)
            {
                xEventGroupSetBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_AUTONOMOUS);

                xEventGroupClearBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_MANUAL);

                Serial.println("Mode: AUTONOMOUS");
            }
            else
            {
                xEventGroupSetBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_MANUAL);

                xEventGroupClearBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_AUTONOMOUS);

                Serial.println("Mode: MANUAL");
            }

            lastAutonomousState = autonomousPressed;
        }

        /* -------------------------
           MANUAL SPRAY CONTROL
           (Only active in MANUAL mode)
        ------------------------- */

        EventBits_t bits =
            xEventGroupGetBits(g_systemEventGroup);

        bool manualMode = (bits & SYSTEM_BIT_MANUAL);

        if (manualMode)
        {
            /* Clear all spray bits first */
            xEventGroupClearBits(
                g_systemEventGroup,
                SYSTEM_BIT_LEFT |
                SYSTEM_BIT_RIGHT |
                SYSTEM_BIT_BOTH);

            if (bothPressed)
            {
                xEventGroupSetBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_BOTH);

                Serial.println("Manual: BOTH");
            }
            else
            {
                if (leftPressed)
                {
                    xEventGroupSetBits(
                        g_systemEventGroup,
                        SYSTEM_BIT_LEFT);

                    Serial.println("Manual: LEFT");
                }

                if (rightPressed)
                {
                    xEventGroupSetBits(
                        g_systemEventGroup,
                        SYSTEM_BIT_RIGHT);

                    Serial.println("Manual: RIGHT");
                }
            }
        }
        else
        {
            /* In autonomous mode → clear manual spray bits */
            xEventGroupClearBits(
                g_systemEventGroup,
                SYSTEM_BIT_LEFT |
                SYSTEM_BIT_RIGHT |
                SYSTEM_BIT_BOTH);
        }

        /* 100ms polling */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}