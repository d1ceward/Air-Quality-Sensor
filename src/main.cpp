#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Local includes
#include "config.h"
#include "display.h"
#include "wifi.h"
#include "webui.h"

// Timing variables
unsigned long lastUpdate = 0;

// Create web server instance
ESP8266WebServer server(80);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);

  // Small delay to allow serial monitor to connect
  delay(1000);

  // Startup messages
  Serial.println();
  Serial.println(String(PROJECT_NAME) + " starting...");

  // Initialize display
  Display::begin();

  // Load WiFi configuration and connect
  WifiConnect::loadConfig();
  WifiConnect::connect(WifiConnect::getSSID(), WifiConnect::getPASS());

  // Web server routes
  setupWebUI(server);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
