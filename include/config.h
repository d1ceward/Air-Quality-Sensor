
#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

// Project identification
constexpr const char *PROJECT_NAME = "AQ Sensor";
constexpr const char *PROJECT_VERSION = "0.1.0";

// Serial configuration
constexpr uint32_t SERIAL_BAUD_RATE = 115200U;
constexpr uint16_t SERIAL_CONNECT_DELAY_MS = 1000U;

// Storage configuration
constexpr uint16_t EEPROM_SIZE = 128;

// WiFi configuration
// const IPAddress STATIC_IP(192, 168, 1, 100);
// const IPAddress GATEWAY(192, 168, 1, 1);
// const IPAddress SUBNET(255, 255, 255, 0);
// const IPAddress DNS(192, 168, 1, 1);

// WiFi credentials (should only be defined here)
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
constexpr char WIFI_HOSTNAME[] = "AQ Sensor";

// WiFi connection parameters
constexpr uint8_t MAX_WIFI_ATTEMPTS = 30U;
constexpr uint16_t WIFI_RETRY_DELAY_MS = 500U;
