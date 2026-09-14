# Air Quality Sensor

Firmware for an ESP8266 based air quality station (AirGradient DIY PRO hardware),
exposing its readings as Prometheus metrics.

Rewritten from the original single-file Arduino sketch
([docs/original_arduino_software.c](docs/original_arduino_software.c)) as a modular
PlatformIO project, with no dependency on the AirGradient library: every sensor
protocol is implemented locally.

## Features

- Prometheus metrics on `/metrics`, every name under an `air_quality_` prefix
- Web UI with live readings, a two hour trend per reading, device information and
  full configuration
- OTA updates, both from PlatformIO over the network and from a browser
- WiFi provisioning through a captive portal access point, with a network scan,
  and no credentials in the source
- Configuration stored as JSON on LittleFS, written atomically, editable without
  reflashing
- NTP time with POSIX timezone rules, so DST is handled automatically
- SGP30 baseline persistence and humidity compensation
- Barometric pressure reduced to sea level from a configurable station altitude
- SenseAir S8 readings validated with a Modbus CRC, manual calibration from the UI
- Sensors probed again while they stay silent, so a wedged bus does not cost a
  reboot

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

Voltage is read on A0: the resistor of the shield sits in series with the
220k/100k divider of the board, so the 1 V full scale of the converter lands at
`1 V x (Rshield + 320k) / 100k` on the cell. The shield fitted here carries
100 kOhm, which puts it at 4.2 V; a standard 130 kOhm shield lands at 4.5 V.
That is the `batteryVoltageScale` setting, to trim against a multimeter if the
resistor tolerances shift the reading. At 4.2 V full scale a freshly charged
cell sits at the very top of the range, so it clips to 100 % rather than
reading over.

The cell fitted here is a single LiPo of 610 mAh, 3.7 V nominal, charged to
4.20 V and cut off at 2.75 V. It takes 610 mA (1 C) of charge current and
1220 mA (2 C) of discharge current at most, 122 mA (0.2 C) being its standard
rate for both.

Expect around 2 hours from that cell. The load is close to 173 mA on the 5 V
rail, of which 48 mA is the SGP30 alone: its hotplate runs continuously and its
baseline algorithm needs an uninterrupted 1 Hz measurement, so it cannot be duty
cycled without losing its calibration. Peaks matter more than the average here:
the SenseAir S8 draws up to 300 mA during a measurement and the radio as much
while transmitting, which is about 1.17 A from the cell through the boost, or
1.9 C on this pack. That sits just under its 2 C limit, leaving no margin: a
cell of a different grade, or one that has aged, browns the board out and
reboots it.

The cell carries a PCM board against overvoltage, deep discharge, over current
and short circuit (EEMB, UL file MH20555, UN38.3 for transport). Its over
current trip is the other thing to watch at a 1.17 A peak: when the board
reboots on a peak rather than on a flat battery, the PCM latching off is as
likely a cause as the boost converter sagging. Off the board, keep the cell
between 40 and 60 % for storage, under 25 degrees, and top it up every three
months to hold it between 3.7 and 4.0 V.

The state of charge curve in [src/battery_curve.cpp](src/battery_curve.cpp)
reports 0 % at 3.00 V, above the 2.75 V cut-off of the cell. The last stretch is
left unreported on purpose: the boost converter of the shield gives up around
there, and a LiPo taken to its cut-off under load ages faster.

A percentage under load reads low: a LiPo sags by 50 to 100 mV at this current,
and the curve is steep in that region.

The 100 % end of the curve is anchored on `batteryFullVolts`, 4.15 V by default,
rather than on the 4.20 V a charger terminates at. A cell relaxes below its
termination voltage once the current stops, and the fan of the PMS5003 pulls it
down further, so anchoring at 4.20 V leaves a full battery reading 97 %. Set it
from the settings page to the voltage the status page shows just after a full
charge. Only the segment above 90 % moves with it; the rest of the curve and the
cut-off are properties of the chemistry and stay where they are.

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
  baseline_store.h          SGP30 baseline, persisted apart from the configuration
  battery.h                 Cell voltage on A0
  battery_curve.h           LiPo state of charge, no hardware dependency
  barometry.h               Pressure conversions, no hardware dependency
  history.h                 Rolling window ring buffer, no hardware dependency
  history_recorder.h        Samples the readings into that window, serves /api/history
  hostname.h                RFC 1123 label sanitising, no hardware dependency
  modbus_crc.h              Modbus CRC16, no hardware dependency
  prom_text.h               Prometheus label escaping, no hardware dependency
  uptime.h                  millis() rollover accounting, no hardware dependency
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
run on the host, in about five seconds and with no board attached:

