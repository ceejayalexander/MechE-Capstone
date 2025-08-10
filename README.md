## Overview
This repository contains the complete firmware for an ESP32 microcontroller that controls a dual-branch heating and cooling system using thermoelectric (TEC) Peltier modules.

## System Specifications

- 36 V TEC modules: Two independent branches, modules connected in series
- Airflow:
  - 6 × 24 V fans (parallel) for heated/cooled air distribution
  - 6 × 12 V fans (parallel) for TEC module cooling
- Temperature monitoring: 6 temperature sensors (one per air duct channel)
- Control hardware: 14 × 5 V MOSFETs for regulating heating/cooling and fan speeds

## Additional Features

- Server logic for client connections via the ESP32 access point
- JavaScript-based data plotting for real-time temperature sensor monitoring
- Autonomous control mode that adjusts system operation based on user-defined settings
