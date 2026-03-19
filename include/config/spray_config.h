#ifndef SPRAY_CONFIG_H
#define SPRAY_CONFIG_H

/*
============================================================
SPRAY SYSTEM CONFIGURATION
============================================================

This file contains configuration parameters used by the
spraying system logic.

It defines:
- Valid LiDAR detection ranges
- Minimum signal strength
- Spray offset compensation
- CAN IDs used by LiDAR sensors and fan controller

Changing these values allows tuning of the spray behavior
without modifying application code.

Used by:
- LidarTask
- Spray control logic
- CAN driver
============================================================
*/


/* =========================================================
   LIDAR DETECTION LIMITS
   ---------------------------------------------------------
   Defines valid distance range for plant detection.

   If measured distance is outside this range,
   the reading will be rejected by the filter.
   ========================================================= */

#define SPRAY_MIN_DISTANCE_CM   20   /* Minimum valid detection distance */
#define SPRAY_MAX_DISTANCE_CM   400  /* Maximum valid detection distance */


/* =========================================================
   SIGNAL STRENGTH THRESHOLD
   ---------------------------------------------------------
   Minimum LiDAR signal strength required to consider
   the measurement valid.

   Helps remove noise or weak reflections.
   ========================================================= */

#define SPRAY_MIN_STRENGTH      200


/* =========================================================
   SPRAY OFFSET
   ---------------------------------------------------------
   Compensation value used to align spray trigger timing
   with the physical nozzle position.

   Example:
   If LiDAR detects plant earlier than nozzle position,
   offset helps synchronize spray timing.
   ========================================================= */

#define SPRAY_OFFSET_COUNT      6
/* =========================================================
   CAN MESSAGE IDENTIFIERS
   ---------------------------------------------------------
   CAN IDs used by system components.

   CAN_ID_LIDAR_LEFT  -> Left LiDAR sensor
   CAN_ID_LIDAR_RIGHT -> Right LiDAR sensor
   CAN_ID_FAN_CMD     -> Command to control fan
   ========================================================= */

#define CAN_ID_LIDAR_LEFT   0x101
#define CAN_ID_LIDAR_RIGHT  0x102
#define CAN_ID_FAN_CMD      0x200


#endif