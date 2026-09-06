#pragma once

#include <Arduino.h>
#include <IPAddress.h>

// Runtime configuration, persisted as JSON on LittleFS (/config.json).
struct AppConfig {
  String deviceId;      // Prometheus "id" label and mDNS/DHCP hostname
  String wifiSsid;
  String wifiPassword;

  bool useStaticIp = false;
  IPAddress ip;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dns;

  uint16_t httpPort = 0;
  String otaPassword;

  String ntpServer;
  String timezone;      // POSIX TZ string

  // Station altitude in meters, used to derive sea level pressure
  float altitudeMeters = 0.0f;

  // Only one of the two boards carries a battery shield, so this stays off
  bool batteryEnabled = false;
  float batteryVoltageScale = 0.0f;

  String displayType;   // one of the DISPLAY_TYPE_* identifiers
  bool showClockPage = true;
  bool flipDisplay = false;  // true = rotate 180 deg (display mounted top left)

  // SGP30 calibration, refreshed by the sensor module
  uint16_t sgpBaselineCo2 = 0;
  uint16_t sgpBaselineTvoc = 0;
  uint32_t sgpBaselineEpoch = 0;
};

namespace Config {
  // Mounts LittleFS and loads /config.json, falling back to defaults.
  void begin();

  AppConfig &get();

  // Serialises the current configuration back to LittleFS.
  bool save();

  // Restores compiled-in defaults and persists them.
  void reset();

  // True when a WiFi network has been configured.
  bool hasWifiCredentials();
}
