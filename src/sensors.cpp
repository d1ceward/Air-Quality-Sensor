#include <Wire.h>

// Local includes
#include "aqi.h"
#include "config.h"
#include "sensors.h"

namespace Sensors {
  namespace {
    uint32_t lastCompensationMs = 0;

    // The SGP30 compensates its readings with the ambient absolute humidity
    void updateHumidityCompensation() {
      if (!Sht3x::hasReading())
        return;

      if (millis() - lastCompensationMs < 30000U)
        return;

      lastCompensationMs = millis();
      Sgp30::setHumidityCompensation(Sht3x::temperatureC(), Sht3x::humidity());
    }
  }

  void begin() {
    Wire.begin();
    Wire.setClock(I2C_BUS_CLOCK_HZ);

    Sht3x::begin();
    Sgp30::begin();
    Bmp180::begin();
    SenseAirS8::begin();
    Pms5003::begin();
  }

  void loop() {
    Pms5003::loop();
    SenseAirS8::loop();
    Sgp30::loop();
    Sht3x::loop();
    Bmp180::loop();
    updateHumidityCompensation();
  }

  Snapshot snapshot() {
    Snapshot result;

    result.hasParticulate = Pms5003::hasReading();
    result.particulate = Pms5003::data();
    result.fanRunning = Pms5003::isFanRunning();
    if (result.hasParticulate)
      result.aqiUs = Aqi::fromPm25Us(result.particulate.pm25);

    result.hasCo2 = SenseAirS8::hasReading();
    result.co2Ppm = SenseAirS8::co2Ppm();

    result.hasTvoc = Sgp30::hasReading();
    result.tvocPpb = Sgp30::tvocPpb();
    result.eco2Ppm = Sgp30::eco2Ppm();

    result.hasClimate = Sht3x::hasReading();
    result.temperatureC = Sht3x::temperatureC();
    result.humidity = Sht3x::humidity();

    result.hasPressure = Bmp180::hasReading();
    result.pressureHpa = Bmp180::pressureHpa();
    result.seaLevelPressureHpa = Bmp180::seaLevelPressureHpa();
    result.altitudeM = Bmp180::altitudeM();
    result.barometerTemperatureC = Bmp180::temperatureC();

    return result;
  }
}
