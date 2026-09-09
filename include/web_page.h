#pragma once

#include <Arduino.h>

// Web UI, served from flash and split in two pages: the readings, open to anyone, and the settings, behind
// the admin password. Both share this head so the stylesheet is stored once.
//
// Kept self-contained, with no CDN, so the interface also works from the setup access point, before the
// device has any internet access.
const char WEB_HEAD[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
:root {
  --bg: #0f1419; --panel: #171d24; --line: #262e38; --text: #e6edf3;
  --muted: #8b98a5; --accent: #4c9aff;
  --good: #3fb950; --fair: #d29922; --poor: #f0883e; --bad: #f85149;
}
@media (prefers-color-scheme: light) {
  :root {
    --bg: #f4f6f8; --panel: #ffffff; --line: #dfe3e8; --text: #1c2128;
    --muted: #62707f; --accent: #0969da;
  }
}
* { box-sizing: border-box; }
[hidden] { display: none !important; }
body {
  margin: 0; padding: 24px 16px 48px; background: var(--bg); color: var(--text);
  font: 15px/1.5 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
}
.wrap { max-width: 940px; margin: 0 auto; }
header { display: flex; align-items: baseline; gap: 12px; flex-wrap: wrap; margin-bottom: 4px; }
h1 { font-size: 20px; margin: 0; font-weight: 600; }
.pill {
  font-size: 12px; padding: 2px 9px; border-radius: 999px;
  border: 1px solid var(--line); color: var(--muted);
}
.pill.on { color: var(--good); border-color: var(--good); }
.pill.off { color: var(--bad); border-color: var(--bad); }
.sub { color: var(--muted); font-size: 13px; margin: 0 0 20px; }
.grid { display: grid; gap: 12px; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); }
.card {
  background: var(--panel); border: 1px solid var(--line); border-radius: 10px;
  padding: 14px 16px 16px;
}
.card .head { display: flex; align-items: center; justify-content: space-between; gap: 8px; }
.card .label {
  font-size: 12px; letter-spacing: .06em; text-transform: uppercase; color: var(--muted);
}
.help-toggle {
  flex: none; width: 18px; height: 18px; padding: 0; border-radius: 999px;
  border: 1px solid var(--line); background: transparent; color: var(--muted);
  font: inherit; font-size: 11px; line-height: 1; cursor: pointer;
}
.help-toggle:hover, .help-toggle[aria-expanded=true] { color: var(--accent); border-color: var(--accent); }
.help {
  font-size: 12px; color: var(--muted); margin-top: 12px; padding-top: 10px;
  border-top: 1px solid var(--line);
}
.help p { margin: 0 0 6px; }
.help p:last-child { margin: 0; }
.card .value { font-size: 34px; font-weight: 650; line-height: 1.1; margin-top: 6px; font-variant-numeric: tabular-nums; }
.card .unit { font-size: 14px; font-weight: 400; color: var(--muted); margin-left: 4px; }
.card .note { font-size: 12px; color: var(--muted); margin-top: 6px; min-height: 18px; }
.spark { display: block; width: 100%; height: 26px; margin-top: 10px; overflow: visible; }
.spark polyline { fill: none; stroke: currentColor; stroke-width: 1.5; stroke-linejoin: round;
  stroke-linecap: round; vector-effect: non-scaling-stroke; opacity: .75; }
