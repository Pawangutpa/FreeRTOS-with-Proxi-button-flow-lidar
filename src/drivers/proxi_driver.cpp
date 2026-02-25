#include "drivers/proxi_driver.h"
#include "rtos/rtos_queues.h"
#include "common/data_types.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/board_config.h"


/* ==========================================================
   CALIBRATION
   ========================================================== */

#define FLOW_PULSES_PER_LITER 274.0f

   /* ==========================================================
      GLOBAL ISR VARIABLES
      ========================================================== */

static volatile uint32_t g_flowPulseCount = 0;
static volatile uint32_t g_nutCountLeft = 0;
static volatile uint32_t g_nutCountRight = 0;

/*
   Multi-core safe spinlock for critical section
*/
static portMUX_TYPE g_proxiMux = portMUX_INITIALIZER_UNLOCKED;

/* ==========================================================
   ISR FUNCTIONS
   ========================================================== */

   /*
    * Flow Sensor ISR
    * Adds debounce protection (50ms)
    */
void IRAM_ATTR FlowSensor_ISR()
{
    static uint32_t lastMicros = 0;
    uint32_t now = micros();

    if ((now - lastMicros) > 50000)   // 50ms debounce
    {
        g_flowPulseCount++;
        lastMicros = now;
    }
}

void IRAM_ATTR NutLeft_ISR()
{
    portENTER_CRITICAL_ISR(&g_proxiMux);
    g_nutCountLeft++;
    portEXIT_CRITICAL_ISR(&g_proxiMux);
}

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

    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN),
        FlowSensor_ISR,
        FALLING);

    attachInterrupt(digitalPinToInterrupt(NUT_SENSOR_LEFT_PIN),
        NutLeft_ISR,
        RISING);

    attachInterrupt(digitalPinToInterrupt(NUT_SENSOR_RIGHT_PIN),
        NutRight_ISR,
        RISING);

    Serial.println("Proxi Driver Initialized");
}

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

    if (deltaTime == 0) return 0;

    float pulsesPerSec = deltaCount / (deltaTime / 1000.0f);
    float speed_mps = (pulsesPerSec / PULSES_PER_REV) * WHEEL_CIRCUMFERENCE;

    return speed_mps * 3.6f;
}

/* ==========================================================
   PROXI TASK
   ========================================================== */

void ProxiTask(void* pvParameters)
{
    proxi_data_t proxi_data;

    uint32_t lastFlowCount = 0;
    float totalLiters = 0.0f;

    while (true)
    {
        uint32_t flowCount;
        uint32_t nutLeft;
        uint32_t nutRight;

        /*
         * Multi-core safe critical section
         */
        portENTER_CRITICAL(&g_proxiMux);

        flowCount = g_flowPulseCount;
        nutLeft = g_nutCountLeft;
        nutRight = g_nutCountRight;

        portEXIT_CRITICAL(&g_proxiMux);

        /*
         * Calculate water consumption
         */
        if (flowCount != lastFlowCount)
        {
            uint32_t delta = flowCount - lastFlowCount;
            totalLiters += (float)delta / FLOW_PULSES_PER_LITER;
            lastFlowCount = flowCount;
        }

        /*
         * Populate structure
         */
        proxi_data.total_water_liters = totalLiters;
        proxi_data.nut_count_left = nutLeft;
        proxi_data.nut_count_right = nutRight;
        proxi_data.nut_count_average =
            (nutLeft + nutRight) / 2.0f;
        proxi_data.wheel_speed_kmph =
            ComputeWheelSpeed(
                (nutLeft + nutRight) / 2);


        /*
         * Send to RTOS queue (non-blocking)
         */
        if (g_proxiQueue != NULL)
        {
            xQueueOverwrite(g_proxiQueue, &proxi_data);
        }

        /*
         * Run every 200 ms
         */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}