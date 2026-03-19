#include <Arduino.h>
#include "drivers/button_driver.h"
#include "rtos/rtos_events.h"
#include "state_machine/button_state_machine.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/board_config.h"

/* ==========================================================
   DRIVER INIT
   ========================================================== */

void Button_Driver_Init(void)
{
    pinMode(BUTTON_AUTONOMOUS_PIN, INPUT_PULLUP);
    pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BOTH_PIN, INPUT_PULLUP);
}

/* ==========================================================
   BUTTON TASK
   ========================================================== */

void ButtonTask(void *pvParameters)
{
    bool lastAutonomousState = false;

    while (true)
    {
        bool autonomousPressed =
            (digitalRead(BUTTON_AUTONOMOUS_PIN) == LOW);

        bool leftPressed =
            (digitalRead(BUTTON_LEFT_PIN) == LOW);

        bool rightPressed =
            (digitalRead(BUTTON_RIGHT_PIN) == LOW);

        bool bothPressed =
            (digitalRead(BUTTON_BOTH_PIN) == LOW);

        /* ==========================================================
           MODE CONTROL
           ========================================================== */

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

                SetButtonState(BUTTON_STATE_AUTONOMOUS);
            }
            else
            {
                xEventGroupSetBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_MANUAL);

                xEventGroupClearBits(
                    g_systemEventGroup,
                    SYSTEM_BIT_AUTONOMOUS);

                SetButtonState(BUTTON_STATE_MANUAL);
            }

            lastAutonomousState = autonomousPressed;
        }

        /* ==========================================================
           MANUAL CONTROL
           ========================================================== */

        EventBits_t bits =
            xEventGroupGetBits(g_systemEventGroup);

        bool manualMode =
            (bits & SYSTEM_BIT_MANUAL);

        if (manualMode)
        {
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

                SetButtonState(BUTTON_STATE_MANUAL_BOTH);
            }
            else
            {
                if (leftPressed)
                {
                    xEventGroupSetBits(
                        g_systemEventGroup,
                        SYSTEM_BIT_LEFT);

                    SetButtonState(BUTTON_STATE_MANUAL_LEFT);
                }

                if (rightPressed)
                {
                    xEventGroupSetBits(
                        g_systemEventGroup,
                        SYSTEM_BIT_RIGHT);

                    SetButtonState(BUTTON_STATE_MANUAL_RIGHT);
                }
            }
        }
        else
        {
            xEventGroupClearBits(
                g_systemEventGroup,
                SYSTEM_BIT_LEFT |
                SYSTEM_BIT_RIGHT |
                SYSTEM_BIT_BOTH);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}