#include <ArduinoJson.h>
#include <LittleFS.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "console.h"

namespace Config {
  namespace {
    AppConfig config;
    bool filesystemMounted = false;

    // Device id defaults to a name derived from the chip id, so two sensors on the same network do not
    // collide.
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
      target.batteryFullVolts = DEFAULT_BATTERY_FULL_VOLTS;
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
      document["wifiNoSleep"] = source.wifiNoSleep;
      document["httpPort"] = source.httpPort;
      document["adminPassword"] = source.adminPassword;
      document["ntpServer"] = source.ntpServer;
      document["timezone"] = source.timezone;
      document["altitudeMeters"] = source.altitudeMeters;
      document["batteryEnabled"] = source.batteryEnabled;
      document["batteryVoltageScale"] = source.batteryVoltageScale;
      document["batteryFullVolts"] = source.batteryFullVolts;
      document["displayType"] = source.displayType;
      document["displayBrightness"] = source.displayBrightness;
      document["displayNightBrightness"] = source.displayNightBrightness;
      document["displayNightStartHour"] = source.displayNightStartHour;
      document["displayNightEndHour"] = source.displayNightEndHour;
      document["showClockPage"] = source.showClockPage;
      document["flipDisplay"] = source.flipDisplay;
      // The SGP30 baseline used to live here. It is refreshed every hour, and rewriting the whole
      // configuration that often put the WiFi credentials at risk on every power cut, so it moved to
      // BASELINE_PATH and is deliberately not written back.
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
      target.wifiNoSleep = document["wifiNoSleep"] | target.wifiNoSleep;
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
      target.batteryFullVolts = document["batteryFullVolts"] | target.batteryFullVolts;
      target.displayType = document["displayType"] | target.displayType;
      target.displayBrightness = document["displayBrightness"] | target.displayBrightness;
      target.displayNightBrightness = document["displayNightBrightness"] | target.displayNightBrightness;
      target.displayNightStartHour = document["displayNightStartHour"] | target.displayNightStartHour;
      target.displayNightEndHour = document["displayNightEndHour"] | target.displayNightEndHour;
      target.showClockPage = document["showClockPage"] | target.showClockPage;
      target.flipDisplay = document["flipDisplay"] | target.flipDisplay;

      // Only read, never written back: BaselineStore picks these up once and moves them to its own file
      target.legacySgpBaselineCo2 = document["sgpBaselineCo2"] | target.legacySgpBaselineCo2;
      target.legacySgpBaselineTvoc = document["sgpBaselineTvoc"] | target.legacySgpBaselineTvoc;
      target.legacySgpBaselineEpoch = document["sgpBaselineEpoch"] | target.legacySgpBaselineEpoch;

      // A port of 0 would make the web server unreachable
      if (target.httpPort == 0)
        target.httpPort = DEFAULT_HTTP_PORT;

      // An empty string is valid JSON, so the fallback above would keep it and leave the clock on UTC with no
      // daylight saving at all
      if (target.timezone.length() == 0)
        target.timezone = DEFAULT_TIMEZONE;

      if (target.ntpServer.length() == 0)
        target.ntpServer = DEFAULT_NTP_SERVER;

      // A scale of 0 would report a flat battery on a healthy cell
      if (target.batteryVoltageScale <= 0.0f)
        target.batteryVoltageScale = DEFAULT_BATTERY_VOLTAGE_SCALE;

      // An anchor out of range, 0 from a configuration written before the setting existed included, would
      // peg the reported charge at one end
      if (target.batteryFullVolts < BATTERY_FULL_VOLTS_MIN || target.batteryFullVolts > BATTERY_FULL_VOLTS_MAX)
        target.batteryFullVolts = DEFAULT_BATTERY_FULL_VOLTS;

      // An hour out of range would make the night window meaningless
      if (target.displayNightStartHour > 23 || target.displayNightEndHour > 23) {
        target.displayNightStartHour = DEFAULT_DISPLAY_NIGHT_START_HOUR;
        target.displayNightEndHour = DEFAULT_DISPLAY_NIGHT_END_HOUR;
      }

