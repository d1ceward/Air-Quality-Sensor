#pragma once

#include <ESP8266WebServer.h>

// Two update paths: ArduinoOTA (pio run -e d1_mini_ota -t upload) and a browser upload form served at
// /update.
namespace NetOta {
  void begin(ESP8266WebServer &server);
  void loop();

  // True while a firmware image is being received.
  bool isUpdating();
}