```sh
pio test -e native
```

The environment matters: `pio test` on its own follows `default_envs`, which is
the firmware, where these suites are ignored and nothing runs.

Seventy five cases cover the PMS5003 frame parser (checksum rejection, resynchro-
nisation after garbage, payload offsets), the Modbus CRC16 against frames from
the SenseAir S8 documentation, the EPA breakpoint table, the barometric
conversions, the LiPo discharge curve, the night window with its midnight wrap,
the `millis()` rollover accounting, hostname sanitising against RFC 1123,
Prometheus label escaping and the trend ring buffer. Expected values come from
the datasheets, from the EPA tables or from an independent computation, never
from the implementation under test.

Both the suites and a firmware build run on every push, see
[.github/workflows/ci.yml](.github/workflows/ci.yml).

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

The `[env:d1_mini_ota]` section of [platformio.ini](platformio.ini) points at the
compiled-in defaults, so it works on a device that has never been configured.
Once the admin password has been changed, override them from the environment
rather than committing the real one:

```sh
PLATFORMIO_UPLOAD_PORT=sensor.local \
PLATFORMIO_UPLOAD_FLAGS=--auth=the-new-password \
  pio run -e d1_mini_ota -t upload
```

## First boot

With no WiFi credentials stored, the device starts an access point named
`air-quality-setup-<chipid>` with the password `airquality`, and shows it on the
display. Joining it should open the page on its own: the device answers every DNS
query with its own address and redirects any unknown path to the readings page,
which is what a phone's captive portal check follows. Failing that,
`http://192.168.4.1/` works.

Open Settings, press **Scan** to list the networks in range, pick yours, fill in
the password and save: the device reboots and joins the network. The station
keeps being retried in the background while the setup access point is up, so a
network that simply took too long to answer is picked up without another reboot.

Afterwards the UI is reachable at `http://<ip>/` or
`http://air-quality-<chipid>.local/`.

The admin password starts at `airquality`, which is in this repository and
therefore public. Both pages of the UI show a banner until it is changed.

