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

    // A burst is collected one conversion per loop() iteration. Taking the fifteen of them back to back
    // would block for about 60 ms, and nothing else runs during that: the web server stops answering and the
    // PMS5003 stream piles up in its receive buffer.
    uint16_t burst[BATTERY_ADC_SAMPLES] = {0};
    uint8_t burstCount = 0;
    bool bursting = false;
    uint32_t lastConversionMs = 0;

    void startBurst() {
      burstCount = 0;
      bursting = true;
      // Backdated so the first conversion happens on the next iteration rather than one gap later
      lastConversionMs = millis() - BATTERY_ADC_SAMPLE_GAP_MS;
    }

    // The converter is noisy, and the radio makes it worse while transmitting. A burst is reduced to its
    // median instead of its mean: a spike lands at one end of the sorted burst and is discarded, where a mean
    // would carry a fifteenth of it into the result.
    float burstVolts() {
      // Insertion sort: the burst is short and only its middle is needed
      for (uint8_t i = 1; i < BATTERY_ADC_SAMPLES; ++i) {
        const uint16_t value = burst[i];
        uint8_t j = i;

        while (j > 0 && burst[j - 1] > value) {
          burst[j] = burst[j - 1];
          --j;
        }

        burst[j] = value;
      }

      const float median = static_cast<float>(burst[BATTERY_ADC_SAMPLES / 2]);

      return median / BATTERY_ADC_FULL_SCALE * Config::get().batteryVoltageScale;
    }

    void commitBurst() {
      const float volts = burstVolts();

      if (hasSample) {
        smoothedVolts += BATTERY_VOLTS_SMOOTHING * (volts - smoothedVolts);
        reportedPercent = BatteryCurve::stablePercent(reportedPercent,
            BatteryCurve::percentFromVolts(smoothedVolts, Config::get().batteryFullVolts),
            BATTERY_PERCENT_DEADBAND);
      } else {
        // Nothing to average against yet, so the first burst stands as it is
        smoothedVolts = volts;
        reportedPercent = BatteryCurve::percentFromVolts(smoothedVolts, Config::get().batteryFullVolts);
        hasSample = true;

        Serial.printf("[battery] %.2f V, %u%%\n", smoothedVolts, reportedPercent);
      }

      lastSampleMs = millis();
    }
  }

  void begin() {
    if (!isEnabled())
      return;

    // The first burst finishes a few iterations into loop(), so the boot log line comes from commitBurst()
    startBurst();
  }

  void loop() {
    if (!isEnabled())
      return;

    if (bursting) {
      // Conversions taken back to back all see the same radio activity, so the burst is spread out
      if (millis() - lastConversionMs < BATTERY_ADC_SAMPLE_GAP_MS)
        return;

      lastConversionMs = millis();
      burst[burstCount++] = static_cast<uint16_t>(analogRead(A0));

      if (burstCount < BATTERY_ADC_SAMPLES)
        return;

      bursting = false;
      commitBurst();

      return;
    }

    if (millis() - lastSampleMs >= BATTERY_INTERVAL_MS)
      startBurst();
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
