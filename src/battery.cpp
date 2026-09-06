#include <Arduino.h>

// Local includes
#include "app_config.h"
#include "battery.h"
#include "battery_curve.h"
#include "config.h"

namespace Battery {
  namespace {
    float lastVolts = 0.0f;
    bool hasSample = false;
    uint32_t lastSampleMs = 0;

    // The converter is noisy while the radio is transmitting, so a reading is
    // averaged over a short burst rather than taken from one conversion.
    float readVolts() {
      uint32_t total = 0;

      for (uint8_t i = 0; i < BATTERY_ADC_SAMPLES; ++i) {
        total += analogRead(A0);
        delay(2);
      }

      const float raw = static_cast<float>(total) / BATTERY_ADC_SAMPLES;

      return raw / BATTERY_ADC_FULL_SCALE * Config::get().batteryVoltageScale;
    }
  }

  void begin() {
    if (!isEnabled())
      return;

    lastVolts = readVolts();
    hasSample = true;
    lastSampleMs = millis();

    Serial.printf("[battery] %.2f V, %u%%\n", lastVolts, percent());
  }

  void loop() {
    if (!isEnabled())
      return;

    if (millis() - lastSampleMs < BATTERY_INTERVAL_MS)
      return;

    lastSampleMs = millis();
    lastVolts = readVolts();
    hasSample = true;
  }

  bool isEnabled() {
    return Config::get().batteryEnabled;
  }

  bool hasReading() {
    return isEnabled() && hasSample;
  }

  float volts() {
    return lastVolts;
  }

  uint8_t percent() {
    return BatteryCurve::percentFromVolts(lastVolts);
  }
}
