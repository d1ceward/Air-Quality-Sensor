#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>

// Prometheus text exposition and the JSON payload consumed by the web UI.
namespace Metrics {
  // Streams the /metrics response in chunks to keep memory usage low.
  void sendPrometheus(ESP8266WebServer &server);

  String toJson();
}
