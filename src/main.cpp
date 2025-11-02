#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Local includes
#include "config.h"
#include "sensor_serial.h"
#include "display.h"
#include "sensor_wifi.h"
#include "webui.h"

// Timing variables
unsigned long lastUpdate = 0;

// Create web server instance
ESP8266WebServer server(80);

void initializeWiFi() {
  SensorWifi::loadConfig();
  const char* ssid = SensorWifi::getSSID();

  if (strlen(ssid) == 0 || strcmp(ssid, WIFI_SSID) == 0) {
    Serial.println("No WiFi config found, starting in AP mode for setup...");
    SensorWifi::startAPMode();
  } else {
    Serial.println("WiFi config found, starting in STA mode...");
    SensorWifi::connect(ssid, SensorWifi::getPASS());
  }
}

void setup() {
  SensorSerial::initializeSerial();
  Display::begin();
  initializeWiFi();

  setupWebUI(server);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
