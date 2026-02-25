#include <Arduino.h>
#include "services/spray_service.h"
#include "rtos/rtos_queues.h"
#include "rtos/rtos_events.h"
#include "config/board_config.h"
#include "config/spray_config.h"
#include "common/data_types.h"
#include "drivers/can_driver.h"
/* ==========================================================
   SEGMENT QUEUE STRUCTURE
   ========================================================== */

#define QUEUE_CAPACITY 20
#define END_OPEN 0xFFFFFFFF

typedef struct
{
    uint32_t start[QUEUE_CAPACITY];
    uint32_t end[QUEUE_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint8_t size;
} segment_queue_t;

static segment_queue_t qLeft = { 0 };
static segment_queue_t qRight = { 0 };

/* ==========================================================
   INTERNAL STATE
   ========================================================== */

static bool sol_left_state = false;
static bool sol_right_state = false;

static bool inCanopyLeft = false;
static bool inCanopyRight = false;

/* ==========================================================
   QUEUE FUNCTIONS
   ========================================================== */

static void QueuePush(segment_queue_t* q, uint32_t s, uint32_t e)
{
    if (q->size >= QUEUE_CAPACITY)
    {
        q->head = (q->head + 1) % QUEUE_CAPACITY;
        q->size--;
    }

    q->start[q->tail] = s;
    q->end[q->tail] = e;

    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->size++;
}

static void QueuePop(segment_queue_t* q)
{
    if (q->size == 0) return;

    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->size--;
}

static uint32_t QueueFrontStart(segment_queue_t* q)
{
    return q->start[q->head];
}

static uint32_t QueueFrontEnd(segment_queue_t* q)
{
    return q->end[q->head];
}

static void QueueSetLastEnd(segment_queue_t* q, uint32_t e)
{
    if (q->size == 0) return;

    int idx = (q->tail + QUEUE_CAPACITY - 1) % QUEUE_CAPACITY;
    q->end[idx] = e;
}

/* ==========================================================
   SOLENOID CONTROL
   ========================================================== */

static void SetSolenoidLeft(bool state)
{
    digitalWrite(SOLENOID_LEFT_PIN, state ? HIGH : LOW);
    sol_left_state = state;
}

static void SetSolenoidRight(bool state)
{
    digitalWrite(SOLENOID_RIGHT_PIN, state ? HIGH : LOW);
    sol_right_state = state;
}

/* ==========================================================
   SPRAY TASK
   ========================================================== */

void SprayTask(void* pvParameters)
{
    lidar_data_t lidar;
    proxi_data_t proxi;

    pinMode(SOLENOID_LEFT_PIN, OUTPUT);
    pinMode(SOLENOID_RIGHT_PIN, OUTPUT);

    SetSolenoidLeft(false);
    SetSolenoidRight(false);

    while (true)
    {
        /* ==============================
           GET MODE
        ============================== */
        EventBits_t bits =
            xEventGroupGetBits(g_systemEventGroup);

        bool autonomous =
            (bits & SYSTEM_BIT_AUTONOMOUS);
        /* ==========================================================
           FAN CONTROL (SAME LOGIC AS NON-RTOS)
           ========================================================== */

        static bool lastFanState = false;

        bool fanState = autonomous ? true : false;

        if (fanState != lastFanState)
        {
            CAN_SendFanCommand(fanState);
            lastFanState = fanState;
        }
        /* ==============================
           READ LATEST DATA
        ============================== */

        if (g_lidarQueue)
            xQueuePeek(g_lidarQueue, &lidar, 0);

        if (g_proxiQueue)
            xQueuePeek(g_proxiQueue, &proxi, 0);

        uint32_t currentCount =
            (uint32_t)proxi.nut_count_average;

        /* ==============================
           AUTONOMOUS MODE
        ============================== */

        if (autonomous)
        {
            /* -------- LEFT -------- */

            if (lidar.left_is_valid)
            {
                if (!inCanopyLeft)
                {
                    inCanopyLeft = true;

                    uint32_t start =
                        currentCount + SPRAY_OFFSET_COUNT;

                    QueuePush(&qLeft,
                        start,
                        END_OPEN);
                }
            }
            else
            {
                if (inCanopyLeft)
                {
                    inCanopyLeft = false;

                    uint32_t end =
                        currentCount + SPRAY_OFFSET_COUNT;

                    QueueSetLastEnd(&qLeft, end);
                }
            }

            /* -------- RIGHT -------- */

            if (lidar.right_is_valid)
            {
                if (!inCanopyRight)
                {
                    inCanopyRight = true;

                    uint32_t start =
                        currentCount + SPRAY_OFFSET_COUNT;

                    QueuePush(&qRight,
                        start,
                        END_OPEN);
                }
            }
            else
            {
                if (inCanopyRight)
                {
                    inCanopyRight = false;

                    uint32_t end =
                        currentCount + SPRAY_OFFSET_COUNT;

                    QueueSetLastEnd(&qRight, end);
                }
            }

            /* ==============================
               PROCESS QUEUES
            ============================== */

            /* LEFT */
            if (qLeft.size > 0)
            {
                uint32_t s = QueueFrontStart(&qLeft);
                uint32_t e = QueueFrontEnd(&qLeft);

                if (!sol_left_state &&
                    currentCount >= s)
                {
                    SetSolenoidLeft(true);
                }

                if (sol_left_state &&
                    e != END_OPEN &&
                    currentCount >= e)
                {
                    SetSolenoidLeft(false);
                    QueuePop(&qLeft);
                }
            }

            /* RIGHT */
            if (qRight.size > 0)
            {
                uint32_t s = QueueFrontStart(&qRight);
                uint32_t e = QueueFrontEnd(&qRight);

                if (!sol_right_state &&
                    currentCount >= s)
                {
                    SetSolenoidRight(true);
                }

                if (sol_right_state &&
                    e != END_OPEN &&
                    currentCount >= e)
                {
                    SetSolenoidRight(false);
                    QueuePop(&qRight);
                }
            }
        }

        /* ==============================
           MANUAL MODE
        ============================== */
        else
        {
            if (bits & SYSTEM_BIT_BOTH)
            {
                SetSolenoidLeft(true);
                SetSolenoidRight(true);
            }
            else
            {
                SetSolenoidLeft(bits & SYSTEM_BIT_LEFT);
                SetSolenoidRight(bits & SYSTEM_BIT_RIGHT);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}