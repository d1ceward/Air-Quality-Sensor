#include <SGP30.h>
#include <Wire.h>
#include <time.h>

// Local includes
#include "app_config.h"
#include "baseline_store.h"
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
    uint32_t lastProbeMs = 0;

    uint16_t tvoc = 0;
    uint16_t eco2 = 0;

    // A stored baseline is only meaningful for about a week (datasheet).
    void restoreBaseline() {
      BaselineStore::Baseline baseline;
      if (!BaselineStore::load(baseline))
        return;

      const uint32_t now = static_cast<uint32_t>(time(nullptr));
      const bool clockValid = now > 1600000000UL && baseline.epoch > 0;

      if (clockValid && now - baseline.epoch > SGP30_BASELINE_MAX_AGE_S) {
        Serial.println(F("[sgp30] stored baseline too old, ignoring"));
        return;
      }

      sensor.setBaseline(baseline.eco2, baseline.tvoc);
      Serial.printf("[sgp30] baseline restored (eCO2 %u, TVOC %u)\n", baseline.eco2, baseline.tvoc);
    }

    void persistBaseline() {
      BaselineStore::Baseline baseline;

      if (!sensor.getBaseline(&baseline.eco2, &baseline.tvoc))
        return;

      baseline.epoch = static_cast<uint32_t>(time(nullptr));

      if (!BaselineStore::save(baseline))
        return;

      Serial.printf("[sgp30] baseline saved (eCO2 %u, TVOC %u)\n", baseline.eco2, baseline.tvoc);
    }

    bool probe() {
      available = sensor.begin() && sensor.isConnected();
      lastProbeMs = millis();

      if (!available)
        return false;

      restoreBaseline();
      lastBaselineSaveMs = millis();

      return true;
    }
  }

  bool begin() {
    if (!probe()) {
      Serial.println(F("[sgp30] sensor not found, retrying in the background"));
      return false;
    }

    return true;
  }

  void loop() {
    // Probed again until it answers rather than written off for the rest of the run
    if (!available) {
      if (millis() - lastProbeMs < SENSOR_RETRY_INTERVAL_MS)
        return;

      if (!probe())
        return;

      Serial.println(F("[sgp30] sensor found"));
    }

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

  bool isAvailable() {
    return available;
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