.spark.empty { visibility: hidden; }
.bar { height: 4px; border-radius: 2px; background: var(--line); margin-top: 12px; overflow: hidden; }
.bar > i { display: block; height: 100%; width: 0; border-radius: 2px; transition: width .4s ease; }
.good { color: var(--good); } .fair { color: var(--fair); }
.poor { color: var(--poor); } .bad { color: var(--bad); }
.bg-good { background: var(--good); } .bg-fair { background: var(--fair); }
.bg-poor { background: var(--poor); } .bg-bad { background: var(--bad); }
section { margin-top: 28px; }
h2 { font-size: 15px; font-weight: 600; margin: 0 0 12px; }
.panel { background: var(--panel); border: 1px solid var(--line); border-radius: 10px; padding: 16px; }
.rows { display: grid; gap: 10px; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); }
label { display: block; font-size: 12px; color: var(--muted); margin-bottom: 4px; }
input[type=text], input[type=password], input[type=number], select {
  width: 100%; padding: 7px 9px; border-radius: 6px; border: 1px solid var(--line);
  background: var(--bg); color: var(--text); font: inherit; font-size: 14px;
}
input[type=checkbox] { margin-right: 6px; }
.check { display: flex; align-items: center; font-size: 14px; margin-top: 22px; }
.actions { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 16px; }
button, .btn {
  border: 1px solid var(--line); background: var(--panel); color: var(--text);
  border-radius: 6px; padding: 8px 14px; font: inherit; font-size: 14px;
  cursor: pointer; text-decoration: none; display: inline-block;
}
button.primary { background: var(--accent); border-color: var(--accent); color: #fff; }
button:hover, .btn:hover { border-color: var(--accent); }
.info { display: grid; gap: 6px 20px; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); font-size: 13px; }
.info span { color: var(--muted); }
.toast {
  position: fixed; left: 50%; bottom: 24px; transform: translateX(-50%);
  background: var(--panel); border: 1px solid var(--line); border-radius: 8px;
  padding: 10px 16px; font-size: 14px; opacity: 0; pointer-events: none; transition: opacity .2s;
}
.toast.show { opacity: 1; }
.banner {
  border: 1px solid var(--fair); border-left-width: 3px; border-radius: 8px;
  background: var(--panel); padding: 12px 14px; margin: 0 0 20px; font-size: 13px;
}
.banner.severe { border-color: var(--bad); }
.banner strong { font-weight: 600; }
.banner a { color: var(--accent); }
.banner p { margin: 0; }
.banner p + p { margin-top: 6px; }
.hint { font-size: 12px; color: var(--muted); margin: 4px 0 0; }
.field-row { display: flex; gap: 8px; align-items: flex-end; }
.field-row > div { flex: 1; min-width: 0; }
.field-row button { flex: none; }
</style>
)rawliteral";

// Readings and device information. No call on this page is authenticated.
const char WEB_STATUS[] PROGMEM = R"rawliteral(<title>Air Quality Sensor</title>
</head>
<body>
<div class="wrap">
  <header>
    <h1 id="device">Air Quality Sensor</h1>
    <span class="pill" id="link">connecting</span>
  </header>
  <p class="sub" id="subtitle">&nbsp;</p>

  <div id="banners"></div>

  <div class="grid" id="cards"></div>

  <section>
    <h2>Device</h2>
    <div class="panel info" id="info"></div>
    <div class="actions">
      <a class="btn" href="/metrics">Prometheus metrics</a>
      <a class="btn" href="/api/history">Trend data</a>
      <a class="btn" href="/settings">Settings</a>
    </div>
  </section>
</div>
<div class="toast" id="toast"></div>

