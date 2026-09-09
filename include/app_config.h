#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
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
  String adminPassword;   // settings, reboot, calibration and firmware upload

  String ntpServer;
  String timezone;      // POSIX TZ string

  // Station altitude in meters, used to derive sea level pressure
  float altitudeMeters = 0.0f;

  // Only one of the two boards carries a battery shield, so this stays off
  bool batteryEnabled = false;
  float batteryVoltageScale = 0.0f;

  String displayType;   // one of the DISPLAY_TYPE_* identifiers

  // Panel contrast, and the window where it drops. Equal hours disable it.
  uint8_t displayBrightness = 0;
  uint8_t displayNightBrightness = 0;
  uint8_t displayNightStartHour = 0;
  uint8_t displayNightEndHour = 0;
  bool showClockPage = true;
  bool flipDisplay = false;  // true = rotate 180 deg (display mounted top left)

  // SGP30 calibration as it used to be stored, only read to migrate an existing config file into
  // /sgp30.json. See BaselineStore.
  uint16_t legacySgpBaselineCo2 = 0;
  uint16_t legacySgpBaselineTvoc = 0;
  uint32_t legacySgpBaselineEpoch = 0;
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

  // True while the admin password is still the one compiled into the firmware, which is public. The web UI
  // turns it into a banner rather than refusing to work, because a sensor on a home network with the factory
  // password is still better than one nobody can reach.
  bool usesDefaultAdminPassword();

  // True when no admin password is set at all, which leaves the settings, the reboot and the firmware
  // upload open to anyone who can reach the device.
  bool hasNoAdminPassword();

  // True once LittleFS is mounted, so the callers that persist their own files can skip the attempt.
  bool isStorageAvailable();

  // Writes a JSON document to path through a temporary file renamed over it, so a power cut during the write
  // cannot leave a truncated file behind. Shared with BaselineStore.
  bool writeJsonAtomically(const char *path, const char *temporaryPath, const JsonDocument &document);
}
