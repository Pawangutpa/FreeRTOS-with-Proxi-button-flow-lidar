#ifndef BUTTON_STATE_MACHINE_H
#define BUTTON_STATE_MACHINE_H

#include <Arduino.h>

/* ==========================================================
   BUTTON STATES
   ========================================================== */

typedef enum
{
    BUTTON_STATE_INIT = 0,
    BUTTON_STATE_AUTONOMOUS,
    BUTTON_STATE_MANUAL,
    BUTTON_STATE_MANUAL_LEFT,
    BUTTON_STATE_MANUAL_RIGHT,
    BUTTON_STATE_MANUAL_BOTH,

    BUTTON_STATE_ERROR

} BUTTON_state_t;

/* ==========================================================
   GLOBAL STATE VARIABLE
   ========================================================== */

extern volatile BUTTON_state_t g_buttonState;

/* ==========================================================
   FUNCTIONS
   ========================================================== */

void SetButtonState(BUTTON_state_t newState);

void StateMachineTaskButton(void *pvParameters);

#endif