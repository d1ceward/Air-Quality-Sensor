#include <Adafruit_BMP085.h>
#include <Wire.h>
#include <math.h>

// Local includes
#include "app_config.h"
#include "barometry.h"
#include "config.h"
#include "console.h"
#include "sensors/bmp180.h"

namespace Bmp180 {
  namespace {
    Adafruit_BMP085 sensor;

    bool available = false;
    bool hasSample = false;
    uint32_t lastSampleMs = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t lastProbeMs = 0;

    float pressure = 0.0f;
    float temperature = 0.0f;

    bool probe() {
      // Ultra high resolution: 0.02 hPa, about 26 ms per conversion
      available = sensor.begin(BMP085_ULTRAHIGHRES, &Wire);
      lastProbeMs = millis();

      return available;
    }
  }

  bool begin() {
    if (!probe()) {
      Console.println(F("[bmp180] sensor not found, retrying in the background"));
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

      Console.println(F("[bmp180] sensor found"));
    }

    if (millis() - lastAttemptMs < PRESSURE_INTERVAL_MS)
      return;

    lastAttemptMs = millis();

    // Blocks for about 60 ms: one temperature conversion then one pressure conversion, both required by the
    // compensation formula.
    const float sampledTemperature = sensor.readTemperature();
    const int32_t sampledPressurePa = sensor.readPressure();

    // The library returns 0 when a conversion fails
    if (sampledPressurePa <= 0 || isnan(sampledTemperature))
      return;

    temperature = sampledTemperature;
    pressure = static_cast<float>(sampledPressurePa) / 100.0f;
    hasSample = true;
    lastSampleMs = millis();
  }

  bool isAvailable() {
    return available;
  }

  bool hasReading() {
    return hasSample && readingAgeMs() < READING_MAX_AGE_MS;
  }

  float pressureHpa() {
    return pressure;
  }

  float seaLevelPressureHpa() {
    return Barometry::toSeaLevelHpa(pressure, Config::get().altitudeMeters);
  }

  float altitudeM() {
    return Barometry::toAltitudeMeters(pressure);
  }

  float temperatureC() {
    return temperature;
  }

  uint32_t readingAgeMs() {
    return hasSample ? millis() - lastSampleMs : UINT32_MAX;
  }
}
