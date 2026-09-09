#include <Arduino.h>

// Local includes
#include "battery.h"
#include "config.h"
#include "history.h"
#include "history_recorder.h"
#include "sensors.h"

namespace HistoryRecorder {
  namespace {
    History::Ring ring;
    uint32_t lastSampleMs = 0;
    bool started = false;

    // Same order as History::Series, and the names the web UI looks the arrays up by
    const char *const SERIES_NAMES[History::SERIES_COUNT] = {
      "pm25", "co2", "tvoc", "temperature", "humidity", "pressure", "battery",
    };

    // Scaling factor each series was stored with, so the UI gets values in the unit it displays
    const float SERIES_FACTORS[History::SERIES_COUNT] = {
      1.0f, 1.0f, 1.0f, 10.0f, 10.0f, 10.0f, 1.0f,
    };

    void take() {
      const Sensors::Snapshot readings = Sensors::snapshot();

      int16_t values[History::SERIES_COUNT];
      values[History::SERIES_PM25] = History::clamped(readings.hasParticulate, readings.particulate.pm25);
      values[History::SERIES_CO2] = History::clamped(readings.hasCo2, readings.co2Ppm);
      values[History::SERIES_TVOC] = History::clamped(readings.hasTvoc, readings.tvocPpb);
      values[History::SERIES_TEMPERATURE] =
        History::scaled(readings.hasClimate, readings.temperatureC, 10.0f);
      values[History::SERIES_HUMIDITY] = History::scaled(readings.hasClimate, readings.humidity, 10.0f);
      values[History::SERIES_PRESSURE] = History::scaled(readings.hasPressure, readings.pressureHpa, 10.0f);
      values[History::SERIES_BATTERY] = History::clamped(Battery::hasReading(), Battery::percent());

      ring.push(values);
      lastSampleMs = millis();
    }

    // Prints a stored slot back in the unit the series is displayed in, or a JSON null for a gap
    void appendSample(String &buffer, uint8_t series, int16_t sample) {
      if (sample == History::NO_SAMPLE) {
        buffer += F("null");
        return;
      }

      if (SERIES_FACTORS[series] == 1.0f) {
        buffer += sample;
        return;
      }

      buffer += String(static_cast<float>(sample) / SERIES_FACTORS[series], 1);
    }
  }

  void begin() {
    ring.clear();

    // The first slot is taken one interval in, once the sensors have had time to produce something
    lastSampleMs = millis();
    started = true;
  }

  void loop() {
    if (!started)
      return;

    if (millis() - lastSampleMs < HISTORY_SAMPLE_INTERVAL_MS)
      return;

    take();
  }

  void sendJson(ESP8266WebServer &server) {
    const uint16_t length = ring.size();

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, F("application/json"), "");

    String buffer;
    buffer.reserve(640);

    buffer += F("{\"intervalSeconds\":");
    buffer += HISTORY_SAMPLE_INTERVAL_MS / 1000UL;
    buffer += F(",\"capacity\":");
    buffer += History::CAPACITY;
    buffer += F(",\"count\":");
    buffer += length;
    buffer += F(",\"series\":{");

    for (uint8_t series = 0; series < History::SERIES_COUNT; ++series) {
      if (series > 0)
        buffer += ',';

      buffer += '"';
      buffer += SERIES_NAMES[series];
      buffer += F("\":[");

      for (uint16_t index = 0; index < length; ++index) {
        if (index > 0)
          buffer += ',';

        appendSample(buffer, series, ring.at(series, index));

        // Flushed as it fills rather than assembled whole: the full payload is a few kilobytes and the heap
        // here is measured in tens of them.
        if (buffer.length() >= 512) {
          server.sendContent(buffer);
          buffer = "";
        }
      }

      buffer += ']';
    }

    buffer += F("}}");

    server.sendContent(buffer);
    server.sendContent("");
  }
}
