#include <ArduinoJson.h>
#include <LittleFS.h>

// Local includes
#include "app_config.h"
#include "config.h"

namespace Config {
  namespace {
    AppConfig config;
    bool filesystemMounted = false;

    // Device id defaults to a name derived from the chip id, so two sensors on
    // the same network do not collide.
    String defaultDeviceId() {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "%s-%06x", DEFAULT_HOSTNAME, ESP.getChipId());
      return String(buffer);
    }

    void applyDefaults(AppConfig &target) {
      target = AppConfig();
      target.deviceId = defaultDeviceId();
      target.httpPort = DEFAULT_HTTP_PORT;
      target.adminPassword = DEFAULT_ADMIN_PASSWORD;
      target.ntpServer = DEFAULT_NTP_SERVER;
      target.timezone = DEFAULT_TIMEZONE;
      target.displayType = DISPLAY_TYPE_SH1106_128X64;
      target.batteryVoltageScale = DEFAULT_BATTERY_VOLTAGE_SCALE;
      target.displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
      target.displayNightBrightness = DEFAULT_DISPLAY_NIGHT_BRIGHTNESS;
      target.displayNightStartHour = DEFAULT_DISPLAY_NIGHT_START_HOUR;
      target.displayNightEndHour = DEFAULT_DISPLAY_NIGHT_END_HOUR;
    }

    bool isSupportedDisplay(const String &value) {
      return value == DISPLAY_TYPE_SH1106_128X64 || value == DISPLAY_TYPE_SSD1306_64X48;
    }

    IPAddress parseIp(const char *value) {
      IPAddress address;
      if (value == nullptr || !address.fromString(value))
        return IPAddress();

      return address;
    }

    void serialize(const AppConfig &source, JsonDocument &document) {
      document["deviceId"] = source.deviceId;
      document["wifiSsid"] = source.wifiSsid;
      document["wifiPassword"] = source.wifiPassword;
      document["useStaticIp"] = source.useStaticIp;
      document["ip"] = source.ip.toString();
      document["gateway"] = source.gateway.toString();
      document["subnet"] = source.subnet.toString();
      document["dns"] = source.dns.toString();
      document["httpPort"] = source.httpPort;
      document["adminPassword"] = source.adminPassword;
      document["ntpServer"] = source.ntpServer;
      document["timezone"] = source.timezone;
      document["altitudeMeters"] = source.altitudeMeters;
      document["batteryEnabled"] = source.batteryEnabled;
      document["batteryVoltageScale"] = source.batteryVoltageScale;
      document["displayType"] = source.displayType;
      document["displayBrightness"] = source.displayBrightness;
      document["displayNightBrightness"] = source.displayNightBrightness;
      document["displayNightStartHour"] = source.displayNightStartHour;
      document["displayNightEndHour"] = source.displayNightEndHour;
      document["showClockPage"] = source.showClockPage;
      document["flipDisplay"] = source.flipDisplay;
      document["sgpBaselineCo2"] = source.sgpBaselineCo2;
      document["sgpBaselineTvoc"] = source.sgpBaselineTvoc;
      document["sgpBaselineEpoch"] = source.sgpBaselineEpoch;
    }