<script>
const CARDS = [
  { key: 'co2', label: 'CO2', unit: 'ppm', max: 2500, steps: [800, 1200, 2000],
    value: r => r.co2.value, valid: r => r.co2.valid,
    help: [
      'Carbon dioxide concentration, an indicator of how effectively the room is ventilated. Levels rise as occupants breathe and fall once the air is renewed.',
      'Outdoor air is typically around 420 ppm. Above 1200 ppm, ventilation is advisable; above 2000 ppm, comfort and concentration are measurably affected.',
    ] },
  { key: 'pm25', label: 'PM2.5', unit: 'ug/m3', max: 150, steps: [9, 35, 55],
    value: r => r.pm25.value, valid: r => r.pm25.valid,
    note: r => 'AQI US ' + r.pm25.aqius + (r.pm25.fanRunning ? ' - fan running' : ' - fan asleep'),
    help: [
      'Concentration of fine particles small enough to reach the lower airways. Common indoor sources are cooking, combustion, candles and outdoor pollution entering the room.',
      'A measurement cycle runs every 5 minutes, so the displayed value reflects the most recent cycle rather than the present instant.',
      'AQI US expresses the same concentration on the standard 0 to 500 air quality index: below 50 is good, above 100 is considered unhealthy. The index uses the breakpoints the EPA revised in 2024, so a given concentration reads slightly higher than it did under the previous table.',
    ] },
  { key: 'tvoc', label: 'TVOC', unit: 'ppb', max: 2200, steps: [220, 660, 1430],
    value: r => r.tvoc.value, valid: r => r.tvoc.valid,
    note: r => 'eCO2 ' + r.tvoc.eco2 + ' ppm',
    help: [
      'Total volatile organic compounds: a combined indicator of airborne chemical pollutants such as solvents, cleaning agents, paints, cooking vapours and emissions from new furnishings.',
      'The reading tracks variation against the ambient baseline rather than an absolute concentration. Brief peaks after cooking or cleaning are expected and usually clear within the hour; sustained high values call for ventilation.',
      'eCO2 is an estimated CO2 equivalent derived from the same chemical signal. It is an approximation given for reference; the CO2 reading is the measured value.',
    ] },
  { key: 'pressure', label: 'Pressure', unit: 'hPa', min: 950, max: 1050, steps: [null],
    value: r => r.pressure.value.toFixed(1), valid: r => r.pressure.valid,
    note: r => 'sea level ' + r.pressure.seaLevel.toFixed(1) + ' hPa',
    help: [
      'Atmospheric pressure at the device. The trend carries more information than the absolute value: a sustained fall generally precedes unsettled weather, a rise more stable conditions.',
      'The sea level figure is the same measurement corrected for the configured station altitude, which allows direct comparison with published forecasts.',
    ] },
  { key: 'battery', label: 'Battery', unit: '%', max: 100, steps: [null],
    value: r => r.battery.percent, valid: r => r.battery.valid,
    show: r => r.battery.enabled,
    note: r => r.battery.volts.toFixed(2) + ' V',
    help: [
      'Remaining charge of the battery pack. The card is shown only when a battery is fitted.',
      'The percentage is derived from the pack voltage. It can hold steady for long periods and then drop quickly towards the end of the discharge, which is the normal behaviour of a lithium cell.',
    ] },
  { key: 'temperature', label: 'Temperature', unit: '°C', max: 40, steps: [null],
    value: r => r.climate.temperature.toFixed(1), valid: r => r.climate.valid,
    help: [
      'Ambient air temperature.',
      'The measurement is taken inside the enclosure, so the reading can sit marginally above the true room temperature.',
    ] },
  { key: 'humidity', label: 'Humidity', unit: '%', max: 100, steps: [null],
    value: r => r.climate.humidity.toFixed(0), valid: r => r.climate.valid,
    help: [
      'Relative humidity. A range of 40 to 60 percent is generally considered comfortable: drier air causes irritation and static, damper air favours condensation and mould.',
      'The value is expressed relative to the maximum the air can hold at its current temperature, so it varies as the room warms or cools even when the moisture content is unchanged.',
    ] },
];

const grade = (value, steps) => {
  if (steps[0] === null) return '';
  if (value < steps[0]) return 'good';
  if (value < steps[1]) return 'fair';
  if (value < steps[2]) return 'poor';
  return 'bad';
};

const el = id => document.getElementById(id);

function toast(message) {
  const node = el('toast');
  node.textContent = message;
  node.classList.add('show');
  setTimeout(() => node.classList.remove('show'), 2600);
}

function buildCards() {
  el('cards').innerHTML = CARDS.map(card => `
    <div class="card" id="card-${card.key}">
      <div class="head">
        <div class="label">${card.label}</div>
        <button class="help-toggle" type="button" data-help="${card.key}" aria-expanded="false"
                aria-controls="help-${card.key}" title="About ${card.label}">?</button>
      </div>
      <div class="value"><span data-value>--</span><span class="unit">${card.unit}</span></div>
      <div class="note" data-note></div>
      <div class="bar"><i data-bar></i></div>
      <svg class="spark empty" data-spark viewBox="0 0 100 26" preserveAspectRatio="none"
           aria-hidden="true"></svg>
      <div class="help" id="help-${card.key}" hidden>
        ${card.help.map(line => '<p>' + line + '</p>').join('')}
      </div>
    </div>`).join('');

  // One listener on the grid, as the cards are rebuilt only here but toggled often
  el('cards').addEventListener('click', event => {
    const button = event.target.closest('.help-toggle');
    if (!button) return;

    const help = el('help-' + button.dataset.help);
    help.hidden = !help.hidden;
    button.setAttribute('aria-expanded', String(!help.hidden));
  });
}

