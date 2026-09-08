#pragma once

#include <Arduino.h>

// WiFi lifecycle: connects to the configured network and falls back to a setup access point so the device
// stays reachable even with wrong credentials.
namespace NetWifi {
  void begin();
  void loop();

  bool isConnected();
  bool isAccessPointActive();

  String hostname();
  String accessPointSsid();
  String ipAddress();
  int8_t rssi();
}
