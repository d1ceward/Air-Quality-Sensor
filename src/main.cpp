#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <memory>

// Local includes
#include "app_config.h"
#include "config.h"
#include "display.h"
#include "metrics.h"
#include "net_ota.h"
#include "net_time.h"
#include "net_wifi.h"
#include "sensors.h"
#include "webui.h"

// The listening port comes from the runtime configuration, so the server is
// built once the configuration has been loaded.
static std::unique_ptr<ESP8266WebServer> server;

static void printBanner() {
  Serial.println();
  Serial.printf("%s %s\n", PROJECT_NAME, PROJECT_VERSION);
  Serial.printf("chip %06x, flash %u KiB, core %s\n",
                ESP.getChipId(), ESP.getFlashChipRealSize() / 1024, ESP.getCoreVersion().c_str());
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  printBanner();

  Config::begin();
  Display::begin();
  Sensors::begin();

  NetWifi::begin();
  NetTime::begin();

  server.reset(new ESP8266WebServer(Config::get().httpPort));
  NetOta::begin(*server);
  WebUI::begin(*server);
}

void loop() {
  NetOta::loop();

  // Nothing else must touch the bus or the display during a firmware upload
  if (NetOta::isUpdating())
    return;

  NetWifi::loop();
  WebUI::loop();
  Sensors::loop();
  Display::loop();
}
