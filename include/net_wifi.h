#pragma once

#include <Arduino.h>

// WiFi lifecycle: connects to the configured network and falls back to a setup access point so the device
// stays reachable even with wrong credentials.
namespace NetWifi {
  // One network found by a scan
  struct ScanResult {
    String ssid;
    int8_t rssi = 0;
    bool secured = false;
  };

  enum class ScanState : uint8_t {
    Running,  // a scan is in flight, call again in a moment
    Ready,    // results were written out
    Failed
  };

  void begin();
  void loop();

  bool isConnected();
  bool isAccessPointActive();

  String hostname();
  String accessPointSsid();
  String ipAddress();
  int8_t rssi();

  // Fills results with the networks in range, strongest first and one entry per name, and sets found to how
  // many were written. Asynchronous: the first call starts the scan and returns Running, and a later call
  // returns Ready with the results. Typing the SSID by hand was the only way to configure the device before.
  ScanState scan(ScanResult *results, uint8_t capacity, uint8_t &found);
}
