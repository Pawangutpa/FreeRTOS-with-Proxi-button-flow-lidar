#include <Arduino.h>
#include "rtos/rtos_tasks.h"
#include "rtos/rtos_queues.h"
#include "rtos/rtos_events.h"
#include "drivers/can_driver.h"
#include "drivers/proxi_driver.h"
#include "drivers/button_driver.h"
#include "common/data_types.h"
#include "services/spray_service.h"
#include "state_machine/can_state_machine.h"
#include "state_machine/Button_state_machine.h"
#include "state_machine/proxi_state_machine.h"
#include "log/system_logger.h"
/* ==========================================================
   GLOBAL RTOS OBJECTS
   ========================================================== */

   /*
   Queues are used for inter-task communication.

   Queue size = 1 because we always want the
   LATEST sensor data (overwrite behaviour).
   */

QueueHandle_t g_lidarQueue = NULL;
QueueHandle_t g_proxiQueue = NULL;


/*
Event group used for system mode control.

Example:
MANUAL MODE
AUTONOMOUS MODE
FAULT MODE
*/
EventGroupHandle_t g_systemEventGroup = NULL;


/* ==========================================================
   DEBUG TASKS
   ========================================================== */

   /*
   Lidar Debug Task

   Purpose:
   Monitor LIDAR values without disturbing
   the main control logic.

   QueuePeek is used instead of QueueReceive
   so the data remains available for other tasks.
   */

static void LidarDebugTask(void* pvParameters)
{
    lidar_data_t rx;

    while (true)
    {
        if (xQueuePeek(g_lidarQueue, &rx, portMAX_DELAY))
        {
            /*
            Uncomment if debugging LIDAR
            */

            // Serial.print("[LIDAR] L:");
            // Serial.print(rx.left_distance_cm);
            // Serial.print("  R:");
            // Serial.print(rx.right_distance_cm );
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/*
Proxi Debug Task

Purpose:
Monitor proximity sensor data
such as nut detection and water usage.
*/

static void ProxiDebugTask(void* pvParameters)
{
    proxi_data_t rx;

    while (true)
    {
        if (xQueuePeek(g_proxiQueue, &rx, portMAX_DELAY))
        {

            /*
            Enable these logs only during debugging
            */

            // Serial.print("  [PROXI] Water:");
            // Serial.print(rx.total_water_liters);
            // Serial.print(" | NutAvg:");
            // Serial.print(rx.nut_count_average);

            // Serial.print("  Left Nut:");
            // Serial.print(rx.nut_count_left);

            // Serial.print("  Right Nut:");
            // Serial.print(rx.nut_count_right);
            //  Serial.print(" wheel speed");
            // Serial.print(rx.wheel_speed_kmph);
            // Serial.println();
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


/*
Mode Monitor Task

Purpose:
Monitor system mode via EventGroup

Useful for debugging state transitions
between MANUAL and AUTONOMOUS modes.
*/

static void ModeMonitorTask(void* pvParameters)
{
    while (true)
    {
        EventBits_t bits = xEventGroupGetBits(g_systemEventGroup);

        /*
        Enable logs only when debugging system mode
        */

        //  if (bits & SYSTEM_BIT_AUTONOMOUS)
        //      Serial.print("    [MODE] AUTONOMOUS");

        //  else if (bits & SYSTEM_BIT_MANUAL)
        //  Serial.print("  [MODE] MANUAL");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/* ==========================================================
   CREATE RTOS TASKS AND OBJECTS
   ========================================================== */

void RTOS_CreateTasks(void)
{
    Serial.println("Initializing RTOS System...");


    /* ======================================================
       CREATE QUEUES
       ====================================================== */

    g_lidarQueue = xQueueCreate(1, sizeof(lidar_data_t));



    g_proxiQueue = xQueueCreate(1, sizeof(proxi_data_t));



    /* ======================================================
       CREATE EVENT GROUP
       ====================================================== */

    g_systemEventGroup = xEventGroupCreate();

    if (g_systemEventGroup == NULL)
    {
        Serial.println("ERROR: Event Group Creation Failed");
    }


    /* ======================================================
       INITIALIZE HARDWARE DRIVERS
       ====================================================== */

    Serial.println("Initializing Drivers...");

    CAN_Driver_Init();
    Proxi_Driver_Init();
    Button_Driver_Init();

    Serial.println("Drivers Initialized");


    /* ======================================================
       CREATE MAIN SYSTEM TASKS
       ====================================================== */

    Serial.println("Creating RTOS Tasks...");

    /*
    Task priorities:

    SprayTask     -> Highest priority (controls actuators)
    LidarTask     -> Sensor input
    ProxiTask     -> Sensor input
    ButtonTask    -> User interaction
    */

    xTaskCreatePinnedToCore(
        LidarTask,
        "LidarTask",
        4096,
        NULL,
        5,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        ProxiTask,
        "ProxiTask",
        4096,
        NULL,
        5,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        ButtonTask,
        "ButtonTask",
        4096,
        NULL,
        4,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        SprayTask,
        "SprayTask",
        4096,
        NULL,
        6,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        StateMachineTask,
        "StateMachinecan",
        4096,
        NULL,
        2,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        StateMachineTaskButton,
        "StateMachineButton",
        4096,
        NULL,
        1,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        StateMachineTaskProxi,
        "StateMachineProxi",
        4096,
        NULL,
        1,
        NULL,
        1);


    /* ======================================================
       CREATE DEBUG TASKS
       ====================================================== */

       /*
       Debug tasks run at low priority
       so they never interfere with
       real-time control.
       */

    xTaskCreatePinnedToCore(
        LidarDebugTask,
        "LidarDebug",
        4096,
        NULL,
        2,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        ProxiDebugTask,
        "ProxiDebug",
        4096,
        NULL,
        2,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        ModeMonitorTask,
        "ModeMonitor",
        4096,
        NULL,
        1,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        SystemLoggerTask,
        "SystemLogger",
        6144,
        NULL,
        1,
        NULL,
        1
    );


    /* ======================================================
       DEFAULT SYSTEM MODE
       ====================================================== */

       /*
       System starts in MANUAL mode by default
       until switched by user or automation logic.
       */

    xEventGroupSetBits(g_systemEventGroup, SYSTEM_BIT_MANUAL);

    Serial.println("RTOS Tasks Created Successfully");
}