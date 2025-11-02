#include <Arduino.h>

// Local includes
#include "config.h"
#include "sensor_serial.h"

namespace SensorSerial {
  void initializeSerial() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Allow serial monitor to connect
    delay(SERIAL_CONNECT_DELAY_MS);

    Serial.println();
    Serial.println(String(PROJECT_NAME) + " starting...");
  }
}