// The device keeps working with the password that ships with the firmware, but that password is public, so
// the page says so rather than staying quiet about it.
function renderBanners(status) {
  const notices = [];
  const security = status.security || {};

  if (security.adminPasswordEmpty)
    notices.push(['severe', 'No admin password is set. Anyone who can reach this device can change its '
      + 'settings and upload firmware to it. <a href="/settings">Set one now</a>.']);
  else if (security.adminPasswordDefault)
    notices.push(['', 'The admin password is still the one built into the firmware, which is public. '
      + '<a href="/settings">Change it</a>.']);

  // A sensor that never answered is either not fitted or not wired; one that answered and went quiet is a
  // different problem, and worth telling apart.
  const readings = status.readings;
  const silent = [
    ['CO2', readings.co2], ['particulate', readings.pm25], ['TVOC', readings.tvoc],
    ['temperature and humidity', readings.climate], ['pressure', readings.pressure],
  ].filter(([, reading]) => reading.detected && !reading.valid).map(([name]) => name);

  // Held back for the first minute: the particulate sensor takes 40 seconds to stream its first frame and
  // the CO2 sensor is polled every five, so a fresh boot would otherwise flag every one of them
  const missing = status.uptimeSeconds < 60 ? [] : [
    ['CO2', readings.co2], ['particulate', readings.pm25], ['TVOC', readings.tvoc],
    ['temperature and humidity', readings.climate], ['pressure', readings.pressure],
  ].filter(([, reading]) => !reading.detected).map(([name]) => name);

  if (silent.length)
    notices.push(['severe', 'Stopped answering: ' + silent.join(', ') + '.']);

  if (missing.length)
    notices.push(['', 'Never answered since boot: ' + missing.join(', ')
      + '. Check the wiring, the device keeps probing.']);

  el('banners').innerHTML = notices
    .map(([level, text]) => `<div class="banner ${level}"><p>${text}</p></div>`).join('');
}

function renderStatus(status) {
  el('device').textContent = status.deviceId;
  const online = status.network.connected;
  const link = el('link');
  link.textContent = online ? status.network.ssid + ' - ' + status.network.rssi + ' dBm'
                            : (status.network.accessPoint ? 'setup mode' : 'offline');
  link.className = 'pill ' + (online ? 'on' : 'off');
  const clock = status.time ? status.time.replace('T', ' ') + ' ' + (status.timezone || '') : '';
  el('subtitle').textContent = status.network.ip + (clock ? ' - ' + clock.trim() : '');

  for (const card of CARDS) {
    const node = el('card-' + card.key);

    // A card whose hardware is absent is hidden rather than left empty
    node.hidden = card.show ? !card.show(status.readings) : false;
    if (node.hidden) continue;

    const valid = card.valid(status.readings);
    const raw = valid ? card.value(status.readings) : null;
    const numeric = valid ? Number(raw) : 0;
    const level = valid ? grade(numeric, card.steps) : '';

    node.querySelector('[data-value]').textContent = valid ? raw : '--';
    node.querySelector('[data-value]').className = level;
    node.querySelector('[data-note]').textContent = valid && card.note ? card.note(status.readings) : '';
    const bar = node.querySelector('[data-bar]');
    const floor = card.min || 0;
    const ratio = (numeric - floor) / (card.max - floor) * 100;
    bar.style.width = valid ? Math.max(0, Math.min(100, ratio)) + '%' : '0';
    bar.className = level ? 'bg-' + level : 'bg-good';
  }

  const rows = [
    ['Firmware', status.version],
    ['Hostname', status.network.hostname],
    ['IP address', status.network.ip],
    ['MAC address', status.network.mac],
    ['Uptime', formatUptime(status.uptimeSeconds)],
    ['Free heap', (status.freeHeap / 1024).toFixed(1) + ' KiB'
      + (status.heapFragmentation !== undefined ? ' - ' + status.heapFragmentation + '% fragmented' : '')],
  ];

  el('info').innerHTML = rows
    .map(([key, value]) => `<div><span>${key}</span><br>${value}</div>`).join('');

  renderBanners(status);
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor(seconds % 86400 / 3600);
  const minutes = Math.floor(seconds % 3600 / 60);
  return (days ? days + 'd ' : '') + hours + 'h ' + minutes + 'm';
}

