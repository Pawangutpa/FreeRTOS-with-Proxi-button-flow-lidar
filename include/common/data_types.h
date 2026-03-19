#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ==========================================================
   FILE: data_types.h
   DESCRIPTION:
   Common data structures shared between RTOS tasks.

   These structures are used for communication through
   FreeRTOS queues between different modules such as:

   - Lidar Sensor Task
   - Proximity / Wheel Sensor Task
   - GPS Task
   - Spray Control Task
   ========================================================== */


/* ==========================================================
   LIDAR DATA STRUCTURE
   PURPOSE:
   Stores both raw and filtered data from left and right
   CAN-based LiDAR sensors.

   Raw values come directly from CAN frames.
   Filtered values are processed using median + validation
   filtering before being used by spray logic.
   ========================================================== */

typedef struct
{
    /* ---------- RAW SENSOR DATA ---------- */

    uint16_t left_raw_distance_cm;     // Raw distance from left LiDAR (cm)
    uint16_t left_raw_strength;        // Signal strength from left LiDAR

    uint16_t right_raw_distance_cm;    // Raw distance from right LiDAR (cm)
    uint16_t right_raw_strength;       // Signal strength from right LiDAR

    /* ---------- FILTERED DISTANCE ---------- */

    uint16_t left_distance_cm;         // Median-filtered distance (left)
    uint16_t right_distance_cm;        // Median-filtered distance (right)

    /* ---------- VALIDITY FLAGS ---------- */

    bool left_is_valid;                // True if left measurement passes validation
    bool right_is_valid;               // True if right measurement passes validation

} lidar_data_t;


/* ==========================================================
   PROXI DATA STRUCTURE
   PURPOSE:
   Stores data from proximity / wheel / nut detection sensors.

   Used for:
   - Water usage tracking
   - Nut detection counting
   - Vehicle movement estimation
   ========================================================== */

typedef struct
{
    float total_water_liters;          // Total water sprayed (liters)

    uint32_t nut_count_left;           // Nuts detected on left side
    uint32_t nut_count_right;          // Nuts detected on right side

    float nut_count_average;           // Average nut count between sensors

    float wheel_speed_kmph;            // Vehicle speed calculated from wheel sensor
   
   bool pto_shaft_active;             // True if PTO shaft is currently active  
    

} proxi_data_t;


/* ==========================================================
   GPS DATA STRUCTURE
   PURPOSE:
   Stores raw and filtered GPS information.

   Raw coordinates come directly from GPS module.
   Filtered coordinates are used by navigation or logging.
   ========================================================== */

typedef struct
{
    /* ---------- RAW GPS DATA ---------- */

    double raw_latitude;               // Raw latitude from GPS
    double raw_longitude;              // Raw longitude from GPS

    /* ---------- FILTERED POSITION ---------- */

    double latitude;                   // Filtered latitude
    double longitude;                  // Filtered longitude

    /* ---------- MOTION DATA ---------- */

    float  speed_kmph;                 // Vehicle speed from GPS

    /* ---------- GPS STATUS ---------- */

    uint8_t fix_quality;               // GPS fix quality (0 = no fix, >0 = valid)

    uint32_t timestamp_sec;            // GPS timestamp (seconds)

} gps_data_t;

#endif