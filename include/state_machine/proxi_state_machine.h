#ifndef PROXI_STATE_MACHINE_H
#define PROXI_STATE_MACHINE_H

#include <stdint.h>

/* ==========================================================
   PROXI STATES
   ========================================================== */

typedef enum
{
    PROXI_STATE_INIT = 0,

    PROXI_STATE_IDLE,
    PROXI_STATE_MOVING,
    PROXI_STATE_SPRAYING,

    PROXI_STATE_FLOW_SENSOR_FAULT,
    PROXI_STATE_PTO_SENSOR_FAULT,
    PROXI_STATE_NUT_LEFT_FAULT,
    PROXI_STATE_NUT_RIGHT_FAULT,

    PROXI_STATE_ERROR

} PROXI_state_t;

/* ==========================================================
   GLOBAL STATE
   ========================================================== */

extern volatile PROXI_state_t g_proxiState;

/* ==========================================================
   FUNCTIONS
   ========================================================== */

void SetProxiState(PROXI_state_t newState);

void StateMachineTaskProxi(void *pvParameters);

#endif