      // An unknown panel would leave the device without a usable display
      if (!isSupportedDisplay(target.displayType)) {
        Console.printf("[config] unknown display \"%s\", falling back to %s\n",
                      target.displayType.c_str(), DISPLAY_TYPE_SH1106_128X64);
        target.displayType = DISPLAY_TYPE_SH1106_128X64;
      }
    }
  }

  void begin() {
    applyDefaults(config);

    filesystemMounted = LittleFS.begin();
    if (!filesystemMounted) {
      Console.println(F("[config] LittleFS mount failed, formatting"));
      filesystemMounted = LittleFS.format() && LittleFS.begin();
    }

    if (!filesystemMounted) {
      Console.println(F("[config] LittleFS unavailable, using defaults"));
      return;
    }

    // A temporary file still around means an earlier save was cut short; the target was never touched, so
    // the leftover is just dropped.
    if (LittleFS.exists(CONFIG_TMP_PATH)) {
      Console.println(F("[config] dropping a leftover temporary file from an interrupted save"));
      LittleFS.remove(CONFIG_TMP_PATH);
    }

    File file = LittleFS.open(CONFIG_PATH, "r");
    if (!file) {
      Console.println(F("[config] no config file, using defaults"));
      save();
      return;
    }

    JsonDocument document;
    const DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error) {
      Console.printf("[config] invalid config file (%s), using defaults\n", error.c_str());
      return;
    }

    deserialize(document, config);
    Console.printf("[config] loaded, device id \"%s\"\n", config.deviceId.c_str());
  }

  AppConfig &get() {
    return config;
  }

  bool writeJsonAtomically(const char *path, const char *temporaryPath, const JsonDocument &document) {
    if (!filesystemMounted)
      return false;

    // Opening the target itself in "w" truncates it before a single byte is written, so a power cut in
    // between leaves an empty file and takes the WiFi credentials with it. The rename below is the operation
    // that replaces the old content, and LittleFS makes it atomic.
    File file = LittleFS.open(temporaryPath, "w");
    if (!file) {
      Console.printf("[config] unable to open %s for writing\n", temporaryPath);
      return false;
    }

    const size_t written = serializeJson(document, file);
    const bool failed = written == 0 || file.getWriteError() != 0;
    file.close();

    if (failed) {
      Console.printf("[config] failed to write %s\n", temporaryPath);
      LittleFS.remove(temporaryPath);
      return false;
    }

    // LittleFS replaces an existing destination on its own, but an interrupted earlier attempt can leave the
    // filesystem in a state where it refuses, so the fallback clears the way and tries once more.
    if (LittleFS.rename(temporaryPath, path))
      return true;

    LittleFS.remove(path);
    if (LittleFS.rename(temporaryPath, path))
      return true;

    Console.printf("[config] unable to rename %s over %s\n", temporaryPath, path);
    LittleFS.remove(temporaryPath);

    return false;
  }

  bool save() {
    JsonDocument document;
    serialize(config, document);

    return writeJsonAtomically(CONFIG_PATH, CONFIG_TMP_PATH, document);
  }

  void reset() {
    applyDefaults(config);

    // The baseline is calibration of the sensor that was in this room, so it goes with the rest
    if (filesystemMounted) {
      LittleFS.remove(BASELINE_PATH);
      LittleFS.remove(BASELINE_TMP_PATH);
    }

    save();
  }

  bool hasWifiCredentials() {
    return config.wifiSsid.length() > 0;
  }

  bool usesDefaultAdminPassword() {
    return config.adminPassword == DEFAULT_ADMIN_PASSWORD;
  }

  bool hasNoAdminPassword() {
    return config.adminPassword.length() == 0;
  }

  bool isStorageAvailable() {
    return filesystemMounted;
  }
}
