#include <Arduino.h>
#include "config.h"
#include "display.h"

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);

  while (!Serial) { ; } // Wait for Serial to be ready

  Serial.println();
  Serial.println(F(PROJECT_NAME " starting..."));

  Display::begin();
}

void loop() {
  // lastUpdate = millis();
}
