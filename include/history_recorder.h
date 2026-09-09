#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>

// Samples the readings into a rolling in-RAM window so the web UI can draw a trend next to each value.
// See History for the ring itself.
namespace HistoryRecorder {
  void begin();
  void loop();

  // Streams the window as JSON: the sample interval, how many slots are filled, and one array per series
  // with nulls where a sensor had nothing to give. Streamed in chunks because the whole payload is a few
  // kilobytes, well past what fits in a String on this board.
  void sendJson(ESP8266WebServer &server);
}
