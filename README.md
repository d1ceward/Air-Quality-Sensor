# Air Quality Sensor

Firmware for an ESP8266 based air quality station (AirGradient DIY PRO hardware),
exposing its readings as Prometheus metrics.

Rewritten from the original single-file Arduino sketch
([original_arduino_software.c](original_arduino_software.c)) as a modular
PlatformIO project, with no dependency on the AirGradient library: every sensor
protocol is implemented locally.

## Features

- Prometheus metrics on `/metrics`, metric names unchanged from the original firmware
- Web UI with live readings, device information and full configuration
- OTA updates, both from PlatformIO over the network and from a browser
- WiFi provisioning through a fallback access point, no credentials in the source
- Configuration stored as JSON on LittleFS, editable without reflashing
- NTP time with POSIX timezone rules, so DST is handled automatically
- SGP30 baseline persistence and humidity compensation
- Barometric pressure reduced to sea level from a configurable station altitude
- SenseAir S8 readings validated with a Modbus CRC, manual calibration from the UI

## Hardware

| Sensor | Measures | Bus | Pins |
| --- | --- | --- | --- |
| Plantower PMS5003 | PM1.0 / PM2.5 / PM10, particle counts | UART 9600 | D5 (RX), D6 (TX) |
| SenseAir S8 | CO2 | Modbus RTU 9600 | D4 (RX), D3 (TX) |
| Sensirion SHT3x | temperature, humidity | I2C `0x44` | D1 (SCL), D2 (SDA) |
| Sensirion SGP30 | TVOC, eCO2 | I2C `0x58` | D1 (SCL), D2 (SDA) |
| Bosch BMP180 (GY-68) | barometric pressure, altitude | I2C `0x77` | D1 (SCL), D2 (SDA) |
| SH1106 OLED 128x64 | display | I2C `0x3C` | D1 (SCL), D2 (SDA) |
| SSD1306 OLED 64x48 | display, smaller board | I2C `0x3C` | D1 (SCL), D2 (SDA) |
| Wemos battery shield | LiPo charging, 5 V boost | analog | A0 through the shield jumper |

The pin map lives in [include/config.h](include/config.h).

### Battery

The 128x64 board can run from a Wemos D1 mini battery shield. Enable it under
Battery in the web UI; it stays off by default so the other board is unaffected.

Voltage is read on A0: the 130 kOhm resistor of the shield sits in series with
the 220k/100k divider of the board, so the 1 V full scale of the converter
lands at 4.5 V on the cell. That is the `batteryVoltageScale` setting, to trim
against a multimeter if the resistor tolerances shift the reading.

Expect around 1.6 hours from a 500 mAh cell. The load is close to 173 mA on the
5 V rail, of which 48 mA is the SGP30 alone: its hotplate runs continuously and
its baseline algorithm needs an uninterrupted 1 Hz measurement, so it cannot be
duty cycled without losing its calibration. Peaks matter more than the average
here: the SenseAir S8 draws up to 300 mA during a measurement and the radio as
much while transmitting, which is about 1.17 A from the cell through the boost,
or 2.3 C on a 500 mAh pack. Both the cell and the shield need to sustain that
or the board will brown out and reboot.

A percentage under load reads low: a LiPo sags by 50 to 100 mV at this current,
and the curve is steep in that region.

### Night dimming

The panel contrast drops inside a configurable window, 22:00 to 07:00 by
default. A night brightness of 0 switches the panel off entirely rather than
drawing a black page, which an OLED still lights faintly and still ages. That
matters here: the layout is static and shown around the clock, so a dimmer night
also buys screen life.

The window is half open, `[from, until)`: with 22 and 7, hour 22 is dim and hour
7 is bright again. Setting both hours to the same value turns the feature off.

Until SNTP has answered the hour is unknown, and the display stays at full
brightness rather than risk blanking itself for good.

### Two boards, one firmware

The same image runs on the full board and on the smaller one, which carries only
the SenseAir S8, the PMS5003, an SHT3x and a 64x48 display. Nothing has to be
declared: the SGP30 and the BMP180 fail their I2C probe, their metrics are
omitted and their cards disappear from the web UI. Both boards wire the two
serial sensors to the same pins.

