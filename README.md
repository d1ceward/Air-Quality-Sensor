# Air Quality Sensor (ESP8266 D1 Mini)

Modularized PlatformIO project for an Air Quality Sensor using a D1 Mini (ESP8266) and an SSD1306 64x48 OLED.

## Project Structure
```
├─ platformio.ini          # Build configuration (environments, flags, libs)
├─ src/
│  ├─ main.cpp             # Application entry point (high-level orchestration)
│  └─ display.cpp          # Display abstraction implementation
├─ include/
│  ├─ config.h             # Global compile-time configuration/constants
│  └─ display.h            # Display abstraction interface
├─ lib/                    # (Optional) External custom libraries you create
├─ test/                   # Unit / integration tests (future)
└─ README.md
```

## Design Principles
- Separation of concerns: `main.cpp` orchestrates, modules do the work.
- Config centralization via `config.h`.
- Display isolated behind a small namespace API (`Display::showStatus`).
- Strict compiler warnings enabled (`-Wall -Wextra`) to catch issues early.
- Ready for expansion: add sensor modules (`sensors/air.cpp`, `wifi/manager.cpp`, etc.).

## Adding Sensors
Create a header in `include/` (e.g., `sensor_air.h`) and a source file in `src/` implementing:
```
namespace Sensors {
  void begin();
  void loop();
  float temperatureC();
  float humidity();
  // etc.
}
```
Call `Sensors::begin()` in `setup()` and `Sensors::loop()` in `loop()`. Query readings when updating the display.

## Testing
You can add a native (host) test environment by uncommenting the `env:native` block in `platformio.ini` and placing test files in `test/` using Unity test framework:
```
#include <unity.h>
void test_example() { TEST_ASSERT_EQUAL(4, 2+2); }
void setup() { UNITY_BEGIN(); RUN_TEST(test_example); UNITY_END(); }
void loop() {}
```
Flash builds exclude test code by default; tests run in their own environment.

## Build & Upload
From the project directory:
```
# Build
pio run
# Upload firmware
pio run -t upload
# Open monitor
pio device monitor -b 115200
```

## Next Steps / Suggestions
- Implement sensor modules for SHT and AirGradient sensors.
- Add WiFi provisioning with WiFiManager, then NTP time sync.
- Create a ring buffer for historical air quality metrics.
- Add OTA update support (ArduinoOTA / ESP8266HTTPUpdateServer).
- Add unit tests for pure logic (parsing, formatting, averaging).

## License
Specify your license here (e.g., MIT, Apache-2.0).
