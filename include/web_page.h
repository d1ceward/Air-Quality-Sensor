#pragma once

#include <Arduino.h>

// Single page web UI, served from flash. Kept self-contained (no CDN) so the
// interface also works from the setup access point, without internet access.
const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Air Quality Sensor</title>
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
.card .label {
  font-size: 12px; letter-spacing: .06em; text-transform: uppercase; color: var(--muted);
}
.card .value { font-size: 34px; font-weight: 650; line-height: 1.1; margin-top: 6px; font-variant-numeric: tabular-nums; }
.card .unit { font-size: 14px; font-weight: 400; color: var(--muted); margin-left: 4px; }
.card .note { font-size: 12px; color: var(--muted); margin-top: 6px; min-height: 18px; }
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
</style>
</head>
<body>
<div class="wrap">
  <header>
    <h1 id="device">Air Quality Sensor</h1>
    <span class="pill" id="link">connecting</span>
  </header>
  <p class="sub" id="subtitle">&nbsp;</p>

  <div class="grid" id="cards"></div>

  <section>
    <h2>Device</h2>
    <div class="panel info" id="info"></div>
    <div class="actions">
      <a class="btn" href="/metrics">Prometheus metrics</a>
      <a class="btn" href="/update">Firmware update</a>
      <button id="calibrate">Calibrate CO2</button>
      <button id="reboot">Reboot</button>
    </div>
  </section>

  <section>
    <h2>Settings</h2>
    <form class="panel" id="settings">
      <p id="settingsLock" hidden>Settings are password protected.
        <button type="button" id="unlock">Unlock</button></p>
      <div id="settingsFields">
      <div class="rows">
        <div><label for="deviceId">Device id</label><input type="text" id="deviceId" name="deviceId"></div>
        <div><label for="wifiSsid">WiFi network</label><input type="text" id="wifiSsid" name="wifiSsid"></div>
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
        <div><label for="timezone">Timezone (POSIX TZ)</label><input type="text" id="timezone" name="timezone"></div>
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
      </div>
    </form>
  </section>
</div>
<div class="toast" id="toast"></div>

<script>
const CARDS = [
  { key: 'co2', label: 'CO2', unit: 'ppm', max: 2500, steps: [800, 1200, 2000],
    value: r => r.co2.value, valid: r => r.co2.valid },
  { key: 'pm25', label: 'PM2.5', unit: 'ug/m3', max: 150, steps: [12, 35, 55],
    value: r => r.pm25.value, valid: r => r.pm25.valid,
    note: r => 'AQI US ' + r.pm25.aqius + (r.pm25.fanRunning ? ' - fan running' : ' - fan asleep') },
  { key: 'tvoc', label: 'TVOC', unit: 'ppb', max: 2200, steps: [220, 660, 1430],
    value: r => r.tvoc.value, valid: r => r.tvoc.valid,
    note: r => 'eCO2 ' + r.tvoc.eco2 + ' ppm' },
  { key: 'pressure', label: 'Pressure', unit: 'hPa', min: 950, max: 1050, steps: [null],
    value: r => r.pressure.value.toFixed(1), valid: r => r.pressure.valid,
    note: r => 'sea level ' + r.pressure.seaLevel.toFixed(1) + ' hPa' },
  { key: 'battery', label: 'Battery', unit: '%', max: 100, steps: [null],
    value: r => r.battery.percent, valid: r => r.battery.valid,
    show: r => r.battery.enabled,
    note: r => r.battery.volts.toFixed(2) + ' V' },
  { key: 'temperature', label: 'Temperature', unit: '°C', max: 40, steps: [null],
    value: r => r.climate.temperature.toFixed(1), valid: r => r.climate.valid },
  { key: 'humidity', label: 'Humidity', unit: '%', max: 100, steps: [null],
    value: r => r.climate.humidity.toFixed(0), valid: r => r.climate.valid },
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
      <div class="label">${card.label}</div>
      <div class="value"><span data-value>--</span><span class="unit">${card.unit}</span></div>
      <div class="note" data-note></div>
      <div class="bar"><i data-bar></i></div>
    </div>`).join('');
}

function renderStatus(status) {
  el('device').textContent = status.deviceId;
  const online = status.network.connected;
  const link = el('link');
  link.textContent = online ? status.network.ssid + ' - ' + status.network.rssi + ' dBm'
                            : (status.network.accessPoint ? 'setup mode' : 'offline');
  link.className = 'pill ' + (online ? 'on' : 'off');
  el('subtitle').textContent = status.network.ip + (status.time ? ' - ' + status.time.replace('T', ' ') : '');

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

  el('info').innerHTML = [
    ['Firmware', status.version],
    ['Hostname', status.network.hostname],
    ['IP address', status.network.ip],
    ['MAC address', status.network.mac],
    ['Uptime', formatUptime(status.uptimeSeconds)],
    ['Free heap', (status.freeHeap / 1024).toFixed(1) + ' KiB'],
  ].map(([key, value]) => `<div><span>${key}</span><br>${value}</div>`).join('');
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor(seconds % 86400 / 3600);
  const minutes = Math.floor(seconds % 3600 / 60);
  return (days ? days + 'd ' : '') + hours + 'h ' + minutes + 'm';
}

let credentials = '';
try { credentials = sessionStorage.getItem('auth') || ''; } catch (error) { /* storage off */ }

const encode = text => btoa(String.fromCharCode(...new TextEncoder().encode(text)));

// fetch() never raises the browser's own password dialog, so a 401 is handled
// here. Asking on page load would be rude, hence the interactive flag.
async function authedFetch(url, options = {}, interactive = true) {
  const send = () => fetch(url, {
    ...options,
    cache: 'no-store',
    headers: credentials ? { ...options.headers, Authorization: 'Basic ' + credentials } : options.headers,
  });

  let response = await send();
  if (response.status !== 401 || !interactive) return response;

  const password = prompt('Admin password');
  if (password === null) return response;

  credentials = encode('admin:' + password);
  try { sessionStorage.setItem('auth', credentials); } catch (error) { /* storage off */ }

  response = await send();
  if (response.status === 401) {
    credentials = '';
    try { sessionStorage.removeItem('auth'); } catch (error) { /* storage off */ }
    toast('Wrong password');
  }

  return response;
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

async function loadSettings(interactive = false) {
  const response = await authedFetch('/api/config', {}, interactive);
  const locked = response.status === 401;

  el('settingsLock').hidden = !locked;
  el('settingsFields').hidden = locked;
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

  const response = await authedFetch('/api/config', {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload),
  });

  toast(response.ok ? 'Saved, rebooting...' : 'Save failed');
});

el('reload').addEventListener('click', () => loadSettings(true));
el('unlock').addEventListener('click', () => loadSettings(true));

el('reboot').addEventListener('click', async () => {
  if (!confirm('Reboot the sensor?')) return;
  await authedFetch('/api/reboot', { method: 'POST' });
  toast('Rebooting...');
});

el('calibrate').addEventListener('click', async () => {
  if (!confirm('Start CO2 background calibration? The sensor must have been in fresh outdoor air for several minutes.')) return;
  const response = await authedFetch('/api/calibrate-co2', { method: 'POST' });
  toast(response.ok ? 'Calibration started' : 'Calibration failed');
});

buildCards();
refresh();
loadSettings();
setInterval(refresh, 3000);
</script>
</body>
</html>
)rawliteral";