The panel is the only thing that cannot be detected, so it is a setting. Pick it
under Display in the web UI, or set `displayType` in `/config.json` to
`sh1106_128x64` or `ssd1306_64x48`.

The 64x48 layout shows CO2, PM2.5, temperature and humidity. It has no room for
TVOC or pressure, so a full sensor board paired with the small panel would only
report those two through the web UI and the metrics.

## Project structure

```
platformio.ini              Build environments (serial and OTA upload)
include/
  config.h                  Compile-time configuration: pins, intervals, defaults
  app_config.h              Runtime configuration structure
  display.h                 Page rotation, panel selected at boot
  display_pages.h           One renderer per panel geometry
  display_schedule.h        Night window, no hardware dependency
  metrics.h                 Prometheus and JSON serialisation
  net_ota.h                 ArduinoOTA and browser firmware upload
  net_time.h                SNTP and timezone handling
  net_wifi.h                Station mode, setup access point, mDNS
  sensors.h                 Aggregated snapshot and US AQI conversion
  sensors/bmp180.h          Barometric pressure, sea level correction
  aqi.h                     US EPA index, no hardware dependency
  battery.h                 Cell voltage on A0
  battery_curve.h           LiPo state of charge, no hardware dependency
  barometry.h               Pressure conversions, no hardware dependency
  modbus_crc.h              Modbus CRC16, no hardware dependency
  sensors/pms5003.h         PMS5003 fan duty cycle and serial plumbing
  sensors/pms_frame.h       Plantower frame parser, no hardware dependency
  sensors/senseair_s8.h     Modbus RTU client
  sensors/sgp30.h           TVOC with baseline persistence
  sensors/sht3x.h           Temperature and humidity
  web_page.h                Web UI, embedded in flash
  webui.h                   HTTP routes
src/                        Matching implementations
```

Each module owns its own state and exposes a small namespace API. `main.cpp`
only wires them together.

## Tests

The protocol and conversion logic is kept free of Arduino dependencies so it can
run on the host, in about two seconds and with no board attached:

```sh
pio test -e native
```

The environment matters: `pio test` on its own follows `default_envs`, which is
the firmware, where these suites are ignored and nothing runs.

Thirty two cases cover the PMS5003 frame parser (checksum rejection, resynchro-
nisation after garbage, payload offsets), the Modbus CRC16 against frames from
the SenseAir S8 documentation, the EPA breakpoint table, the barometric
conversions, the LiPo discharge curve and the night window, midnight wrap
included. Expected values come from the datasheets or from an independent
computation, never from the implementation under test.

Everything else needs the hardware: the drivers, the web server and the display
are exercised on the device.

## Build and flash

```sh
# First flash, over USB
pio run -t upload

# Serial monitor
pio device monitor

# Later updates, over the network
pio run -e d1_mini_ota -t upload
```

Adjust `upload_port` and `--auth` in the `[env:d1_mini_ota]` section of
[platformio.ini](platformio.ini) to match the device address and its OTA
password.

## First boot

With no WiFi credentials stored, the device starts an access point named
`air-quality-setup-<chipid>` with the password `airquality`, and shows it on the
display. Connect to it, open `http://192.168.4.1/`, fill in the WiFi
settings and save: the device reboots and joins the network.

Afterwards the UI is reachable at `http://<ip>/` or
`http://air-quality-<chipid>.local/`.

