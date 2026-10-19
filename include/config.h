#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------------------------------------
// Compile-time configuration. Anything a user may want to change at runtime lives in AppConfig instead.
// -----------------------------------------------------------------------------------------------------------

// Project identification
constexpr char PROJECT_NAME[] = "Air Quality Sensor";
constexpr char PROJECT_VERSION[] = "1.4.0";

// Serial
constexpr uint32_t SERIAL_BAUD_RATE = 115200U;

// -----------------------------------------------------------------------------------------------------------
// Pin map (AirGradient DIY PRO wiring, kept identical to the original sketch)
// -----------------------------------------------------------------------------------------------------------

// Plantower PMS5003 particulate sensor, UART 9600 baud
constexpr uint8_t PIN_PMS_RX = D5;  // ESP receives on D5 (sensor TX)
constexpr uint8_t PIN_PMS_TX = D6;  // ESP transmits on D6 (sensor RX)

// SenseAir S8 CO2 sensor, Modbus RTU over UART 9600 baud
constexpr uint8_t PIN_CO2_RX = D4;  // ESP receives on D4 (sensor TX)
constexpr uint8_t PIN_CO2_TX = D3;  // ESP transmits on D3 (sensor RX)

// The SH1106 display, the SHT3x, the SGP30 and the BMP180 share the hardware I2C bus (D1 = SCL, D2 = SDA) at
// addresses 0x3C, 0x44, 0x58 and 0x77.
constexpr uint32_t I2C_BUS_CLOCK_HZ = 100000U;
constexpr uint8_t SHT3X_I2C_ADDRESS = 0x44;

// -----------------------------------------------------------------------------------------------------------
// Sampling schedule
// -----------------------------------------------------------------------------------------------------------

constexpr uint32_t CO2_INTERVAL_MS = 5000U;
// The SGP30 baseline algorithm requires a 1 Hz sampling rate (datasheet)
constexpr uint32_t TVOC_INTERVAL_MS = 1000U;
constexpr uint32_t TEMP_HUM_INTERVAL_MS = 2500U;
// Atmospheric pressure moves slowly and each reading blocks for about 60 ms
constexpr uint32_t PRESSURE_INTERVAL_MS = 10000U;

// The PMS5003 fan is duty cycled to extend its lifetime: it is woken up, left to stabilise, sampled, then put
// back to sleep.
constexpr uint32_t PMS_WARMUP_MS = 30000U;   // datasheet: stable after 30 s
constexpr uint32_t PMS_SAMPLE_MS = 10000U;   // averaging window after warm-up
constexpr uint32_t PMS_SLEEP_MS = 300000U;   // fan off between measurements

// A reading older than this is reported as unavailable
constexpr uint32_t READING_MAX_AGE_MS = 300000U;

// A sensor that did not answer at boot is probed again on this interval. A bus still wedged when setup() ran,
// or a connector reseated while the board is powered, otherwise costs a reboot.
constexpr uint32_t SENSOR_RETRY_INTERVAL_MS = 30000U;

// The particulate reading is only refreshed once per duty cycle, so it needs a longer validity window than
// the continuously sampled sensors. Derived from the cycle itself (one cycle and a half) so both stay
// consistent when the timings above are changed.
constexpr uint32_t PMS_READING_MAX_AGE_MS =
  (PMS_WARMUP_MS + PMS_SAMPLE_MS + PMS_SLEEP_MS) * 3U / 2U;

// SGP30 baseline is persisted so the sensor recovers its calibration on boot
constexpr uint32_t SGP30_BASELINE_SAVE_INTERVAL_MS = 3600000U;  // 1 h
constexpr uint32_t SGP30_BASELINE_MAX_AGE_S = 7U * 24U * 3600U; // 1 week

// -----------------------------------------------------------------------------------------------------------
// Trend window kept in RAM for the web UI
// -----------------------------------------------------------------------------------------------------------

// History::CAPACITY slots at this interval, so two hours by default
constexpr uint32_t HISTORY_SAMPLE_INTERVAL_MS = 60000U;

// -----------------------------------------------------------------------------------------------------------
// Display
// -----------------------------------------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------------------------------------
// Battery, when a Wemos D1 mini battery shield is fitted
// -----------------------------------------------------------------------------------------------------------

