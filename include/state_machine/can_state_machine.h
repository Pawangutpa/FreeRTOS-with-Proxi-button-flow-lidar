#ifndef CAN_STATE_MACHINE_H
#define CAN_STATE_MACHINE_H

#include <Arduino.h>

typedef enum
{
    SYS_STATE_INIT = 0,
    SYS_STATE_CAN_INIT,
    SYS_STATE_CAN_STARTED,
    SYS_STATE_LIDAR_WAIT,
    SYS_STATE_RUNNING,

    /* ERROR STATES */
    SYS_STATE_CAN_START_FAILED,
    SYS_STATE_QUEUE_ERROR,
    SYS_STATE_FAN_TX_ERROR,

} system_state_t;

extern volatile system_state_t g_systemState;

void StateMachineTask(void *pvParameters);
void SetSystemState(system_state_t newState);

#endif