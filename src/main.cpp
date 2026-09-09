#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <memory>

// Local includes
#include "app_config.h"
#include "battery.h"
#include "config.h"
#include "display.h"
#include "history_recorder.h"
#include "metrics.h"
#include "net_ota.h"
#include "net_time.h"
#include "net_wifi.h"
#include "sensors.h"
#include "uptime.h"
#include "webui.h"

// The listening port comes from the runtime configuration, so the server is built once the configuration has
// been loaded.
static std::unique_ptr<ESP8266WebServer> server;

// A slave that was interrupted mid-byte by a prior reset can leave SDA held low, wedging the bus before
// anyone gets to call Wire.begin(). Clocking SCL manually lets it finish the byte it was stuck on and release
// the line.
static void recoverI2cBus() {
  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
  delayMicroseconds(5);

  if (digitalRead(SDA) == HIGH)
    return;

  pinMode(SCL, OUTPUT);
  for (uint8_t i = 0; i < 9 && digitalRead(SDA) == LOW; ++i) {
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
  }

  // Manual STOP condition: SDA low-to-high while SCL is high
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(SCL, HIGH);
  delayMicroseconds(5);
  digitalWrite(SDA, HIGH);
  delayMicroseconds(5);

  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
}

// A sensor keeps its state across a warm reset of the ESP, so one left in the middle of a measurement answers
// nothing until it is told to start over. The general call reset clears every device at once; it runs before
// the display is initialized because it resets that too.
static void resetI2cDevices() {
  Wire.beginTransmission(0x00);
  Wire.write(0x06);
  Wire.endTransmission();
  delay(10);
}

static void printBanner() {
  Serial.println();
  Serial.printf("%s %s\n", PROJECT_NAME, PROJECT_VERSION);
  Serial.printf("chip %06x, flash %u KiB, core %s\n",
                ESP.getChipId(), ESP.getFlashChipRealSize() / 1024, ESP.getCoreVersion().c_str());

  // Repeated banners mean the board is restarting; the reason says whether it is a brownout, a watchdog or a
  // crash.
  Serial.printf("boot: %s\n", ESP.getResetReason().c_str());
  Serial.printf("reset info: %s\n", ESP.getResetInfo().c_str());
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  printBanner();

  Uptime::tick(millis());

  Config::begin();

  recoverI2cBus();
  Wire.begin();
  Wire.setClock(I2C_BUS_CLOCK_HZ);
  resetI2cDevices();

  Display::begin();
  Battery::begin();
  Sensors::begin();

  NetWifi::begin();
  NetTime::begin();
  HistoryRecorder::begin();

  server.reset(new ESP8266WebServer(Config::get().httpPort));
  NetOta::begin(*server);
  WebUI::begin(*server);
}

void loop() {
  // Called on every iteration, well inside the 49.7 days it takes millis() to wrap, which is what lets
  // Uptime rebuild a count that does not fall back to zero
  Uptime::tick(millis());

  NetOta::loop();

  // Nothing else must touch the bus or the display during a firmware upload
  if (NetOta::isUpdating())
    return;

  NetWifi::loop();
  WebUI::loop();
  Sensors::loop();
  Battery::loop();
  Display::loop();
  HistoryRecorder::loop();
}