// Turns a series into one polyline per run of consecutive readings, so a gap where the sensor had nothing
// to give stays a gap instead of a line drawn straight through it.
function sparkPaths(samples) {
  const values = samples.filter(sample => sample !== null);
  if (values.length < 2) return '';

  let low = Math.min(...values);
  let high = Math.max(...values);

  // A flat series would divide by zero, and it belongs in the middle of the box rather than on its floor
  if (high - low < 1e-9) {
    low -= 1;
    high += 1;
  }

  const x = index => samples.length < 2 ? 0 : index / (samples.length - 1) * 100;
  const y = value => 25 - (value - low) / (high - low) * 24;

  const segments = [];
  let current = [];

  for (let index = 0; index < samples.length; ++index) {
    if (samples[index] === null) {
      if (current.length > 1) segments.push(current);
      current = [];
      continue;
    }

    current.push(x(index).toFixed(2) + ',' + y(samples[index]).toFixed(2));
  }

  if (current.length > 1) segments.push(current);

  return segments.map(points => `<polyline points="${points.join(' ')}"/>`).join('');
}

function renderHistory(history) {
  for (const card of CARDS) {
    const node = el('card-' + card.key);
    const spark = node.querySelector('[data-spark]');
    const paths = sparkPaths(history.series[card.key] || []);

    spark.innerHTML = paths;
    spark.classList.toggle('empty', paths === '');
  }
}

async function refresh() {
  try {
    const response = await fetch('/api/status', { cache: 'no-store' });
    renderStatus(await response.json());
  } catch (error) {
    el('link').className = 'pill off';
    el('link').textContent = 'unreachable';
  }
}

// The window only advances one slot a minute, so it is fetched far less often than the readings and a
// failure is simply ignored: the sparkline is the one part of the page nobody needs an error about.
async function refreshHistory() {
  try {
    const response = await fetch('/api/history', { cache: 'no-store' });
    renderHistory(await response.json());
  } catch (error) {}
}

buildCards();
refresh();
refreshHistory();
setInterval(refresh, 3000);
setInterval(refreshHistory, 60000);
</script>
</body>
</html>
)rawliteral";

