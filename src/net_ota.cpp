#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "display.h"
#include "net_ota.h"
#include "net_wifi.h"

namespace NetOta {
  namespace {
    ESP8266HTTPUpdateServer httpUpdater;
    bool updating = false;

    void onStart() {
      updating = true;
      Serial.println(F("[ota] update started"));
      Display::showMessage("OTA update", "0%");
    }

    void onProgress(unsigned int progress, unsigned int total) {
      if (total == 0)
        return;

      static uint8_t lastPercent = 255;
      const uint8_t percent = static_cast<uint8_t>(progress * 100UL / total);

      if (percent == lastPercent)
        return;

      lastPercent = percent;
      Display::showMessage("OTA update", String(percent) + "%");
    }

    void onEnd() {
      updating = false;
      Serial.println(F("[ota] update finished, rebooting"));
      Display::showMessage("OTA update", "done", "rebooting");
    }

    void onError(ota_error_t error) {
      updating = false;
      Serial.printf("[ota] error %u\n", error);
      Display::showMessage("OTA update", "failed", String(error));
    }
  }

  void begin(ESP8266WebServer &server) {
    const AppConfig &config = Config::get();

    ArduinoOTA.setHostname(NetWifi::hostname().c_str());
    if (config.adminPassword.length() > 0)
      ArduinoOTA.setPassword(config.adminPassword.c_str());

    ArduinoOTA.onStart(onStart);
    ArduinoOTA.onProgress(onProgress);
    ArduinoOTA.onEnd(onEnd);
    ArduinoOTA.onError(onError);
    ArduinoOTA.begin();

    // Browser upload form, protected with the same password
    httpUpdater.setup(&server, "/update", ADMIN_USER, config.adminPassword.c_str());

    Serial.printf("[ota] ready (network upload and http://%s:%u/update)\n",
                  NetWifi::hostname().c_str(), config.httpPort);
  }

  void loop() {
    ArduinoOTA.handle();
  }

  bool isUpdating() {
    return updating || Update.isRunning();
  }
}
