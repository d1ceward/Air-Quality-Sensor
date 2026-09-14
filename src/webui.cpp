#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "history_recorder.h"
#include "metrics.h"
#include "net_wifi.h"
#include "sensors.h"
#include "web_page.h"
#include "webui.h"

namespace WebUI {
  namespace {
    ESP8266WebServer *httpServer = nullptr;
    uint32_t rebootAtMs = 0;
    // Set when the reboot is meant to wipe the configuration first, so the answer goes out before the
    // filesystem is touched
    bool factoryResetPending = false;

    // A phone offers what it can see; more than this is a list nobody scrolls
    constexpr uint8_t MAX_SCAN_RESULTS = 20;

    void scheduleReboot(uint32_t delayMs = 500U) {
      rebootAtMs = millis() + delayMs;
    }

    // Guards the administrative routes. /metrics and /api/status stay open: a scraper cannot be asked for
    // credentials, and the readings are what the device exists to publish.
    bool authorised() {
      const String &password = Config::get().adminPassword;
      if (password.length() == 0)
        return true;

      if (httpServer->authenticate(ADMIN_USER, password.c_str()))
        return true;

      httpServer->requestAuthentication(BASIC_AUTH, "Air Quality Sensor",
                                        "Authentication required\n");
      return false;
    }

    void sendJson(int code, const String &payload) {
      httpServer->send(code, F("application/json"), payload);
    }

    // Both pages share the same head, so it is streamed separately rather than stored twice in flash.
    void sendPage(PGM_P body) {
      httpServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
      httpServer->send(200, F("text/html; charset=utf-8"), "");
      httpServer->sendContent_P(WEB_HEAD);
      httpServer->sendContent_P(body);
      httpServer->sendContent("");
    }

    void handleRoot() {
      sendPage(WEB_STATUS);
    }

    void handleSettingsPage() {
      if (!authorised())
        return;

      sendPage(WEB_SETTINGS);
    }

    void handleMetrics() {
      Metrics::sendPrometheus(*httpServer);
    }

    void handleStatus() {
      sendJson(200, Metrics::toJson());
    }

    // Networks in range, so the SSID does not have to be typed from memory. The first call starts the scan
    // and answers "scanning"; the page polls until the results are in.
    void handleScan() {
      if (!authorised())
        return;

      NetWifi::ScanResult results[MAX_SCAN_RESULTS];
      uint8_t found = 0;
      const NetWifi::ScanState state = NetWifi::scan(results, MAX_SCAN_RESULTS, found);

      if (state == NetWifi::ScanState::Failed) {
        sendJson(500, F("{\"scanning\":false,\"error\":\"scan failed\",\"networks\":[]}"));
        return;
      }

      if (state == NetWifi::ScanState::Running) {
        sendJson(200, F("{\"scanning\":true,\"networks\":[]}"));
        return;
      }

      JsonDocument document;
      document["scanning"] = false;
      JsonArray networks = document["networks"].to<JsonArray>();

      for (uint8_t index = 0; index < found; ++index) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = results[index].ssid;
        network["rssi"] = results[index].rssi;
        network["secured"] = results[index].secured;
      }

      String payload;
      serializeJson(document, payload);
      sendJson(200, payload);
    }

    void handleHistory() {
      HistoryRecorder::sendJson(*httpServer);
    }

    void handleGetConfig() {
      if (!authorised())
        return;

      const AppConfig &config = Config::get();

      JsonDocument document;
      document["deviceId"] = config.deviceId;
      document["wifiSsid"] = config.wifiSsid;
      document["useStaticIp"] = config.useStaticIp;
      document["ip"] = config.ip.toString();
      document["gateway"] = config.gateway.toString();
      document["subnet"] = config.subnet.toString();
      document["dns"] = config.dns.toString();
      document["httpPort"] = config.httpPort;
      document["ntpServer"] = config.ntpServer;
      document["timezone"] = config.timezone;
      document["altitudeMeters"] = config.altitudeMeters;
      document["batteryEnabled"] = config.batteryEnabled;
      document["batteryVoltageScale"] = config.batteryVoltageScale;
      document["batteryFullVolts"] = config.batteryFullVolts;
      document["displayType"] = config.displayType;
      document["displayBrightness"] = config.displayBrightness;
      document["displayNightBrightness"] = config.displayNightBrightness;
      document["displayNightStartHour"] = config.displayNightStartHour;
      document["displayNightEndHour"] = config.displayNightEndHour;
      document["showClockPage"] = config.showClockPage;
      document["flipDisplay"] = config.flipDisplay;
      // Secrets are never sent back; empty fields mean "keep the current value"
      document["wifiPassword"] = "";
      document["adminPassword"] = "";

      String payload;
      serializeJson(document, payload);
      sendJson(200, payload);
    }

    // Assigns an IP field only when the submitted value parses
    void assignIp(JsonDocument &document, const char *key, IPAddress &target) {
      const char *value = document[key];
      if (value == nullptr)
        return;

      IPAddress parsed;
      if (parsed.fromString(value))
        target = parsed;
    }

