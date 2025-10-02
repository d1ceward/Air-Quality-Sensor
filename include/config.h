#pragma once

/**
 * Global compile-time configuration constants.
 */

#include <Arduino.h>

// I2C Pins (override default if needed)
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN SDA
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN SCL
#endif

// Display update interval (ms)
constexpr unsigned long DISPLAY_UPDATE_INTERVAL_MS = 1000UL;

// Warmup duration for sensors (ms)
constexpr unsigned long SENSOR_WARMUP_MS = 30UL * 1000UL; // 30 seconds

// Project identification
#define PROJECT_NAME "Air Quality Sensor"
#define PROJECT_VERSION "0.1.0"
