#include <SHTSensor.h>
#include <Wire.h>

// Local includes
#include "config.h"
#include "console.h"
#include "sensors/sht3x.h"

namespace Sht3x {
  namespace {
    // SHT3X maps to I2C address 0x44 (ADDR pin tied to VSS)
    SHTSensor sensor(SHTSensor::SHT3X);

    bool available = false;
    bool hasSample = false;
    uint32_t lastSampleMs = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t lastProbeMs = 0;

    float temperature = 0.0f;
    float relativeHumidity = 0.0f;

    bool probe() {
      available = sensor.init();
      lastProbeMs = millis();

      if (!available)
        return false;

      sensor.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH);

      return true;
    }
  }

  bool begin() {
    static_assert(SHT3X_I2C_ADDRESS == 0x44, "SHTSensor::SHT3X is hard-wired to address 0x44");

    if (!probe()) {
      Console.println(F("[sht3x] sensor not found, retrying in the background"));
      return false;
    }

    return true;
  }

  void loop() {
    // A bus still wedged when setup() ran, or a connector reseated while the board is powered, used to cost
    // a reboot. The probe is cheap, so it is simply retried until it answers.
    if (!available) {
      if (millis() - lastProbeMs < SENSOR_RETRY_INTERVAL_MS)
        return;

      if (!probe())
        return;

      Console.println(F("[sht3x] sensor found"));
    }

    if (millis() - lastAttemptMs < TEMP_HUM_INTERVAL_MS)
      return;

    lastAttemptMs = millis();

    if (!sensor.readSample())
      return;

    const float sampledTemperature = sensor.getTemperature();
    const float sampledHumidity = sensor.getHumidity();

    // The driver reports NAN when a sample could not be decoded
    if (isnan(sampledTemperature) || isnan(sampledHumidity))
      return;

    temperature = sampledTemperature;
    relativeHumidity = sampledHumidity;
    hasSample = true;
    lastSampleMs = millis();
  }

  bool isAvailable() {
    return available;
  }

  bool hasReading() {
    return hasSample && readingAgeMs() < READING_MAX_AGE_MS;
  }

  float temperatureC() {
    return temperature;
  }

  float humidity() {
    return relativeHumidity;
  }

  uint32_t readingAgeMs() {
    return hasSample ? millis() - lastSampleMs : UINT32_MAX;
  }
}