// Configuration form and the administrative actions. The page itself is behind the admin password, so the
// browser asks once and then carries the credentials on the calls this script makes.
const char WEB_SETTINGS[] PROGMEM = R"rawliteral(<title>Settings</title>
</head>
<body>
<div class="wrap">
  <header>
    <h1>Settings</h1>
    <a class="pill" href="/">back to readings</a>
  </header>
  <p class="sub">Saving restarts the device.</p>

  <div id="banners"></div>

    <form class="panel" id="settings">
      <div class="rows">
        <div><label for="deviceId">Device id</label><input type="text" id="deviceId" name="deviceId"></div>
        <div class="field-row">
          <div><label for="wifiSsid">WiFi network</label>
            <input type="text" id="wifiSsid" name="wifiSsid" list="networks" autocomplete="off">
            <datalist id="networks"></datalist>
          </div>
          <button type="button" id="scan" title="List the networks in range">Scan</button>
        </div>
        <div><label for="wifiPassword">WiFi password</label><input type="password" id="wifiPassword" name="wifiPassword" placeholder="unchanged"></div>
        <div class="check"><input type="checkbox" id="batteryEnabled" name="batteryEnabled"><label for="batteryEnabled" style="margin:0">Battery shield fitted</label></div>
        <div><label for="batteryVoltageScale">Battery ADC scale (V)</label><input type="number" id="batteryVoltageScale" name="batteryVoltageScale" step="0.01" min="1"></div>
        <div><label for="displayType">Display</label><select id="displayType" name="displayType">
          <option value="sh1106_128x64">SH1106 128x64</option>
          <option value="ssd1306_64x48">SSD1306 64x48</option>
        </select></div>
        <div><label for="httpPort">HTTP port</label><input type="number" id="httpPort" name="httpPort" min="1" max="65535"></div>
        <div><label for="altitudeMeters">Station altitude (m)</label><input type="number" id="altitudeMeters" name="altitudeMeters" step="1"></div>
        <div><label for="ntpServer">NTP server</label><input type="text" id="ntpServer" name="ntpServer"></div>
        <div><label for="timezone">Timezone (POSIX TZ)</label><input type="text" id="timezone" name="timezone" list="timezones">
          <datalist id="timezones">
            <option value="CET-1CEST,M3.5.0,M10.5.0/3">Paris, Berlin, Madrid</option>
            <option value="GMT0BST,M3.5.0/1,M10.5.0">London, Dublin, Lisbon</option>
            <option value="EST5EDT,M3.2.0,M11.1.0">New York, Toronto</option>
            <option value="UTC0">UTC, no daylight saving</option>
          </datalist></div>
        <div><label for="adminPassword">Admin password</label><input type="password" id="adminPassword" name="adminPassword" placeholder="unchanged"></div>
        <div class="check"><input type="checkbox" id="useStaticIp" name="useStaticIp"><label for="useStaticIp" style="margin:0">Static IP address</label></div>
        <div><label for="ip">IP address</label><input type="text" id="ip" name="ip"></div>
        <div><label for="gateway">Gateway</label><input type="text" id="gateway" name="gateway"></div>
        <div><label for="subnet">Subnet mask</label><input type="text" id="subnet" name="subnet"></div>
        <div><label for="dns">DNS server</label><input type="text" id="dns" name="dns"></div>
        <div><label for="displayBrightness">Brightness (0-255)</label><input type="number" id="displayBrightness" name="displayBrightness" min="0" max="255"></div>
        <div><label for="displayNightBrightness">Night brightness (0 = off)</label><input type="number" id="displayNightBrightness" name="displayNightBrightness" min="0" max="255"></div>
        <div><label for="displayNightStartHour">Night from (hour)</label><input type="number" id="displayNightStartHour" name="displayNightStartHour" min="0" max="23"></div>
        <div><label for="displayNightEndHour">Night until (hour)</label><input type="number" id="displayNightEndHour" name="displayNightEndHour" min="0" max="23"></div>
        <div class="check"><input type="checkbox" id="showClockPage" name="showClockPage"><label for="showClockPage" style="margin:0">Show clock page</label></div>
        <div class="check"><input type="checkbox" id="flipDisplay" name="flipDisplay"><label for="flipDisplay" style="margin:0">Rotate display 180&deg;</label></div>
      </div>
      <div class="actions">
        <button type="submit" class="primary">Save and reboot</button>
        <button type="button" id="reload">Discard changes</button>
      </div>
    </form>

  <section>
    <h2>Maintenance</h2>
    <div class="panel">
      <div class="actions">
        <a class="btn" href="/update">Firmware update</a>
        <button id="calibrate">Calibrate CO2</button>
        <button id="reboot">Reboot</button>
        <button id="factory-reset">Factory reset</button>
      </div>
      <p class="hint">A factory reset clears every setting, the WiFi credentials and the admin password
        included, and drops the stored CO2 sensor baseline. The device comes back up in setup mode.</p>
    </div>
  </section>
</div>
<div class="toast" id="toast"></div>

<script>
const el = id => document.getElementById(id);

function toast(message) {
  const node = el('toast');
  node.textContent = message;
  node.classList.add('show');
  setTimeout(() => node.classList.remove('show'), 2600);
}

