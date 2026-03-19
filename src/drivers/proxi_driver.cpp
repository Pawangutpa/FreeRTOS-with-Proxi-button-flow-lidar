#include "drivers/proxi_driver.h"
#include "rtos/rtos_queues.h"
#include "common/data_types.h"
#include "state_machine/proxi_state_machine.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/board_config.h"

/* ==========================================================
   CALIBRATION
   ========================================================== */

#define FLOW_PULSES_PER_LITER 274.0f
#define PTO_TIMEOUT_MS       500     // PTO considered OFF if no pulse in 500ms

/* ==========================================================
   GLOBAL ISR VARIABLES
   ========================================================== */

static volatile uint32_t g_flowPulseCount = 0;
static volatile uint32_t g_nutCountLeft = 0;
static volatile uint32_t g_nutCountRight = 0;
static volatile uint32_t g_lastPTOPulseTime = 0;

/*
   Multi-core safe spinlock
*/
static portMUX_TYPE g_proxiMux = portMUX_INITIALIZER_UNLOCKED;

/* ==========================================================
   ISR FUNCTIONS
   ========================================================== */

/*
   Flow sensor ISR
*/
void IRAM_ATTR FlowSensor_ISR()
{
    static uint32_t lastMicros = 0;
    uint32_t now = micros();

    /* 5 ms debounce */
    if ((now - lastMicros) > 5000)
    {
        portENTER_CRITICAL_ISR(&g_proxiMux);
        g_flowPulseCount++;
        portEXIT_CRITICAL_ISR(&g_proxiMux);

        lastMicros = now;
    }
}

/*
   PTO shaft ISR
*/
void IRAM_ATTR PTOShaft_ISR()
{
    uint32_t now = millis();

    portENTER_CRITICAL_ISR(&g_proxiMux);
    g_lastPTOPulseTime = now;
    portEXIT_CRITICAL_ISR(&g_proxiMux);
}

/*
   Nut sensor left ISR
*/
void IRAM_ATTR NutLeft_ISR()
{
    portENTER_CRITICAL_ISR(&g_proxiMux);
    g_nutCountLeft++;
    portEXIT_CRITICAL_ISR(&g_proxiMux);
}

/*
   Nut sensor right ISR
*/
void IRAM_ATTR NutRight_ISR()
{
    portENTER_CRITICAL_ISR(&g_proxiMux);
    g_nutCountRight++;
    portEXIT_CRITICAL_ISR(&g_proxiMux);
}

/* ==========================================================
   DRIVER INITIALIZATION
   ========================================================== */

void Proxi_Driver_Init(void)
{
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(NUT_SENSOR_LEFT_PIN, INPUT_PULLUP);
    pinMode(NUT_SENSOR_RIGHT_PIN, INPUT_PULLUP);
    pinMode(PTO_SHAFT_SENSOR_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(FLOW_SENSOR_PIN),
        FlowSensor_ISR,
        FALLING);

    attachInterrupt(
        digitalPinToInterrupt(NUT_SENSOR_LEFT_PIN),
        NutLeft_ISR,
        RISING);

    attachInterrupt(
        digitalPinToInterrupt(NUT_SENSOR_RIGHT_PIN),
        NutRight_ISR,
        RISING);

    attachInterrupt(
        digitalPinToInterrupt(PTO_SHAFT_SENSOR_PIN),
        PTOShaft_ISR,
        RISING);
}

/* ==========================================================
   WHEEL SPEED COMPUTATION
   ========================================================== */

static float ComputeWheelSpeed(uint32_t currentCount)
{
    const float WHEEL_DIAMETER = 0.69f;
    const float WHEEL_CIRCUMFERENCE = 3.1415926f * WHEEL_DIAMETER;
    const float PULSES_PER_REV = 5.0f;

    static uint32_t lastCount = 0;
    static uint32_t lastTime = 0;

    uint32_t now = millis();

    uint32_t deltaCount = currentCount - lastCount;
    uint32_t deltaTime = now - lastTime;

    lastCount = currentCount;
    lastTime = now;

    if (deltaTime == 0)
        return 0;

    float pulsesPerSec =
        deltaCount / (deltaTime / 1000.0f);

    float speed_mps =
        (pulsesPerSec / PULSES_PER_REV) *
        WHEEL_CIRCUMFERENCE;

    return speed_mps * 3.6f;
}

/* ==========================================================
   PROXI TASK
   ========================================================== */

void ProxiTask(void* pvParameters)
{
    proxi_data_t proxi_data = {0};

    uint32_t lastFlowCount = 0;
    float totalLiters = 0.0f;

    while (true)
    {
        uint32_t flowCount;
        uint32_t nutLeft;
        uint32_t nutRight;
        uint32_t lastPTOPulse;

        /* ==============================
           Copy ISR values safely
        ============================== */

        portENTER_CRITICAL(&g_proxiMux);

        flowCount = g_flowPulseCount;
        nutLeft = g_nutCountLeft;
        nutRight = g_nutCountRight;
        lastPTOPulse = g_lastPTOPulseTime;

        portEXIT_CRITICAL(&g_proxiMux);

        /* ==============================
           FLOW CALCULATION
        ============================== */

        uint32_t deltaFlow = flowCount - lastFlowCount;

        if (deltaFlow > 0)
        {
            totalLiters +=
                (float)deltaFlow / FLOW_PULSES_PER_LITER;

            lastFlowCount = flowCount;
        }

        /* ==============================
           PTO STATUS DETECTION
        ============================== */

        uint32_t now = millis();

        if ((now - lastPTOPulse) < PTO_TIMEOUT_MS)
        {
            proxi_data.pto_shaft_active = true;
        }
        else
        {
            proxi_data.pto_shaft_active = false;
        }

        /* ==============================
           POPULATE DATA STRUCTURE
        ============================== */

        proxi_data.total_water_liters = totalLiters;

        proxi_data.nut_count_left = nutLeft;
        proxi_data.nut_count_right = nutRight;

        proxi_data.nut_count_average =
            (nutLeft + nutRight) / 2.0f;

        proxi_data.wheel_speed_kmph =
            ComputeWheelSpeed(
                (nutLeft + nutRight) / 2);

        /* ==================================================
           STATE MACHINE UPDATE
        ================================================== */

        PROXI_state_t newState = PROXI_STATE_IDLE;

        if (!proxi_data.pto_shaft_active)
        {
            newState = PROXI_STATE_IDLE;
        }
        else
        {
            if (proxi_data.wheel_speed_kmph > 0.5f)
            {
                if (deltaFlow > 0)
                {
                    newState = PROXI_STATE_SPRAYING;
                }
                else
                {
                    newState = PROXI_STATE_MOVING;
                }
            }
        }

        /* Fault detection */

        if (proxi_data.pto_shaft_active && deltaFlow == 0)
        {
            newState = PROXI_STATE_FLOW_SENSOR_FAULT;
        }

        if (nutLeft == 0)
        {
            newState = PROXI_STATE_NUT_LEFT_FAULT;
        }

        if (nutRight == 0)
        {
            newState = PROXI_STATE_NUT_RIGHT_FAULT;
        }

        /* Update global state */
        SetProxiState(newState);

        /* ==============================
           SEND TO RTOS QUEUE
        ============================== */

        if (g_proxiQueue != NULL)
        {
            xQueueOverwrite(g_proxiQueue, &proxi_data);
        }

        /* Run every 200 ms */

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}