## HTTP endpoints

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/` | Readings and device information, open |
| GET | `/settings` | Configuration page, password protected |
| GET | `/metrics` | Prometheus exposition |
| GET | `/api/status` | Readings and device state, JSON |
| GET | `/api/history` | Two hour trend window, one array per reading, JSON |
| GET | `/api/config` | Current configuration, secrets redacted, password protected |
| POST | `/api/config` | Update configuration, then reboot, password protected |
| GET | `/api/scan` | Networks in range, password protected |
| POST | `/api/reboot` | Reboot, password protected |
| POST | `/api/factory-reset` | Clear every setting, then reboot, password protected |
| POST | `/api/calibrate-co2` | Start SenseAir S8 background calibration, password protected |
| GET, POST | `/update` | Firmware upload form |

`/api/scan` is asynchronous: the first call starts the scan and answers
`{"scanning":true}`, a later call returns the results. A blocking scan would
hold the radio for a couple of seconds with nothing else in `loop()` running.

`/api/history` serves the window the UI draws its sparklines from. It is kept in
RAM only, one sample a minute over a hundred and twenty slots, and starts empty
after every boot; Prometheus is what keeps the real history.

### Authentication

The UI is two pages. `/` shows the readings and asks for nothing. `/settings`
carries the configuration form and the maintenance actions, and is password
protected, as are the routes it calls.

Protecting the settings page itself rather than only its API means the browser
raises its own password dialog on a normal navigation, then carries the
credentials on the calls that page makes. A `fetch` never raises that dialog,
which is why the settings live behind their own URL.

The protected routes, `/settings`, `/api/config` both ways, `/api/scan`,
`/api/reboot`, `/api/factory-reset`, `/api/calibrate-co2` and `/update`, ask for
HTTP basic authentication with the user `admin` and the admin password. The same password is what ArduinoOTA uses,
so there is a single secret to manage. Clearing it in the settings leaves
everything open.

`/`, `/metrics`, `/api/status` and `/api/history` stay open: a scraper cannot be
asked for credentials, and the readings are what the device exists to publish.
Nothing on the readings page calls an authenticated route.

Basic authentication over plain HTTP means the password crosses the network
base64 encoded, which is not encryption. It keeps a household from changing the
settings by accident; it does not stand up to anyone capturing traffic on the
link. TLS on an ESP8266 web server is not a practical alternative.

The WiFi password and the admin password are stored in clear text in
`/config.json`, and `/api/config` never sends any of them back. There is
no key storage on this part to do better with: anyone holding the board can read
the flash. Treat the device as trusted only as far as the network it sits on.

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

Every metric is prefixed with `air_quality_`, so short names such as `temp` or
`pressure` cannot collide with another exporter scraped by the same Prometheus.
The original firmware exported them unprefixed: dashboards and alerting rules
written against `tvoc`, `co2`, `pm25`, `aqius`, `temp` or `hum` need renaming to
`air_quality_tvoc` and so on. The `id` and `mac` labels are unchanged.

Exported: `air_quality_tvoc`, `_eco2`, `_co2`, `_pm25`, `_pm1`, `_pm10`,
`_aqius`, `_particle_count_03`, `_particle_count_25`, `_temp`, `_hum`,
`_pressure`, `_pressure_sea_level`, `_altitude`, `_battery_volts`,
`_battery_percent`, `_wifi_rssi`, `_uptime_seconds`, `_heap_free_bytes`,
`_heap_fragmentation_percent`, `_sensor_up` and `_sensor_detected`.

Pressure is exported in hPa. `air_quality_pressure` is what the sensor measures,
`air_quality_pressure_sea_level` is that value reduced to sea level using the
configured station altitude, which is the figure comparable to weather
forecasts. `air_quality_altitude` is derived from the standard atmosphere
(1013.25 hPa) and drifts with the weather, so it is a rough indication rather
than a position.

A metric is omitted while its sensor has no fresh reading, instead of reporting
a misleading zero. The PMS5003 fan is duty cycled to preserve its laser and its
fan, so `air_quality_pm25` is refreshed once per cycle, about every six
minutes.

An omitted metric on its own does not say whether the sensor died or the whole
device was unreachable, so two families carry that separately, one series per
sensor under a `sensor` label taking `co2`, `tvoc`, `particulate`, `climate` or
`pressure`:

- `air_quality_sensor_up` is 1 while the sensor has a fresh reading
- `air_quality_sensor_detected` is 1 once it has answered since boot

Both are always present, so an alert reads on the value rather than on
`absent()`, and a sensor that was never wired is told apart from one that
stopped answering:

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

`air_quality_uptime_seconds` counts the `millis()` rollovers, so it keeps rising
past the 49.7 days at which the raw counter wraps back to zero. Firmware 1.1.0
and earlier reported that wrap as a reboot.

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
| Timezone | `CET-1CEST,M3.5.0,M10.5.0/3` | POSIX TZ string, French summer time by default |
| Admin password | `airquality` | Settings, reboot, calibration, ArduinoOTA and `/update` |
| Battery shield | off | Enables the A0 reading and the gauge |
| Battery ADC scale | `4.2` V | Full scale of the divider, `4.5` on a 130k shield, trim against a multimeter |
| Display | `sh1106_128x64` | Or `ssd1306_64x48` for the smaller board |
| Brightness | `255` | Panel contrast during the day |
| Night brightness | `30` | `0` switches the panel off |
| Night from, until | `22`, `7` | Equal hours disable the dimming |
| Clock page | on | Alternate the display with date and time |
| Rotate display | off | For a display mounted the other way up |

Saving reboots the device. The configuration is written to a temporary file and
renamed over the real one, so a power cut during a save leaves the previous
configuration intact rather than a truncated file. The SGP30 baseline lives in
its own `/sgp30.json` for the same reason: it is refreshed every hour, and that
write has no business touching the WiFi credentials. A configuration written by
1.1.0 or earlier is migrated on the first boot.

**Factory reset**, under Maintenance, clears all of the above, the WiFi
credentials and the admin password included, and drops the stored baseline. The
device comes back up on its setup access point.

## Deliberately not implemented

**Deep sleep.** The battery shield makes it tempting, but the device exists to
answer a scrape and serve a page, and both need it awake. Waking on a timer also
means wiring D0 to RST on the board, which not every build has. Autonomy on the
shield is a matter of hours, and that is the trade.
