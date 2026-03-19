#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <Arduino.h>

/*
============================================================
BUTTON DRIVER INTERFACE
============================================================

This module handles initialization and monitoring of
user input buttons connected to the system.

Responsibilities:
- Configure button GPIO pins
- Detect button presses
- Debounce button inputs
- Run a FreeRTOS task to monitor button states

Typical Use:
1. Initialize driver in system startup
2. Create ButtonTask in RTOS scheduler

Example:
    Button_Driver_Init();
    xTaskCreate(ButtonTask, "ButtonTask", 2048, NULL, 2, NULL);

Related Files:
- board_config.h : Defines button GPIO pins
- rtos_queues.h  : Used if button events are sent to queues
============================================================
*/


/* =========================================================
   Button Driver Initialization
   ---------------------------------------------------------
   Configures GPIO pins for button inputs.

   Must be called once during system initialization
   before ButtonTask is started.
   ========================================================= */
void Button_Driver_Init(void);


/* =========================================================
   Button Monitoring Task
   ---------------------------------------------------------
   FreeRTOS task responsible for:

   - Reading button states
   - Debouncing button presses
   - Generating button events
   - Sending events to other modules (if required)

   Parameter:
   pvParameters -> FreeRTOS task parameter (unused)

   This task typically runs periodically with a
   small delay (e.g., 10-20 ms).
   ========================================================= */
void ButtonTask(void *pvParameters);


#endif