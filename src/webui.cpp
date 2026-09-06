#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "metrics.h"
#include "net_wifi.h"
#include "sensors.h"
#include "web_page.h"
#include "webui.h"

namespace WebUI {
  namespace {
    ESP8266WebServer *httpServer = nullptr;
    uint32_t rebootAtMs = 0;

    void scheduleReboot(uint32_t delayMs = 500U) {
      rebootAtMs = millis() + delayMs;
    }

    void sendJson(int code, const String &payload) {
      httpServer->send(code, F("application/json"), payload);
    }

    void handleRoot() {
      httpServer->send_P(200, PSTR("text/html; charset=utf-8"), WEB_PAGE);
    }

    void handleMetrics() {
      Metrics::sendPrometheus(*httpServer);
    }

    void handleStatus() {
      sendJson(200, Metrics::toJson());
    }

    void handleGetConfig() {
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
      document["displayType"] = config.displayType;
      document["displayBrightness"] = config.displayBrightness;
      document["displayNightBrightness"] = config.displayNightBrightness;
      document["displayNightStartHour"] = config.displayNightStartHour;
      document["displayNightEndHour"] = config.displayNightEndHour;
      document["showClockPage"] = config.showClockPage;
      document["flipDisplay"] = config.flipDisplay;
      // Secrets are never sent back; empty fields mean "keep the current value"
      document["wifiPassword"] = "";
      document["otaPassword"] = "";

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

      const char *otaPassword = document["otaPassword"];
      if (otaPassword != nullptr && strlen(otaPassword) > 0)
        config.otaPassword = otaPassword;

      if (config.httpPort == 0)
        config.httpPort = DEFAULT_HTTP_PORT;

      if (!Config::save()) {
        sendJson(500, F("{\"error\":\"unable to persist configuration\"}"));
        return;
      }

      sendJson(200, F("{\"status\":\"saved\",\"rebooting\":true}"));
      scheduleReboot(1000U);
    }

    void handleReboot() {
      sendJson(200, F("{\"status\":\"rebooting\"}"));
      scheduleReboot();
    }

    void handleCalibrateCo2() {
      const bool started = SenseAirS8::calibrate();
      sendJson(started ? 200 : 500,
               started ? F("{\"status\":\"calibration started\"}")
                       : F("{\"error\":\"calibration request failed\"}"));
    }

    void handleNotFound() {
      httpServer->send(404, F("text/plain"), F("Not found\n"));
    }
  }

  void begin(ESP8266WebServer &server) {
    httpServer = &server;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/metrics", HTTP_GET, handleMetrics);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/config", HTTP_POST, handlePostConfig);
    server.on("/api/reboot", HTTP_POST, handleReboot);
    server.on("/api/calibrate-co2", HTTP_POST, handleCalibrateCo2);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.printf("[http] listening on port %u\n", Config::get().httpPort);
  }

  void loop() {
    if (httpServer != nullptr)
      httpServer->handleClient();

    if (rebootAtMs != 0 && millis() >= rebootAtMs) {
      Serial.println(F("[http] rebooting on request"));
      ESP.restart();
    }
  }
}