constexpr uint32_t BATTERY_INTERVAL_MS = 10000U;
// Odd count: the burst is reduced to its median, which drops the spikes the radio puts on the converter where
// a mean would average them in.
constexpr uint8_t BATTERY_ADC_SAMPLES = 15U;
// Conversions taken back to back all see the same radio activity, so the burst is spread over about 60 ms
// rather than 15. One conversion is taken per loop() iteration, so this is a floor and not a delay.
constexpr uint32_t BATTERY_ADC_SAMPLE_GAP_MS = 4U;
constexpr float BATTERY_ADC_FULL_SCALE = 1023.0f;
// Weight of a new burst in the running voltage. At one burst every 10 s a real step is followed in about a
// minute, while the converter noise averages out.
constexpr float BATTERY_VOLTS_SMOOTHING = 0.2f;
// One converter step is 4.4 mV, worth about two points on the steep part of the discharge curve, so the
// reported figure only moves once it is out by more.
constexpr uint8_t BATTERY_PERCENT_DEADBAND = 3U;
// The resistor of the shield plus the 220k/100k divider of the board set what the 1 V full scale of the
// converter corresponds to on the cell: 4.2 V for the 100k shield fitted here, 4.5 V for the 130k of the
// standard one. Overridden by the batteryVoltageScale setting.
constexpr float DEFAULT_BATTERY_VOLTAGE_SCALE = 4.2f;
// What this board reads on a freshly charged cell, which anchors the 100 % end of the curve. A charger cuts
// out at 4.2 V, but the cell relaxes to about 4.15 V once the current stops and the fan of the PMS5003 pulls
// it down further, so anchoring at 4.2 V leaves a full battery stuck a few points short. Adjust it to the
// voltage the device settles on after a full charge. Overridden by the batteryFullVolts setting.
constexpr float DEFAULT_BATTERY_FULL_VOLTS = 4.15f;
// Bounds of that setting. Below the lower one the top of the curve would swallow most of the charge, above
// the upper one no cell would ever reach 100 %.
constexpr float BATTERY_FULL_VOLTS_MIN = 3.90f;
constexpr float BATTERY_FULL_VOLTS_MAX = 4.35f;

// -----------------------------------------------------------------------------------------------------------
// Network
// -----------------------------------------------------------------------------------------------------------

constexpr uint16_t DEFAULT_HTTP_PORT = 80U;
constexpr char DEFAULT_HOSTNAME[] = "air-quality";
constexpr char DEFAULT_NTP_SERVER[] = "europe.pool.ntp.org";
// POSIX timezone string: Central European Time with automatic DST
constexpr char DEFAULT_TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3";
// Guards the settings, the reboot and calibration calls, and the firmware upload form. An empty password
// leaves all of them open.
constexpr char DEFAULT_ADMIN_PASSWORD[] = "airquality";
constexpr char ADMIN_USER[] = "admin";

// Fallback access point used when no WiFi network is configured or reachable
constexpr char SETUP_AP_PASSWORD[] = "airquality";
// Wildcard DNS on the access point, so a phone joining it lands on the settings page on its own instead of
// having to be pointed at 192.168.4.1 by hand.
constexpr uint16_t CAPTIVE_PORTAL_DNS_PORT = 53U;
// Association plus DHCP after a warm reset regularly takes more than 20 s, and every second under it is a
// boot that lands in setup mode while the network was fine.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000U;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 30000U;

// -----------------------------------------------------------------------------------------------------------
// Files on the LittleFS partition
// -----------------------------------------------------------------------------------------------------------

constexpr char CONFIG_PATH[] = "/config.json";
// Written next to the target and renamed over it, so a power cut during a save cannot leave a truncated
// configuration behind
constexpr char CONFIG_TMP_PATH[] = "/config.json.tmp";

// The SGP30 baseline is refreshed every hour. Keeping it out of the configuration means that hourly write
// cannot take the WiFi credentials down with it.
constexpr char BASELINE_PATH[] = "/sgp30.json";
constexpr char BASELINE_TMP_PATH[] = "/sgp30.json.tmp";
