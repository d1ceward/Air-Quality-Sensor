#include "display.h"
#include "config.h"

// 64x48 SSD1306 (example) - adjust if using different variant
static U8G2_SSD1306_64X48_ER_1_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

namespace Display {
  void begin() {
    u8g2.setBusClock(100000); // safe default
    u8g2.begin();
    drawStrings(PROJECT_NAME, "v" PROJECT_VERSION, "Booting...");
  }

  void drawStrings(const String &line1, const String &line2, const String &line3) {
    u8g2.firstPage();

    do {
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(1, 8, line1.c_str());
      u8g2.drawStr(1, 16, line2.c_str());
      u8g2.drawStr(1, 24, line3.c_str());
    } while (u8g2.nextPage());
  }
}
