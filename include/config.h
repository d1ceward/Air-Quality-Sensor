#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Compile-time configuration.
// Anything a user may want to change at runtime lives in AppConfig instead.
// ---------------------------------------------------------------------------

// Project identification
constexpr char PROJECT_NAME[] = "Air Quality Sensor";
constexpr char PROJECT_VERSION[] = "1.0.0";

// Serial
constexpr uint32_t SERIAL_BAUD_RATE = 115200U;

// ---------------------------------------------------------------------------
// Pin map (AirGradient DIY PRO wiring, kept identical to the original sketch)
// ---------------------------------------------------------------------------

// Plantower PMS5003 particulate sensor, UART 9600 baud
constexpr uint8_t PIN_PMS_RX = D5;  // ESP receives on D5 (sensor TX)
constexpr uint8_t PIN_PMS_TX = D6;  // ESP transmits on D6 (sensor RX)

// SenseAir S8 CO2 sensor, Modbus RTU over UART 9600 baud
constexpr uint8_t PIN_CO2_RX = D4;  // ESP receives on D4 (sensor TX)
constexpr uint8_t PIN_CO2_TX = D3;  // ESP transmits on D3 (sensor RX)

// The SH1106 display, the SHT3x, the SGP30 and the BMP180 share the hardware
// I2C bus (D1 = SCL, D2 = SDA) at addresses 0x3C, 0x44, 0x58 and 0x77.
constexpr uint32_t I2C_BUS_CLOCK_HZ = 100000U;
constexpr uint8_t SHT3X_I2C_ADDRESS = 0x44;

// ---------------------------------------------------------------------------
// Sampling schedule
// ---------------------------------------------------------------------------

constexpr uint32_t CO2_INTERVAL_MS = 5000U;
// The SGP30 baseline algorithm requires a 1 Hz sampling rate (datasheet)
constexpr uint32_t TVOC_INTERVAL_MS = 1000U;
constexpr uint32_t TEMP_HUM_INTERVAL_MS = 2500U;
// Atmospheric pressure moves slowly and each reading blocks for about 60 ms
constexpr uint32_t PRESSURE_INTERVAL_MS = 10000U;

// The PMS5003 fan is duty cycled to extend its lifetime: it is woken up,
// left to stabilise, sampled, then put back to sleep.
constexpr uint32_t PMS_WARMUP_MS = 30000U;   // datasheet: stable after 30 s
constexpr uint32_t PMS_SAMPLE_MS = 10000U;   // averaging window after warm-up
constexpr uint32_t PMS_SLEEP_MS = 300000U;   // fan off between measurements

// A reading older than this is reported as unavailable
constexpr uint32_t READING_MAX_AGE_MS = 300000U;

// The particulate reading is only refreshed once per duty cycle, so it needs a
// longer validity window than the continuously sampled sensors. Derived from
// the cycle itself (one cycle and a half) so both stay consistent when the
// timings above are changed.
constexpr uint32_t PMS_READING_MAX_AGE_MS =
  (PMS_WARMUP_MS + PMS_SAMPLE_MS + PMS_SLEEP_MS) * 3U / 2U;

// SGP30 baseline is persisted so the sensor recovers its calibration on boot
constexpr uint32_t SGP30_BASELINE_SAVE_INTERVAL_MS = 3600000U;  // 1 h
constexpr uint32_t SGP30_BASELINE_MAX_AGE_S = 7U * 24U * 3600U; // 1 week

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

// Supported panels, selected at runtime through the configuration
constexpr char DISPLAY_TYPE_SH1106_128X64[] = "sh1106_128x64";
constexpr char DISPLAY_TYPE_SSD1306_64X48[] = "ssd1306_64x48";

// Contrast register of the panel, 0 turns the display off entirely
constexpr uint8_t DEFAULT_DISPLAY_BRIGHTNESS = 255U;
constexpr uint8_t DEFAULT_DISPLAY_NIGHT_BRIGHTNESS = 30U;
constexpr uint8_t DEFAULT_DISPLAY_NIGHT_START_HOUR = 22U;
constexpr uint8_t DEFAULT_DISPLAY_NIGHT_END_HOUR = 7U;

constexpr uint32_t DISPLAY_SENSORS_PAGE_MS = 6000U;
constexpr uint32_t DISPLAY_CLOCK_PAGE_MS = 4000U;

// ---------------------------------------------------------------------------
// Battery, when a Wemos D1 mini battery shield is fitted
// ---------------------------------------------------------------------------

constexpr uint32_t BATTERY_INTERVAL_MS = 10000U;
constexpr uint8_t BATTERY_ADC_SAMPLES = 8U;
constexpr float BATTERY_ADC_FULL_SCALE = 1023.0f;
// 130k on the shield plus the 220k/100k divider of the board: the 1 V full
// scale of the converter corresponds to 4.5 V on the cell.
constexpr float DEFAULT_BATTERY_VOLTAGE_SCALE = 4.5f;

// ---------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------

constexpr uint16_t DEFAULT_HTTP_PORT = 80U;
constexpr char DEFAULT_HOSTNAME[] = "air-quality";
constexpr char DEFAULT_NTP_SERVER[] = "europe.pool.ntp.org";
// POSIX timezone string: Central European Time with automatic DST
constexpr char DEFAULT_TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3";
// Guards the settings, the reboot and calibration calls, and the firmware
// upload form. An empty password leaves all of them open.
constexpr char DEFAULT_ADMIN_PASSWORD[] = "airquality";
constexpr char ADMIN_USER[] = "admin";

// Fallback access point used when no WiFi network is configured or reachable
constexpr char SETUP_AP_PASSWORD[] = "airquality";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000U;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 30000U;

// Configuration file on the LittleFS partition
constexpr char CONFIG_PATH[] = "/config.json";
