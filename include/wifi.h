#pragma once

#include <ESP8266WiFi.h>

#define WIFI_SSID_MAXLEN 31
#define WIFI_PASS_MAXLEN 31

namespace WifiConnect {
  void connect(const char* ssid, const char* pass);
  void loadConfig();
  void saveConfig(const char* ssid, const char* pass);
  const char* getSSID();
  const char* getPASS();
}
