#pragma once

#include <ESP8266WebServer.h>

// HTTP surface: web UI, JSON API and the Prometheus endpoint.
namespace WebUI {
  void begin(ESP8266WebServer &server);
  void loop();
}
