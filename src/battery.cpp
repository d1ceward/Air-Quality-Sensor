#include <Arduino.h>

// Local includes
#include "app_config.h"
#include "battery.h"
#include "battery_curve.h"
#include "config.h"

namespace Battery {
  namespace {
    float smoothedVolts = 0.0f;
    uint8_t reportedPercent = 0;
    bool hasSample = false;
    uint32_t lastSampleMs = 0;

    // The converter is noisy, and the radio makes it worse while transmitting. A burst is reduced to its
    // median instead of its mean: a spike lands at one end of the sorted burst and is discarded, where a mean
    // would carry a twentieth of it into the result.
    float readVolts() {
      uint16_t samples[BATTERY_ADC_SAMPLES];

      for (uint8_t i = 0; i < BATTERY_ADC_SAMPLES; ++i) {
        samples[i] = analogRead(A0);
        delay(BATTERY_ADC_SAMPLE_GAP_MS);
      }

      // Insertion sort: the burst is short and only its middle is needed
      for (uint8_t i = 1; i < BATTERY_ADC_SAMPLES; ++i) {
        const uint16_t value = samples[i];
        uint8_t j = i;

        while (j > 0 && samples[j - 1] > value) {
          samples[j] = samples[j - 1];
          --j;
        }

        samples[j] = value;
      }

      const float median = static_cast<float>(samples[BATTERY_ADC_SAMPLES / 2]);

      return median / BATTERY_ADC_FULL_SCALE * Config::get().batteryVoltageScale;
    }

    void takeSample() {
      const float volts = readVolts();

      if (hasSample) {
        smoothedVolts += BATTERY_VOLTS_SMOOTHING * (volts - smoothedVolts);
        reportedPercent = BatteryCurve::stablePercent(reportedPercent,
            BatteryCurve::percentFromVolts(smoothedVolts), BATTERY_PERCENT_DEADBAND);
      } else {
        // Nothing to average against yet, so the first burst stands as it is
        smoothedVolts = volts;
        reportedPercent = BatteryCurve::percentFromVolts(smoothedVolts);
        hasSample = true;
      }

      lastSampleMs = millis();
    }
  }

  void begin() {
    if (!isEnabled())
      return;

    takeSample();

    Serial.printf("[battery] %.2f V, %u%%\n", smoothedVolts, reportedPercent);
  }

  void loop() {
    if (!isEnabled())
      return;

    if (millis() - lastSampleMs < BATTERY_INTERVAL_MS)
      return;

    takeSample();
  }

  bool isEnabled() {
    return Config::get().batteryEnabled;
  }

  bool hasReading() {
    return isEnabled() && hasSample;
  }

  float volts() {
    return smoothedVolts;
  }

  uint8_t percent() {
    return reportedPercent;
  }
}
