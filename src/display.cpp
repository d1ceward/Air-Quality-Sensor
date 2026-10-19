#include <U8g2lib.h>
#include <Wire.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "console.h"
#include "display.h"
#include "display_pages.h"
#include "display_schedule.h"
#include "net_time.h"
#include "net_wifi.h"
#include "sensors.h"

namespace Display {
  namespace {
    enum class Page : uint8_t { Sensors, Clock };

    // The panel is only known once the configuration is loaded, so the U8g2 object is built at boot rather
    // than declared statically.
    U8G2 *oled = nullptr;
    const DisplayPages::Layout *layout = nullptr;

    Page currentPage = Page::Sensors;
    uint32_t pageShownMs = 0;

    uint8_t appliedBrightness = 0;
    bool brightnessApplied = false;

    uint8_t scheduledBrightness() {
      const AppConfig &config = Config::get();
      struct tm now;

      // Without a synced clock the hour is unknown, so stay bright rather than risk blanking the display for
      // good.
      if (!NetTime::localTime(now))
        return config.displayBrightness;

      const bool night = DisplaySchedule::isNight(static_cast<uint8_t>(now.tm_hour),
                                                  config.displayNightStartHour,
                                                  config.displayNightEndHour);

      return night ? config.displayNightBrightness : config.displayBrightness;
    }

    // Only touches the panel when the setting actually changes
    void applyBrightness() {
      const uint8_t wanted = scheduledBrightness();
      if (brightnessApplied && wanted == appliedBrightness)
        return;

      appliedBrightness = wanted;
      brightnessApplied = true;

      // Zero switches the panel off rather than drawing an all black page, which an OLED still lights faintly
      // and still ages.
      if (wanted == 0) {
        oled->setPowerSave(1);
        return;
      }

      oled->setPowerSave(0);
      oled->setContrast(wanted);
    }

    // The clock slot carries the setup instructions, but only while the access point is really up: they name
    // an SSID and an address that do not exist otherwise. Before the first SNTP answer the slot has nothing
    // to show either, so it is skipped rather than filled with a clock full of dashes.
    bool drawSecondaryPage() {
      if (NetWifi::isAccessPointActive()) {
        layout->drawSetup(*oled);
        return true;
      }

      if (!NetTime::isSynced())
        return false;

      layout->drawClock(*oled);
      return true;
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
      Console.println(F("[display] not enough memory for the frame buffer"));
      return;
    }

    Console.printf("[display] %s\n", config.displayType.c_str());

    oled->setBusClock(I2C_BUS_CLOCK_HZ);
    oled->begin();
    applyBrightness();
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
    applyBrightness();

    // Nothing to draw while the panel is off
    if (appliedBrightness == 0)
      return;

    if (currentPage == Page::Sensors && clockEnabled && drawSecondaryPage()) {
      currentPage = Page::Clock;
      return;
    }

    currentPage = Page::Sensors;
    layout->drawSensors(*oled, Sensors::snapshot());
  }

  void showMessage(const String &line1, const String &line2, const String &line3) {
    if (oled == nullptr)
      return;

    // Boot and OTA progress are worth waking a sleeping panel for; the next page change puts it back to
    // whatever the schedule says.
    if (appliedBrightness == 0) {
      oled->setPowerSave(0);
      oled->setContrast(Config::get().displayBrightness);
      brightnessApplied = false;
    }

    layout->drawMessage(*oled, line1, line2, line3);

    // Keep the message visible for at least one page duration
    pageShownMs = millis();
    currentPage = Page::Sensors;
  }
}
