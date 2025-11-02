#include <ESP8266WiFi.h>

// Local includes
#include "config.h"
#include "sensor_wifi.h"
#include "storage.h"

#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32
#define DEFAULT_AP_SSID "AirQualitySetup"
#define DEFAULT_AP_PASS "airquality123"

namespace SensorWifi {
  static char wifi_ssid[WIFI_SSID_MAXLEN + 1] = {0};
  static char wifi_pass[WIFI_PASS_MAXLEN + 1] = {0};
  static bool isAPMode = true;

  // --- AP/STA Mode Functions ---
  void startAPMode() {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
      isAPMode = true;
  }

  void startSTAMode(const char* ssid, const char* pass) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass);
      isAPMode = false;
  }

  // This function should be called after user sets WiFi config via webui
  void applyWiFiConfigFromWebUI(const char* ssid, const char* pass) {
      startSTAMode(ssid, pass);
  }

  // Optionally, you can call this in main if you want to always start AP for config
  void setupWiFi() {
    startAPMode();
  }

  void loadConfig() {
    // Initialize EEPROM
    Storage::begin();

    // Read stored SSID and password
    String ssid = Storage::readString(WIFI_SSID_ADDR, WIFI_SSID_MAXLEN);
    String pass = Storage::readString(WIFI_PASS_ADDR, WIFI_PASS_MAXLEN);

    // Use default values if none are stored
    if (ssid.length() == 0)
      ssid = WIFI_SSID;
    if (pass.length() == 0)
      pass = WIFI_PASSWORD;

    // Update in-memory copies
    strncpy(wifi_ssid, ssid.c_str(), WIFI_SSID_MAXLEN);
    wifi_ssid[WIFI_SSID_MAXLEN] = '\0';
    strncpy(wifi_pass, pass.c_str(), WIFI_PASS_MAXLEN);
    wifi_pass[WIFI_PASS_MAXLEN] = '\0';
  }

  void saveConfig(const char* ssid, const char* pass) {
    Storage::writeString(WIFI_SSID_ADDR, ssid, WIFI_SSID_MAXLEN);
    Storage::writeString(WIFI_PASS_ADDR, pass, WIFI_PASS_MAXLEN);
    Storage::commit();
    strncpy(wifi_ssid, ssid, WIFI_SSID_MAXLEN);
    wifi_ssid[WIFI_SSID_MAXLEN] = '\0';
    strncpy(wifi_pass, pass, WIFI_PASS_MAXLEN);
    wifi_pass[WIFI_PASS_MAXLEN] = '\0';
  }

  const char* getSSID() {
    // If not set, return empty string
    if (wifi_ssid[0] == '\0' || strcmp(wifi_ssid, WIFI_SSID) == 0) return "";
    return wifi_ssid;
  }
  const char* getPASS() { return wifi_pass; }

  void connect(const char* ssid, const char* pass) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    int wifi_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_attempts < MAX_WIFI_ATTEMPTS) {
        delay(WIFI_RETRY_DELAY_MS);
        Serial.print(".");
        ++wifi_attempts;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi connected. IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi connection failed.");
    }
  }
}