## HTTP endpoints

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/` | Web UI |
| GET | `/metrics` | Prometheus exposition |
| GET | `/api/status` | Readings and device state, JSON |
| GET | `/api/config` | Current configuration, secrets redacted, password protected |
| POST | `/api/config` | Update configuration, then reboot, password protected |
| POST | `/api/reboot` | Reboot, password protected |
| POST | `/api/calibrate-co2` | Start SenseAir S8 background calibration, password protected |
| GET, POST | `/update` | Firmware upload form |

### Authentication

The administrative routes, `/api/config` both ways, `/api/reboot`,
`/api/calibrate-co2` and `/update`, ask for HTTP basic authentication with the
user `admin` and the admin password. The same password is what ArduinoOTA uses,
so there is a single secret to manage. Clearing it in the settings leaves
everything open.

`/`, `/metrics` and `/api/status` stay open: a scraper cannot be asked for
credentials, and the readings are what the device exists to publish.

Basic authentication over plain HTTP means the password crosses the network
base64 encoded, which is not encryption. It keeps a household from changing the
settings by accident; it does not stand up to anyone capturing traffic on the
link. TLS on an ESP8266 web server is not a practical alternative.

## Prometheus

```yaml
scrape_configs:
  - job_name: air_quality
    metrics_path: /metrics
    static_configs:
      - targets: ['192.168.1.2:80']
```

The original firmware also answered on `/`; that path now serves the web UI, so
a scrape configuration pointing at `/` must be updated to `/metrics`.

Metrics `tvoc`, `co2`, `pm25`, `aqius`, `temp` and `hum` keep their original
names and their `id` and `mac` labels. Added: `pm1`, `pm10`, `eco2`,
`particle_count_03`, `particle_count_25`, `pressure`, `pressure_sea_level`,
`altitude`, `battery_volts`, `battery_percent`, `wifi_rssi`, `uptime_seconds`
and `heap_free_bytes`.

Pressure is exported in hPa. `pressure` is what the sensor measures,
`pressure_sea_level` is that value reduced to sea level using the configured
station altitude, which is the figure comparable to weather forecasts.
`altitude` is derived from the standard atmosphere (1013.25 hPa) and drifts with
the weather, so it is a rough indication rather than a position.

A metric is omitted while its sensor has no fresh reading, instead of reporting
a misleading zero. The PMS5003 fan is duty cycled to preserve its laser and its
fan, so `pm25` is refreshed once per cycle, about every six minutes.

## Configuration

Everything below is editable from the web UI and stored in `/config.json` on the
LittleFS partition.

| Setting | Default | Notes |
| --- | --- | --- |
| Device id | `air-quality-<chipid>` | Prometheus `id` label, hostname and mDNS name |
| WiFi network, password | empty | Empty password field keeps the stored one |
| Static IP | off | Address, gateway, subnet and DNS |
| HTTP port | `80` | |
| Station altitude | `0` m | Used to reduce pressure to sea level |
| NTP server | `europe.pool.ntp.org` | |
| Timezone | `CET-1CEST,M3.5.0,M10.5.0/3` | POSIX TZ string, DST included |
| Admin password | `airquality` | Settings, reboot, calibration, ArduinoOTA and `/update` |
| Battery shield | off | Enables the A0 reading and the gauge |
| Battery ADC scale | `4.5` V | Full scale of the divider, trim against a multimeter |
| Display | `sh1106_128x64` | Or `ssd1306_64x48` for the smaller board |
| Brightness | `255` | Panel contrast during the day |
| Night brightness | `30` | `0` switches the panel off |
| Night from, until | `22`, `7` | Equal hours disable the dimming |
| Clock page | on | Alternate the display with date and time |
| Rotate display | off | For a display mounted the other way up |

## Differences from the original sketch

- No AirGradient library: the PMS5003 frame parser, the SenseAir S8 Modbus
  client and the sensor scheduling are implemented in this repository
- Credentials and network settings moved out of the source into `/config.json`
- SGP30 sampled at 1 Hz as its baseline algorithm requires, with the baseline
  saved hourly and restored on boot
- SenseAir S8 responses validated with a Modbus CRC instead of scanning for a
  start byte
- PM2.5 averaged over a sampling window instead of a single frame
- NTP with real timezone rules, replacing the fixed `+2 h` offset
- US AQI computed with the official EPA breakpoints (values can differ by one
  index point from the original approximation)
- Barometric pressure added with the BMP180. Its own temperature reading is only
  exposed on `/api/status` for diagnostics: the die self-heats, so the SHT3x
  remains the source of the `temp` metric
