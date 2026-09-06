#include <SGP30.h>
#include <Wire.h>
#include <time.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "sensors/sgp30.h"

namespace Sgp30 {
  namespace {
    SGP30 sensor;

    bool available = false;
    bool hasSample = false;
    uint32_t lastSampleMs = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t lastBaselineSaveMs = 0;

    uint16_t tvoc = 0;
    uint16_t eco2 = 0;

    // A stored baseline is only meaningful for about a week (datasheet).
    void restoreBaseline() {
      const AppConfig &config = Config::get();
      if (config.sgpBaselineCo2 == 0 && config.sgpBaselineTvoc == 0)
        return;

      const uint32_t now = static_cast<uint32_t>(time(nullptr));
      const bool clockValid = now > 1600000000UL && config.sgpBaselineEpoch > 0;

      if (clockValid && now - config.sgpBaselineEpoch > SGP30_BASELINE_MAX_AGE_S) {
        Serial.println(F("[sgp30] stored baseline too old, ignoring"));
        return;
      }

      sensor.setBaseline(config.sgpBaselineCo2, config.sgpBaselineTvoc);
      Serial.printf("[sgp30] baseline restored (eCO2 %u, TVOC %u)\n",
                    config.sgpBaselineCo2, config.sgpBaselineTvoc);
    }

    void persistBaseline() {
      uint16_t baselineCo2 = 0;
      uint16_t baselineTvoc = 0;

      if (!sensor.getBaseline(&baselineCo2, &baselineTvoc))
        return;

      AppConfig &config = Config::get();
      config.sgpBaselineCo2 = baselineCo2;
      config.sgpBaselineTvoc = baselineTvoc;
      config.sgpBaselineEpoch = static_cast<uint32_t>(time(nullptr));
      Config::save();

      Serial.printf("[sgp30] baseline saved (eCO2 %u, TVOC %u)\n", baselineCo2, baselineTvoc);
    }
  }

  bool begin() {
    available = sensor.begin() && sensor.isConnected();
    if (!available) {
      Serial.println(F("[sgp30] sensor not found"));
      return false;
    }

    restoreBaseline();
    lastBaselineSaveMs = millis();
    return true;
  }

  void loop() {
    if (!available)
      return;

    if (millis() - lastAttemptMs >= TVOC_INTERVAL_MS) {
      lastAttemptMs = millis();

      // Blocks for about 15 ms, the sensor requires a measurement every second
      if (sensor.measure(false)) {
        tvoc = sensor.getTVOC();
        eco2 = sensor.getCO2();
        hasSample = true;
        lastSampleMs = millis();
      }
    }

    if (hasSample && millis() - lastBaselineSaveMs >= SGP30_BASELINE_SAVE_INTERVAL_MS) {
      lastBaselineSaveMs = millis();
      persistBaseline();
    }
  }

  bool hasReading() {
    return hasSample && readingAgeMs() < READING_MAX_AGE_MS;
  }

  uint16_t tvocPpb() {
    return tvoc;
  }

  uint16_t eco2Ppm() {
    return eco2;
  }

  uint32_t readingAgeMs() {
    return hasSample ? millis() - lastSampleMs : UINT32_MAX;
  }

  void setHumidityCompensation(float temperatureC, float relativeHumidity) {
    if (!available)
      return;

    sensor.setRelHumidity(temperatureC, relativeHumidity);
  }
}
