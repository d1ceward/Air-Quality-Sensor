#include <Adafruit_BMP085.h>
#include <Wire.h>
#include <math.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "sensors/bmp180.h"

namespace Bmp180 {
  namespace {
    // International Standard Atmosphere reference
    constexpr float SEA_LEVEL_PRESSURE_HPA = 1013.25f;
    constexpr float BAROMETRIC_EXPONENT = 5.255f;
    constexpr float BAROMETRIC_SCALE_M = 44330.0f;

    Adafruit_BMP085 sensor;

    bool available = false;
    bool hasSample = false;
    uint32_t lastSampleMs = 0;
    uint32_t lastAttemptMs = 0;

    float pressure = 0.0f;
    float temperature = 0.0f;

    // Barometric formula, same relation used in both directions
    float pressureRatio(float altitudeMeters) {
      return powf(1.0f - altitudeMeters / BAROMETRIC_SCALE_M, BAROMETRIC_EXPONENT);
    }
  }

  bool begin() {
    // Ultra high resolution: 0.02 hPa, about 26 ms per conversion
    available = sensor.begin(BMP085_ULTRAHIGHRES, &Wire);
    if (!available) {
      Serial.println(F("[bmp180] sensor not found"));
      return false;
    }

    return true;
  }

  void loop() {
    if (!available)
      return;

    if (millis() - lastAttemptMs < PRESSURE_INTERVAL_MS)
      return;

    lastAttemptMs = millis();

    // Blocks for about 60 ms: one temperature conversion then one pressure
    // conversion, both required by the compensation formula.
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

  bool hasReading() {
    return hasSample && readingAgeMs() < READING_MAX_AGE_MS;
  }

  float pressureHpa() {
    return pressure;
  }

  float seaLevelPressureHpa() {
    const float altitude = Config::get().altitudeMeters;
    if (altitude == 0.0f)
      return pressure;

    return pressure / pressureRatio(altitude);
  }

  float altitudeM() {
    if (pressure <= 0.0f)
      return 0.0f;

    return BAROMETRIC_SCALE_M *
           (1.0f - powf(pressure / SEA_LEVEL_PRESSURE_HPA, 1.0f / BAROMETRIC_EXPONENT));
  }

  float temperatureC() {
    return temperature;
  }

  uint32_t readingAgeMs() {
    return hasSample ? millis() - lastSampleMs : UINT32_MAX;
  }
}
