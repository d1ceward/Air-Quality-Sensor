// Core entry point. High-level orchestration only.
#include <Arduino.h>
#include "config.h"
#include "display.h"

// Future: include sensor modules (e.g., air quality, temp/humidity) here.

static unsigned long bootMillis = 0;
static bool warmupDone = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for serial (ESP8266 often returns immediately) */ }
  Serial.println();
  Serial.println(F(PROJECT_NAME " starting..."));

  Display::begin();
  // Display::showStatus("Warming up", "sensors...", "");

  bootMillis = millis();
}

void loop() {
  const unsigned long now = millis();

  if (!warmupDone && (now - bootMillis) >= SENSOR_WARMUP_MS) {
    warmupDone = true;
    Serial.println(F("Warmup complete."));
    // Placeholder sensor values (replace with real readings)
    float tempC = 0.0f;
    float hum = 0.0f;
    Display::showStatus(String("T:") + tempC + "C", String("H:") + hum + "%", "Ready");
  }

  Display::loop();
  // Add sensor polling / WiFi handling / networking here.
}
