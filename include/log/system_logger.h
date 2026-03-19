#ifndef SYSTEM_LOGGER_H
#define SYSTEM_LOGGER_H

#include <Arduino.h>
#include "state_machine/can_state_machine.h"
#include "state_machine/button_state_machine.h"
#include "state_machine/proxi_state_machine.h"
extern QueueHandle_t g_lidarQueue;
extern QueueHandle_t g_proxiQueue;

/* ==========================================================
   FUNCTIONS
   ========================================================== */

const char* GetSystemStateString(system_state_t state);
const char* GetButtonStateString(BUTTON_state_t state);
const char* GetProxiStateString(PROXI_state_t state);

/* Logging Task */
void SystemLoggerTask(void *pvParameters);

#endif