#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

// Project identification
constexpr const char* PROJECT_NAME = "AQ Sensor";
constexpr const char* PROJECT_VERSION = "0.1.0";

// WiFi configuration
const IPAddress STATIC_IP(192, 168, 1, 100);
const IPAddress GATEWAY(192, 168, 1, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress DNS(192, 168, 1, 1);
constexpr const char*WIFI_SSID = "your_ssid";
constexpr const char* WIFI_PASSWORD ="your_password";
constexpr const char* WIFI_HOSTNAME = "AQ Sensor";
constexpr int MAX_WIFI_ATTEMPTS = 30;
constexpr int WIFI_RETRY_DELAY_MS = 500;