// The page is already authenticated, so a 401 here means the browser dropped its cached credentials rather
// than a wrong password.
async function send(url, options) {
  const response = await fetch(url, { ...options, cache: 'no-store' });
  if (response.status === 401) toast('Not authorised, reload the page');

  return response;
}

async function loadSettings() {
  const response = await send('/api/config');
  if (!response.ok) return;

  const config = await response.json();
  for (const [key, value] of Object.entries(config)) {
    const field = el(key);
    if (!field) continue;
    if (field.type === 'checkbox') field.checked = Boolean(value);
    else field.value = value;
  }
}

el('settings').addEventListener('submit', async event => {
  event.preventDefault();
  const payload = {};
  for (const field of event.target.elements) {
    if (!field.name) continue;
    if (field.type === 'checkbox') payload[field.name] = field.checked;
    else if (field.value !== '') payload[field.name] = field.type === 'number' ? Number(field.value) : field.value;
  }

  const response = await send('/api/config', {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload),
  });

  if (response.ok) toast('Saved, rebooting...');
  else if (response.status !== 401) toast('Save failed');
});

el('reload').addEventListener('click', () => loadSettings());

// The scan runs asynchronously on the device, so the first answer only says it started
const scanButton = el('scan');

async function pollScan(attemptsLeft) {
  const response = await send('/api/scan');
  if (!response.ok) {
    toast('Scan failed');
    return false;
  }

  const result = await response.json();

  if (result.scanning) {
    if (attemptsLeft === 0) {
      toast('Scan timed out');
      return false;
    }

    await new Promise(resolve => setTimeout(resolve, 700));
    return pollScan(attemptsLeft - 1);
  }

  el('networks').innerHTML = result.networks
    .map(network => `<option value="${network.ssid.replace(/"/g, '&quot;')}">`
      + `${network.rssi} dBm${network.secured ? '' : ', open'}</option>`).join('');

  toast(result.networks.length
    ? result.networks.length + ' network(s) found, open the field to pick one'
    : 'No network found');

  return true;
}

scanButton.addEventListener('click', async () => {
  scanButton.disabled = true;
  scanButton.textContent = 'Scanning';

  try {
    await pollScan(12);
  } finally {
    scanButton.disabled = false;
    scanButton.textContent = 'Scan';
  }
});

el('reboot').addEventListener('click', async () => {
  if (!confirm('Reboot the sensor?')) return;
  await send('/api/reboot', { method: 'POST' });
  toast('Rebooting...');
});

el('calibrate').addEventListener('click', async () => {
  if (!confirm('Start CO2 background calibration? The sensor must have been in fresh outdoor air for several minutes.')) return;
  const response = await send('/api/calibrate-co2', { method: 'POST' });
  if (response.ok) toast('Calibration started');
});

// Two confirmations: this one loses the WiFi credentials, so the device is only reachable through its setup
// access point afterwards
el('factory-reset').addEventListener('click', async () => {
  if (!confirm('Clear every setting? The WiFi credentials and the admin password go with them, and the '
    + 'device will only be reachable through its setup access point.')) return;
  if (!confirm('Last chance. Reset the device to its factory configuration?')) return;

  const response = await send('/api/factory-reset', { method: 'POST' });
  if (response.ok) toast('Resetting, look for the setup access point');
});

// Same warning as the readings page: this one is where it gets acted on
async function loadBanners() {
  try {
    const response = await fetch('/api/status', { cache: 'no-store' });
    const security = (await response.json()).security || {};

    if (security.adminPasswordEmpty)
      el('banners').innerHTML = '<div class="banner severe"><p>No admin password is set, so this page and '
        + 'the firmware upload are open to anyone on the network. Fill in <strong>Admin password</strong> '
        + 'below.</p></div>';
    else if (security.adminPasswordDefault)
      el('banners').innerHTML = '<div class="banner"><p>The admin password is still the one built into the '
        + 'firmware, which is public. Change it in <strong>Admin password</strong> below.</p></div>';
  } catch (error) {}
}

loadSettings();
loadBanners();
</script>
</body>
</html>
)rawliteral";
