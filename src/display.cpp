#include <U8g2lib.h>
#include <Wire.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "display.h"
#include "display_pages.h"
#include "net_time.h"
#include "net_wifi.h"
#include "sensors.h"

namespace Display {
  namespace {
    enum class Page : uint8_t { Sensors, Clock };

    // The panel is only known once the configuration is loaded, so the U8g2
    // object is built at boot rather than declared statically.
    U8G2 *oled = nullptr;
    const DisplayPages::Layout *layout = nullptr;

    Page currentPage = Page::Sensors;
    uint32_t pageShownMs = 0;

    // The clock slot shows the setup instructions until the device is online
    void drawSecondaryPage() {
      if (NetWifi::isAccessPointActive() || !NetTime::isSynced()) {
        layout->drawSetup(*oled);
        return;
      }

      layout->drawClock(*oled);
    }
  }

  void begin() {
    const AppConfig &config = Config::get();
    const u8g2_cb_t *rotation = config.flipDisplay ? U8G2_R2 : U8G2_R0;

    if (config.displayType == DISPLAY_TYPE_SSD1306_64X48) {
      oled = new U8G2_SSD1306_64X48_ER_F_HW_I2C(rotation, U8X8_PIN_NONE);
      layout = &DisplayPages::COMPACT_64X48;
    } else {
      oled = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(rotation, U8X8_PIN_NONE);
      layout = &DisplayPages::WIDE_128X64;
    }

    if (oled == nullptr) {
      Serial.println(F("[display] not enough memory for the frame buffer"));
      return;
    }

    Serial.printf("[display] %s\n", config.displayType.c_str());

    oled->setBusClock(I2C_BUS_CLOCK_HZ);
    oled->begin();
    showMessage(PROJECT_NAME, String("v") + PROJECT_VERSION, "starting...");
  }

  void loop() {
    if (oled == nullptr)
      return;

    const bool clockEnabled = Config::get().showClockPage;
    const uint32_t pageDuration =
      currentPage == Page::Sensors ? DISPLAY_SENSORS_PAGE_MS : DISPLAY_CLOCK_PAGE_MS;

    if (pageShownMs != 0 && millis() - pageShownMs < pageDuration)
      return;

    pageShownMs = millis();

    if (currentPage == Page::Sensors && clockEnabled) {
      currentPage = Page::Clock;
      drawSecondaryPage();
      return;
    }

    currentPage = Page::Sensors;
    layout->drawSensors(*oled, Sensors::snapshot());
  }

  void showMessage(const String &line1, const String &line2, const String &line3) {
    if (oled == nullptr)
      return;

    layout->drawMessage(*oled, line1, line2, line3);

    // Keep the message visible for at least one page duration
    pageShownMs = millis();
    currentPage = Page::Sensors;
  }
}
