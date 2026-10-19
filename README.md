# Air Quality Sensor

[![CI](https://github.com/d1ceward/Air-Quality-Sensor/actions/workflows/ci.yml/badge.svg)](https://github.com/d1ceward/Air-Quality-Sensor/actions/workflows/ci.yml)
[![GitHub license](https://img.shields.io/github/license/d1ceward/Air-Quality-Sensor)](LICENSE)

**ESP8266 firmware for AirGradient DIY air quality kits, exposing CO2, particulates, TVOC, temperature,
humidity and pressure as Prometheus metrics, with a web UI, OTA updates and no credentials in the source.**

Built with PlatformIO, with no dependency on the AirGradient library: every sensor protocol is implemented
locally. One firmware image runs on both the full kit and the smaller one.

## Background

It started with an AirGradient DIY kit to solder myself. From day one I wrote my own program for it rather
than running the stock one, still on top of the AirGradient Arduino library. A few of my solder joints on
that first kit did not hold up, so I bought the newer kit, a revision AirGradient was still developing at
the time, and got my program running on it instead.

That program was a single Arduino file whose job was to export the readings to Prometheus. Over time it
gained a barometric sensor and a better layout on the small screen. An early snapshot of that single-file version
is kept in the history of this repository:
[original_arduino_software.c](https://github.com/d1ceward/Air-Quality-Sensor/blob/9ecfa53/original_arduino_software.c).

Much later I repaired the first kit and took the chance to start the program over: a PlatformIO project
split into modules, sensor drivers written from the datasheets instead of the library, a web UI for
everything that used to need a reflash, then battery support. Both kits now run this same firmware, and
that is this repository.

## Features

- Prometheus metrics on `/metrics`, every name under an `air_quality_` prefix
- Web UI with live readings, a two hour trend per reading, device information, serial logs and full
  configuration
- WiFi provisioning through a captive portal access point, with a network scan
- OTA updates, both from PlatformIO over the network and from a browser
- Configuration stored as JSON on LittleFS, written atomically, editable without reflashing
- NTP time with POSIX timezone rules, so DST is handled automatically
- SGP30 baseline persistence and humidity compensation
- Barometric pressure reduced to sea level from a configurable station altitude
- SenseAir S8 readings validated with a Modbus CRC, manual calibration from the UI
- Optional LiPo battery gauge on a Wemos D1 mini battery shield
- Night dimming of the display
- Sensors probed again while they stay silent, so a wedged bus does not cost a reboot
- Ready-made Grafana dashboard

## Hardware

| Part | Measures | Bus | Pins |
| --- | --- | --- | --- |
| Wemos D1 mini (ESP8266) | | | |
| Plantower PMS5003 | PM1.0 / PM2.5 / PM10, particle counts | UART 9600 | D5 (RX), D6 (TX) |
| SenseAir S8 | CO2 | Modbus RTU 9600 | D4 (RX), D3 (TX) |
| Sensirion SHT3x | temperature, humidity | I2C `0x44` | D1 (SCL), D2 (SDA) |
| Sensirion SGP30 | TVOC, eCO2 | I2C `0x58` | D1 (SCL), D2 (SDA) |
| Bosch BMP180 (GY-68) | barometric pressure | I2C `0x77` | D1 (SCL), D2 (SDA) |
| SH1106 OLED 128x64 | display, full kit | I2C `0x3C` | D1 (SCL), D2 (SDA) |
| SSD1306 OLED 64x48 | display, smaller kit | I2C `0x3C` | D1 (SCL), D2 (SDA) |
| Wemos battery shield | LiPo charging, 5 V boost, optional | analog | A0 through the shield jumper |

The pin map follows the AirGradient DIY PRO wiring and lives in [include/config.h](include/config.h).

### Two boards, one firmware

The same image runs on the full board and on the smaller one, which carries only the SenseAir S8, the
PMS5003, an SHT3x and a 64x48 display. Nothing has to be declared: the SGP30 and the BMP180 fail their I2C
probe, their metrics are omitted and their cards disappear from the web UI. Both boards wire the two serial
sensors to the same pins.

The panel is the only thing that cannot be detected, so it is a setting: Display in the web UI, or
`displayType` in `/config.json`, set to `sh1106_128x64` or `ssd1306_64x48`.

The 64x48 layout shows CO2, PM2.5, temperature and humidity. It has no room for TVOC or pressure, so a full
sensor board paired with the small panel only reports those two through the web UI and the metrics.

## Getting started

### Requirements

- [PlatformIO](https://platformio.org/install), either the VS Code extension or the `pio` command line.
  Dependencies are fetched on the first build.
- A USB cable for the first flash. Every later update can go over the network.

### Build and flash

```sh
# First flash, over USB
pio run -t upload

# Serial monitor
pio device monitor
```

### First boot

With no WiFi credentials stored, the device starts an access point named `air-quality-setup-<chipid>` with
the password `airquality`, and shows it on the display. Joining it should open the page on its own: the
device answers every DNS query with its own address and redirects any unknown path to the readings page,
which is what a phone's captive portal check follows. Failing that, `http://192.168.4.1/` works.

Open Settings, press **Scan** to list the networks in range, pick yours, fill in the password and save: the
device reboots and joins the network. The station keeps being retried in the background while the setup
access point is up, so a network that simply took too long to answer is picked up without another reboot.

Afterwards the UI is reachable at `http://<ip>/` or `http://air-quality-<chipid>.local/`.

> **Change the admin password.** It starts at `airquality`, which is in this repository and therefore
> public. Both pages of the UI show a banner until it is changed.

### Updates over the network

```sh
pio run -e d1_mini_ota -t upload
```

The `[env:d1_mini_ota]` section of [platformio.ini](platformio.ini) points at the compiled-in defaults, so it
works on a device that has never been configured. Once the admin password has been changed, override them
from the environment rather than committing the real one:

```sh
PLATFORMIO_UPLOAD_PORT=sensor.local \
PLATFORMIO_UPLOAD_FLAGS=--auth=the-new-password \
  pio run -e d1_mini_ota -t upload
```

A firmware image can also be uploaded from a browser on `/update`.

## Prometheus

```yaml
scrape_configs:
  - job_name: air_quality
    metrics_path: /metrics
    static_configs:
      - targets: ['air-quality-<chipid>.local:80']
```

Every sample carries an `id` label, the device id set in the web UI, and a `mac` label.

| Name | Description |
| --- | --- |
| `air_quality_co2` | CO2, in ppm |
| `air_quality_tvoc` | TVOC, in ppb |
| `air_quality_eco2` | Equivalent CO2 estimated by the SGP30, in ppm |
| `air_quality_pm1` | PM1.0, in ug/m3 |
| `air_quality_pm25` | PM2.5, in ug/m3 |
| `air_quality_pm10` | PM10, in ug/m3 |
| `air_quality_aqius` | US Air Quality Index, from PM2.5 |
| `air_quality_particle_count_03` | Particles above 0.3 um per 0.1 L of air |
| `air_quality_particle_count_25` | Particles above 2.5 um per 0.1 L of air |
| `air_quality_temp` | Temperature, in degrees Celsius |
| `air_quality_hum` | Relative humidity, in percent |
| `air_quality_pressure` | Atmospheric pressure measured at the station, in hPa |
| `air_quality_pressure_sea_level` | Atmospheric pressure reduced to sea level, in hPa |
| `air_quality_altitude` | Altitude derived from the standard atmosphere, in meters |
| `air_quality_battery_volts` | Battery cell voltage, in volts |
| `air_quality_battery_percent` | Battery state of charge, in percent |
| `air_quality_wifi_rssi` | WiFi signal strength, in dBm |
| `air_quality_uptime_seconds` | Time since boot, in seconds |
| `air_quality_heap_free_bytes` | Free heap memory, in bytes |
| `air_quality_heap_fragmentation_percent` | Heap fragmentation, in percent |
| `air_quality_sensor_up` | 1 while the sensor has a fresh reading, per `sensor` label |
| `air_quality_sensor_detected` | 1 once the sensor has answered since boot, per `sensor` label |

A metric is omitted while its sensor has no fresh reading, instead of reporting a misleading zero. The
PMS5003 fan is duty cycled to preserve its laser and its fan, so the particulate metrics are refreshed once
per cycle, about every six minutes.

`air_quality_pressure_sea_level` is the figure comparable to weather forecasts. `air_quality_altitude`
drifts with the weather, so it is a rough indication rather than a position.

`air_quality_uptime_seconds` counts the `millis()` rollovers, so it keeps rising past the 49.7 days at which
the raw counter wraps back to zero.

### Alerting on a silent sensor

An omitted metric on its own does not say whether the sensor died or the whole device was unreachable, so
`air_quality_sensor_up` and `air_quality_sensor_detected` carry that separately, one series per sensor under
a `sensor` label taking `co2`, `tvoc`, `particulate`, `climate` or `pressure`. Both are always present, so an
alert reads on the value rather than on `absent()`, and a sensor that was never wired is told apart from one
that stopped answering:

```yaml
groups:
  - name: air-quality
    rules:
      - alert: AirQualitySensorSilent
        expr: air_quality_sensor_up == 0 and air_quality_sensor_detected == 1
        for: 10m
        annotations:
          summary: '{{ $labels.sensor }} on {{ $labels.id }} stopped answering'
```

## Grafana dashboard

A ready-made dashboard is available in [grafana/dashboard.json](grafana/dashboard.json) (schema v2). Import
it via **Dashboards > New > Import**, then pick your Prometheus data source in the Data source selector at
the top. The Station selector lists every device by its `id` label.

## Configuration

Everything below is editable from the web UI and stored in `/config.json` on the LittleFS partition.

| Setting | Default | Notes |
| --- | --- | --- |
| Device id | `air-quality-<chipid>` | Prometheus `id` label, hostname and mDNS name |
| WiFi network, password | empty | Empty password field keeps the stored one |
| Static IP | off | Address, gateway, subnet and DNS |
| Keep WiFi radio awake | off | Disables modem sleep, steadier scrapes on a weak signal, about 60 mA more |
| HTTP port | `80` | |
| Station altitude | `0` m | Used to reduce pressure to sea level |
| NTP server | `europe.pool.ntp.org` | |
| Timezone | `CET-1CEST,M3.5.0,M10.5.0/3` | POSIX TZ string, Central European Time by default |
| Admin password | `airquality` | Settings, reboot, calibration, ArduinoOTA and `/update` |
| Battery shield | off | Enables the A0 reading and the gauge |
| Battery ADC scale | `4.2` V | Full scale of the divider, `4.5` on a 130k shield, trim against a multimeter |
| Battery full voltage | `4.15` V | What the cell settles at after a full charge, anchors 100 % |
| Display | `sh1106_128x64` | Or `ssd1306_64x48` for the smaller board |
| Brightness | `255` | Panel contrast during the day |
| Night brightness | `30` | `0` switches the panel off |
| Night from, until | `22`, `7` | Equal hours disable the dimming |
| Clock page | on | Alternate the display with date and time |
| Rotate display | off | For a display mounted the other way up |

Saving reboots the device. The configuration is written to a temporary file and renamed over the real one,
so a power cut during a save leaves the previous configuration intact rather than a truncated file. The
SGP30 baseline lives in its own `/sgp30.json` for the same reason: it is refreshed every hour, and that
write has no business touching the WiFi credentials.

**Factory reset**, under Maintenance, clears all of the above, the WiFi credentials and the admin password
included, and drops the stored baseline. The device comes back up on its setup access point.

### Night dimming

The panel contrast drops inside a configurable window, 22:00 to 07:00 by default. A night brightness of 0
switches the panel off entirely rather than drawing a black page, which an OLED still lights faintly and
still ages. The layout is static and shown around the clock, so a dimmer night also buys screen life.

The window is half open, `[from, until)`: with 22 and 7, hour 22 is dim and hour 7 is bright again. Until
SNTP has answered the hour is unknown, and the display stays at full brightness rather than risk blanking
itself for good.

## Web UI and HTTP API

| Method | Path | Purpose | Auth |
| --- | --- | --- | --- |
| GET | `/` | Readings and device information | |
| GET | `/settings` | Configuration page | yes |
| GET | `/metrics` | Prometheus exposition | |
| GET | `/api/status` | Readings and device state, JSON | |
| GET | `/api/history` | Two hour trend window, one array per reading, JSON | |
| GET | `/logs` | Serial output followed live | yes |
| GET | `/api/logs?since=N` | Serial output from byte offset N, plain text | yes |
| GET | `/api/config` | Current configuration, secrets redacted | yes |
| POST | `/api/config` | Update configuration, then reboot | yes |
| GET | `/api/scan` | Networks in range | yes |
| POST | `/api/reboot` | Reboot | yes |
| POST | `/api/factory-reset` | Clear every setting, then reboot | yes |
| POST | `/api/calibrate-co2` | Start SenseAir S8 background calibration | yes |
| GET, POST | `/update` | Firmware upload form | yes |

`/api/scan` is asynchronous: the first call starts the scan and answers `{"scanning":true}`, a later call
returns the results. A blocking scan would hold the radio for a couple of seconds with nothing else in
`loop()` running.

`/api/history` is kept in RAM only, one sample a minute over a hundred and twenty slots, and starts empty
after every boot; Prometheus is what keeps the real history.

`/api/logs` returns the last 2 KiB of what the firmware prints on the serial port, also kept in RAM only.
Offsets count bytes since boot: the `X-Log-Next` header is the value to pass as `since` on the next call,
and `X-Log-Start` says where the answer really begins, which differs from `since` when older bytes were
overwritten or the device rebooted in between.

### Authentication

Protected routes ask for HTTP basic authentication with the user `admin` and the admin password. The same
password is what ArduinoOTA uses, so there is a single secret to manage. Clearing it in the settings leaves
everything open.

The readings page, `/metrics`, `/api/status` and `/api/history` stay open: a scraper cannot be asked for
credentials, and the readings are what the device exists to publish. Protecting the settings page itself,
rather than only its API, means the browser raises its own password dialog on a normal navigation and then
carries the credentials on the calls that page makes, which a `fetch` never does.

> **Security:** basic authentication over plain HTTP sends the password base64 encoded, which is not
> encryption. It keeps a household from changing the settings by accident; it does not stand up to anyone
> capturing traffic on the link, and TLS on an ESP8266 web server is not a practical alternative. The WiFi
> and admin passwords are stored in clear text in `/config.json` (never sent back by `/api/config`), and
> anyone holding the board can read the flash. Treat the device as trusted only as far as the network it
> sits on.

## Battery

The 128x64 board can run from a Wemos D1 mini battery shield. Enable it under Battery in the web UI; it
stays off by default so the other board is unaffected.

### Voltage reading

Voltage is read on A0: the resistor of the shield sits in series with the 220k/100k divider of the board, so
the 1 V full scale of the converter lands at `1 V x (Rshield + 320k) / 100k` on the cell. A standard 130 kOhm
shield lands at 4.5 V; the 100 kOhm one on my board puts it at 4.2 V, the default. That is the
`batteryVoltageScale` setting, to trim against a multimeter if the resistor tolerances shift the reading.

The state of charge curve in [src/battery_curve.cpp](src/battery_curve.cpp) reports 0 % at 3.00 V. The last
stretch down to the cut-off of a typical cell is left unreported on purpose: the boost converter of the
shield gives up around there, and a LiPo taken to its cut-off under load ages faster. A percentage under
load reads low: a LiPo sags by 50 to 100 mV at this current, and the curve is steep in that region.

The 100 % end is anchored on `batteryFullVolts`, 4.15 V by default, rather than on the 4.20 V a charger
terminates at. A cell relaxes below its termination voltage once the current stops, and the fan of the
PMS5003 pulls it down further, so anchoring at 4.20 V leaves a full battery reading 97 %. Set it to the
voltage the status page shows just after a full charge. Only the segment above 90 % moves with it.

### Autonomy and peak current

Expect around 2 hours from a 610 mAh cell. The load is close to 173 mA on the 5 V rail, of which 48 mA is
the SGP30 alone: its hotplate runs continuously and its baseline algorithm needs an uninterrupted 1 Hz
measurement, so it cannot be duty cycled without losing its calibration.

Peaks matter more than the average: the SenseAir S8 draws up to 300 mA during a measurement and the radio as
much while transmitting, about 1.17 A from the cell through the boost converter. On a 610 mAh pack rated 2 C
that leaves no margin, and a cell of a lower grade, or one that has aged, browns the board out and reboots
it. When the board reboots on a peak rather than on a flat battery, the protection board of the cell
latching off on over current is as likely a cause as the boost converter sagging. Pick a cell whose
discharge rating covers that peak.

## Development

### Project structure

```
platformio.ini              Build environments (serial upload, OTA upload, host tests)
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
  aqi.h                     US EPA index, no hardware dependency
  baseline_store.h          SGP30 baseline, persisted apart from the configuration
  battery.h                 Cell voltage on A0
  battery_curve.h           LiPo state of charge, no hardware dependency
  barometry.h               Pressure conversions, no hardware dependency
  history.h                 Rolling window ring buffer, no hardware dependency
  history_recorder.h        Samples the readings into that window, serves /api/history
  console.h                 Serial output mirrored into a RAM buffer for /logs
  log_ring.h                Console ring buffer, no hardware dependency
  hostname.h                RFC 1123 label sanitising, no hardware dependency
  modbus_crc.h              Modbus CRC16, no hardware dependency
  prom_text.h               Prometheus label escaping, no hardware dependency
  uptime.h                  millis() rollover accounting, no hardware dependency
  sensors/bmp180.h          Barometric pressure, sea level correction
  sensors/pms5003.h         PMS5003 fan duty cycle and serial plumbing
  sensors/pms_frame.h       Plantower frame parser, no hardware dependency
  sensors/senseair_s8.h     Modbus RTU client
  sensors/sgp30.h           TVOC with baseline persistence
  sensors/sht3x.h           Temperature and humidity
  web_page.h                Web UI, embedded in flash
  webui.h                   HTTP routes
src/                        Matching implementations
test/                       Host tests, one suite per pure module
grafana/dashboard.json      Grafana dashboard
```

Each module owns its own state and exposes a small namespace API. `main.cpp` only wires them together.

### Tests

The protocol and conversion logic is kept free of Arduino dependencies so it runs on the host, in about five
seconds and with no board attached:

```sh
pio test -e native
```

The environment matters: `pio test` on its own follows `default_envs`, which is the firmware, where these
suites are ignored and nothing runs.

The suites cover the PMS5003 frame parser (checksum rejection, resynchronisation after garbage, payload
offsets), the Modbus CRC16 against frames from the SenseAir S8 documentation, the EPA breakpoint table, the
barometric conversions, the LiPo discharge curve, the night window with its midnight wrap, the `millis()`
rollover accounting, hostname sanitising against RFC 1123, Prometheus label escaping, the trend ring buffer
and the console log buffer. Expected values come from the datasheets, from the EPA tables or from an
independent computation, never from the implementation under test.

[CI](.github/workflows/ci.yml) runs the suites and a firmware build on every push and pull request. The
drivers, the web server and the display need the hardware and are exercised on the device.

### Deliberately not implemented

**Deep sleep.** The battery shield makes it tempting, but the device exists to answer a scrape and serve a
page, and both need it awake. Waking on a timer also means wiring D0 to RST, which not every build has.
Autonomy on the shield is a matter of hours, and that is the trade.

## Upgrading from the single-file firmware

- The web UI now answers on `/`, so a scrape configuration pointing at `/` must be updated to `/metrics`.
- The default HTTP port is 80 instead of 9926.
- Every metric is prefixed with `air_quality_`, so short names such as `temp` or `pressure` cannot collide
  with another exporter scraped by the same Prometheus. Dashboards and alerting rules written against `tvoc`,
  `co2`, `pm25`, `aqius`, `temp` or `hum` need renaming to `air_quality_tvoc` and so on. The `id` and `mac`
  labels are unchanged.
- The AQI follows the EPA table revised in February 2024, so the same concentration reads higher than before.

## Contributing

Bug reports and pull requests are welcome.

1. Fork: [github.com/d1ceward/Air-Quality-Sensor](https://github.com/d1ceward/Air-Quality-Sensor/fork)
2. Create a branch: `git checkout -b my-new-feature`
3. Commit: `git commit -am 'Add some feature'`
4. Push: `git push origin my-new-feature`
5. Open a Pull Request

## License

[MIT](LICENSE)
