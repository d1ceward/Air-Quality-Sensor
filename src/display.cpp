#include "display.h"
#include "config.h"

// 64x48 SSD1306 (example) - adjust if using different variant
static U8G2_SSD1306_64X48_ER_1_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

namespace {
unsigned long lastUpdate = 0;
String cachedL1, cachedL2, cachedL3;
}

namespace Display {

void begin() {
  u8g2.setBusClock(100000); // safe default
  u8g2.begin();
  showSplash();
}

static void drawStrings(const String &l1, const String &l2, const String &l3) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(1, 8, l1.c_str());
    u8g2.drawStr(1, 16, l2.c_str());
    u8g2.drawStr(1, 24, l3.c_str());
  } while (u8g2.nextPage());
}

void showSplash() {
  showStatus(PROJECT_NAME, "v" PROJECT_VERSION, "Booting...");
}

void showStatus(const String &line1, const String &line2, const String &line3) {
  cachedL1 = line1; cachedL2 = line2; cachedL3 = line3;
  drawStrings(cachedL1, cachedL2, cachedL3);
  lastUpdate = millis();
}

void loop() {
  // For periodic refresh logic if needed later (animations, blinking, etc.)
  (void)lastUpdate; // suppress unused warning for now
}

} // namespace Display