    void handlePostConfig() {
      if (!authorised())
        return;

      JsonDocument document;
      const DeserializationError error = deserializeJson(document, httpServer->arg("plain"));

      if (error) {
        sendJson(400, F("{\"error\":\"invalid JSON body\"}"));
        return;
      }

      AppConfig &config = Config::get();

      config.deviceId = document["deviceId"] | config.deviceId;
      config.wifiSsid = document["wifiSsid"] | config.wifiSsid;
      config.useStaticIp = document["useStaticIp"] | config.useStaticIp;
      config.httpPort = document["httpPort"] | config.httpPort;
      config.ntpServer = document["ntpServer"] | config.ntpServer;
      config.timezone = document["timezone"] | config.timezone;
      config.altitudeMeters = document["altitudeMeters"] | config.altitudeMeters;
      config.showClockPage = document["showClockPage"] | config.showClockPage;
      config.batteryEnabled = document["batteryEnabled"] | config.batteryEnabled;
      config.batteryVoltageScale = document["batteryVoltageScale"] | config.batteryVoltageScale;

      if (config.batteryVoltageScale <= 0.0f)
        config.batteryVoltageScale = DEFAULT_BATTERY_VOLTAGE_SCALE;

      config.batteryFullVolts = document["batteryFullVolts"] | config.batteryFullVolts;

      if (config.batteryFullVolts < BATTERY_FULL_VOLTS_MIN || config.batteryFullVolts > BATTERY_FULL_VOLTS_MAX)
        config.batteryFullVolts = DEFAULT_BATTERY_FULL_VOLTS;

      config.displayBrightness = document["displayBrightness"] | config.displayBrightness;
      config.displayNightBrightness = document["displayNightBrightness"] | config.displayNightBrightness;

      // Hours outside the day are silently ignored rather than stored
      const uint8_t nightStart = document["displayNightStartHour"] | config.displayNightStartHour;
      const uint8_t nightEnd = document["displayNightEndHour"] | config.displayNightEndHour;
      if (nightStart <= 23 && nightEnd <= 23) {
        config.displayNightStartHour = nightStart;
        config.displayNightEndHour = nightEnd;
      }

      const char *displayType = document["displayType"];
      if (displayType != nullptr &&
          (strcmp(displayType, DISPLAY_TYPE_SH1106_128X64) == 0 ||
           strcmp(displayType, DISPLAY_TYPE_SSD1306_64X48) == 0))
        config.displayType = displayType;
      config.flipDisplay = document["flipDisplay"] | config.flipDisplay;

      assignIp(document, "ip", config.ip);
      assignIp(document, "gateway", config.gateway);
      assignIp(document, "subnet", config.subnet);
      assignIp(document, "dns", config.dns);

      // Empty password fields keep the stored secrets
      const char *wifiPassword = document["wifiPassword"];
      if (wifiPassword != nullptr && strlen(wifiPassword) > 0)
        config.wifiPassword = wifiPassword;

      const char *adminPassword = document["adminPassword"];
      if (adminPassword != nullptr && strlen(adminPassword) > 0)
        config.adminPassword = adminPassword;

      if (config.httpPort == 0)
        config.httpPort = DEFAULT_HTTP_PORT;

      if (config.timezone.length() == 0)
        config.timezone = DEFAULT_TIMEZONE;

      if (config.ntpServer.length() == 0)
        config.ntpServer = DEFAULT_NTP_SERVER;

      if (!Config::save()) {
        sendJson(500, F("{\"error\":\"unable to persist configuration\"}"));
        return;
      }

      sendJson(200, F("{\"status\":\"saved\",\"rebooting\":true}"));
      scheduleReboot(1000U);
    }

    void handleReboot() {
      if (!authorised())
        return;

      sendJson(200, F("{\"status\":\"rebooting\"}"));
      scheduleReboot();
    }

    // Wipes the configuration, WiFi credentials and admin password included, and the stored SGP30 baseline
    // with it. The device comes back up in setup mode.
    void handleFactoryReset() {
      if (!authorised())
        return;

      sendJson(200, F("{\"status\":\"resetting\"}"));
      factoryResetPending = true;
      scheduleReboot(1000U);
    }

    void handleCalibrateCo2() {
      if (!authorised())
        return;

      const bool started = SenseAirS8::calibrate();
      sendJson(started ? 200 : 500,
               started ? F("{\"status\":\"calibration started\"}")
                       : F("{\"error\":\"calibration request failed\"}"));
    }

    void handleNotFound() {
      // On the setup access point every unknown address is redirected to the readings page, which is what a
      // phone's captive portal check follows: joining the network opens the page on its own instead of
      // leaving someone to find 192.168.4.1 by hand. It points at the root and not at /settings because
      // that one asks for the admin password, and a browser prompting for credentials is not what a
      // captive portal notification should open on.
      if (NetWifi::isAccessPointActive()) {
        httpServer->sendHeader(F("Location"), String("http://") + NetWifi::ipAddress() + "/");
        httpServer->send(302, F("text/plain"), F("Redirecting to the setup page\n"));
        return;
      }

      httpServer->send(404, F("text/plain"), F("Not found\n"));
    }
  }

  void begin(ESP8266WebServer &server) {
    httpServer = &server;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/settings", HTTP_GET, handleSettingsPage);
    server.on("/metrics", HTTP_GET, handleMetrics);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/history", HTTP_GET, handleHistory);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/config", HTTP_POST, handlePostConfig);
    server.on("/api/scan", HTTP_GET, handleScan);
    server.on("/api/reboot", HTTP_POST, handleReboot);
    server.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
    server.on("/api/calibrate-co2", HTTP_POST, handleCalibrateCo2);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.printf("[http] listening on port %u\n", Config::get().httpPort);

    if (Config::hasNoAdminPassword())
      Serial.println(F("[http] no admin password set, the settings and the firmware upload are open"));
    else if (Config::usesDefaultAdminPassword())
      Serial.println(F("[http] admin password is still the one built into the firmware, change it"));
  }

  void loop() {
    if (httpServer != nullptr)
      httpServer->handleClient();

    if (rebootAtMs != 0 && millis() >= rebootAtMs) {
      if (factoryResetPending) {
        Serial.println(F("[http] restoring the factory configuration"));
        Config::reset();
      }

      Serial.println(F("[http] rebooting on request"));
      ESP.restart();
    }
  }
}
