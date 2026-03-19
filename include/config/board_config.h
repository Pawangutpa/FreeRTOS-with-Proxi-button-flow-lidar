#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
============================================================
BOARD CONFIGURATION FILE
============================================================

This file defines all hardware pin mappings for the board.

Purpose:
- Central location for all GPIO assignments
- Easy modification if PCB changes
- Prevent hardcoded pins in driver or task code

Usage:
Drivers and application modules should include this file
to access board-level hardware configuration.

Example:
#include "config/board_config.h"

============================================================
*/


/* =========================================================
   CAN BUS CONFIGURATION
   ---------------------------------------------------------
   Pins used by ESP32 TWAI (CAN controller)

   CAN_TX_PIN  -> ESP32 transmit pin
   CAN_RX_PIN  -> ESP32 receive pin

   These connect to the external CAN transceiver.
   ========================================================= */
#define CAN_TX_PIN 36
#define CAN_RX_PIN 37


/* =========================================================
   SOLENOID CONTROL PINS
   ---------------------------------------------------------
   Digital outputs used to control spray solenoid valves.

   SOLENOID_LEFT_PIN  -> Left nozzle valve
   SOLENOID_RIGHT_PIN -> Right nozzle valve
   ========================================================= */
#define SOLENOID_LEFT_PIN   4
#define SOLENOID_RIGHT_PIN  5


/* =========================================================
   USER BUTTON INPUT PINS
   ---------------------------------------------------------
   Buttons used for manual control and system mode selection.

   BUTTON_AUTONOMOUS_PIN -> Enable/disable autonomous mode
   BUTTON_LEFT_PIN       -> Manual left spray trigger
   BUTTON_RIGHT_PIN      -> Manual right spray trigger
   BUTTON_BOTH_PIN       -> Trigger both nozzles
   ========================================================= */
#define BUTTON_AUTONOMOUS_PIN 40
#define BUTTON_LEFT_PIN       1
#define BUTTON_RIGHT_PIN      41
#define BUTTON_BOTH_PIN       42


/* =========================================================
   PROXIMITY / FLOW SENSOR INPUTS
   ---------------------------------------------------------
   Sensors used for water flow and nut counting.

   FLOW_SENSOR_PIN       -> Flow sensor pulse input
   NUT_SENSOR_LEFT_PIN   -> Left proximity sensor
   NUT_SENSOR_RIGHT_PIN  -> Right proximity sensor
   ========================================================= */
#define FLOW_SENSOR_PIN       16
#define NUT_SENSOR_LEFT_PIN   6
#define NUT_SENSOR_RIGHT_PIN  7
#define PTO_SHAFT_SENSOR_PIN  3   


/* =========================================================
   STATUS LED PINS
   ---------------------------------------------------------
   RGB LED used for system status indication.

   LED_R_PIN -> Red channel
   LED_G_PIN -> Green channel
   LED_B_PIN -> Blue channel

   Can be used to indicate:
   - system status
   - errors
   - connectivity
   ========================================================= */
#define LED_R_PIN 8
#define LED_G_PIN 9
#define LED_B_PIN 10


#endif