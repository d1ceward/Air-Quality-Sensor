#include <ESP8266WebServer.h>

// Local includes
#include "config.h"
#include "wifi.h"
#include "webui.h"

extern ESP8266WebServer server;

String getSystemInfoHtml() {
  String html;
  html += "<h2>ESP8266 System Info</h2>";
  html += "<ul>";
  html += "<li>Chip ID: " + String(ESP.getChipId()) + "</li>";
  html += "<li>Flash Chip ID: " + String(ESP.getFlashChipId()) + "</li>";
  html += "<li>CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz</li>";
  html += "<li>Free Heap: " + String(ESP.getFreeHeap()) + " bytes</li>";
  html += "<li>Uptime: " + String(millis() / 1000) + " s</li>";
  html += "<li>SDK Version: " + String(ESP.getSdkVersion()) + "</li>";
  html += "<li>Core Version: " + String(ESP.getCoreVersion()) + "</li>";
  html += "</ul>";
  return html;
}

String getWiFiInfoHtml() {
  String html;
  html += "<h2>WiFi Info</h2>";
  html += "<ul>";
  html += "<li>SSID: " + String(WiFi.SSID()) + "</li>";
  html += "<li>IP: " + WiFi.localIP().toString() + "</li>";
  html += "<li>MAC: " + WiFi.macAddress() + "</li>";
  html += "<li>RSSI: " + String(WiFi.RSSI()) + " dBm</li>";
  html += "</ul>";
  return html;
}

void handleRoot() {
  String html = "<html><head><title>Air Quality Sensor</title></head><body>";
  html += "<h1>Air Quality Sensor Web UI</h1>";
  html += getSystemInfoHtml();
  html += getWiFiInfoHtml();
  html += "<a href='/config'>WiFi Configuration</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfigForm() {
  String html = "<html><head><title>WiFi Config</title></head><body>";
  html += "<h1>WiFi Configuration</h1>";
  html += "<form method='POST' action='/config'>";
  html += "SSID: <input name='ssid' value='" + String(WifiConnect::getSSID()) + "'><br>";
  html += "Password: <input name='pass' type='password' value='" + String(WifiConnect::getPASS()) + "'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";
  html += "<a href='/'>Back</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfigPost() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    WifiConnect::saveConfig(ssid.c_str(), pass.c_str());
    server.send(200, "text/html", "<html><body><h1>Saved! Reboot to apply changes.</h1><a href='/'>Back</a></body></html>");
    return;
  }
  server.send(400, "text/html", "<html><body><h1>Missing SSID or Password</h1><a href='/config'>Back</a></body></html>");
}

void setupWebUI(ESP8266WebServer& serverRef) {
  serverRef.on("/", HTTP_GET, handleRoot);
  serverRef.on("/config", HTTP_GET, handleConfigForm);
  serverRef.on("/config", HTTP_POST, handleConfigPost);
}
