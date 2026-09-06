#include <Wire.h>

// Local includes
#include "config.h"
#include "sensors.h"

namespace Sensors {
  namespace {
    uint32_t lastCompensationMs = 0;

    // US EPA breakpoints: PM2.5 concentration high, AQI low, AQI high
    struct AqiBreakpoint {
      float concentrationLow;
      float concentrationHigh;
      float aqiLow;
      float aqiHigh;
    };

    const AqiBreakpoint AQI_BREAKPOINTS[] = {
      {0.0f, 12.0f, 0.0f, 50.0f},
      {12.1f, 35.4f, 51.0f, 100.0f},
      {35.5f, 55.4f, 101.0f, 150.0f},
      {55.5f, 150.4f, 151.0f, 200.0f},
      {150.5f, 250.4f, 201.0f, 300.0f},
      {250.5f, 350.4f, 301.0f, 400.0f},
      {350.5f, 500.4f, 401.0f, 500.0f},
    };

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
      result.aqiUs = pm25ToAqiUs(result.particulate.pm25);

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

  uint16_t pm25ToAqiUs(uint16_t pm25) {
    const float concentration = static_cast<float>(pm25);

    for (const AqiBreakpoint &breakpoint : AQI_BREAKPOINTS) {
      if (concentration > breakpoint.concentrationHigh)
        continue;

      const float span = breakpoint.concentrationHigh - breakpoint.concentrationLow;
      const float aqiSpan = breakpoint.aqiHigh - breakpoint.aqiLow;
      const float offset = concentration - breakpoint.concentrationLow;

      return static_cast<uint16_t>(aqiSpan / span * offset + breakpoint.aqiLow + 0.5f);
    }

    return 500;
  }
}