    void deserialize(const JsonDocument &document, AppConfig &target) {
      target.deviceId = document["deviceId"] | target.deviceId;
      target.wifiSsid = document["wifiSsid"] | target.wifiSsid;
      target.wifiPassword = document["wifiPassword"] | target.wifiPassword;
      target.useStaticIp = document["useStaticIp"] | target.useStaticIp;
      target.ip = parseIp(document["ip"]);
      target.gateway = parseIp(document["gateway"]);
      target.subnet = parseIp(document["subnet"]);
      target.dns = parseIp(document["dns"]);
      target.httpPort = document["httpPort"] | target.httpPort;
      // The key used to be otaPassword, back when it only guarded the upload
      const char *legacyPassword = document["otaPassword"];
      if (legacyPassword != nullptr)
        target.adminPassword = legacyPassword;

      target.adminPassword = document["adminPassword"] | target.adminPassword;
      target.ntpServer = document["ntpServer"] | target.ntpServer;
      target.timezone = document["timezone"] | target.timezone;
      target.altitudeMeters = document["altitudeMeters"] | target.altitudeMeters;
      target.batteryEnabled = document["batteryEnabled"] | target.batteryEnabled;
      target.batteryVoltageScale = document["batteryVoltageScale"] | target.batteryVoltageScale;
      target.displayType = document["displayType"] | target.displayType;
      target.displayBrightness = document["displayBrightness"] | target.displayBrightness;
      target.displayNightBrightness = document["displayNightBrightness"] | target.displayNightBrightness;
      target.displayNightStartHour = document["displayNightStartHour"] | target.displayNightStartHour;
      target.displayNightEndHour = document["displayNightEndHour"] | target.displayNightEndHour;
      target.showClockPage = document["showClockPage"] | target.showClockPage;
      target.flipDisplay = document["flipDisplay"] | target.flipDisplay;
      target.sgpBaselineCo2 = document["sgpBaselineCo2"] | target.sgpBaselineCo2;
      target.sgpBaselineTvoc = document["sgpBaselineTvoc"] | target.sgpBaselineTvoc;
      target.sgpBaselineEpoch = document["sgpBaselineEpoch"] | target.sgpBaselineEpoch;

      // A port of 0 would make the web server unreachable
      if (target.httpPort == 0)
        target.httpPort = DEFAULT_HTTP_PORT;

      // An empty string is valid JSON, so the fallback above would keep it and
      // leave the clock on UTC with no daylight saving at all
      if (target.timezone.length() == 0)
        target.timezone = DEFAULT_TIMEZONE;

      if (target.ntpServer.length() == 0)
        target.ntpServer = DEFAULT_NTP_SERVER;

      // A scale of 0 would report a flat battery on a healthy cell
      if (target.batteryVoltageScale <= 0.0f)
        target.batteryVoltageScale = DEFAULT_BATTERY_VOLTAGE_SCALE;
      target.displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
      target.displayNightBrightness = DEFAULT_DISPLAY_NIGHT_BRIGHTNESS;
      target.displayNightStartHour = DEFAULT_DISPLAY_NIGHT_START_HOUR;
      target.displayNightEndHour = DEFAULT_DISPLAY_NIGHT_END_HOUR;

      // An hour out of range would make the night window meaningless
      if (target.displayNightStartHour > 23 || target.displayNightEndHour > 23) {
        target.displayNightStartHour = DEFAULT_DISPLAY_NIGHT_START_HOUR;
        target.displayNightEndHour = DEFAULT_DISPLAY_NIGHT_END_HOUR;
      }

      // An unknown panel would leave the device without a usable display
      if (!isSupportedDisplay(target.displayType)) {
        Serial.printf("[config] unknown display \"%s\", falling back to %s\n",
                      target.displayType.c_str(), DISPLAY_TYPE_SH1106_128X64);
        target.displayType = DISPLAY_TYPE_SH1106_128X64;
      }
    }
  }

  void begin() {
    applyDefaults(config);

    filesystemMounted = LittleFS.begin();
    if (!filesystemMounted) {
      Serial.println(F("[config] LittleFS mount failed, formatting"));
      filesystemMounted = LittleFS.format() && LittleFS.begin();
    }

    if (!filesystemMounted) {
      Serial.println(F("[config] LittleFS unavailable, using defaults"));
      return;
    }

    File file = LittleFS.open(CONFIG_PATH, "r");
    if (!file) {
      Serial.println(F("[config] no config file, using defaults"));
      save();
      return;
    }

    JsonDocument document;
    const DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error) {
      Serial.printf("[config] invalid config file (%s), using defaults\n", error.c_str());
      return;
    }

    deserialize(document, config);
    Serial.printf("[config] loaded, device id \"%s\"\n", config.deviceId.c_str());
  }

  AppConfig &get() {
    return config;
  }

  bool save() {
    if (!filesystemMounted)
      return false;

    JsonDocument document;
    serialize(config, document);

    File file = LittleFS.open(CONFIG_PATH, "w");
    if (!file) {
      Serial.println(F("[config] unable to open config file for writing"));
      return false;
    }

    const bool written = serializeJson(document, file) > 0;
    file.close();

    if (!written)
      Serial.println(F("[config] failed to write config file"));

    return written;
  }

  void reset() {
    applyDefaults(config);
    save();
  }

  bool hasWifiCredentials() {
    return config.wifiSsid.length() > 0;
  }
}
