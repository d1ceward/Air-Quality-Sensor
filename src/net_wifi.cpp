#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "display.h"
#include "hostname.h"
#include "net_wifi.h"

namespace NetWifi {
  namespace {
    // Answers every query with the access point address, which is what makes a phone joining the setup
    // network pop the "sign in" notification instead of leaving the user to find 192.168.4.1 on their own.
    DNSServer dnsServer;
    bool dnsStarted = false;

    String apSsid;
    String host;
    bool accessPointActive = false;
    bool mdnsStarted = false;
    uint32_t connectStartedMs = 0;
    uint32_t lastRetryMs = 0;
    bool waitingForConnection = false;
    bool connected = false;

    // Sanitising itself is in Hostname so it can be exercised on the host; this only bridges it to String
    String sanitizeHostname(const String &value) {
      char label[Hostname::MAX_LENGTH + 1];

      if (Hostname::sanitize(value.c_str(), label, sizeof(label)) == 0)
        return String(DEFAULT_HOSTNAME);

      return String(label);
    }

    void applyStaticIp() {
      const AppConfig &config = Config::get();
      if (!config.useStaticIp)
        return;

      if (!WiFi.config(config.ip, config.gateway, config.subnet, config.dns))
        Serial.println(F("[wifi] static IP configuration rejected"));
    }

    void startCaptivePortal() {
      if (dnsStarted)
        return;

      // Phones probe a known URL and decide there is a portal when the answer is not the one they expect.
      // Replying NoError to the queries this server cannot answer, rather than NXDOMAIN, is what keeps them
      // asking instead of giving up on the network as broken.
      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

      // A wildcard record, so whatever hostname the phone probes resolves here
      if (!dnsServer.start(CAPTIVE_PORTAL_DNS_PORT, "*", WiFi.softAPIP())) {
        Serial.println(F("[wifi] captive portal DNS start failed"));
        return;
      }

      dnsStarted = true;
    }

    void stopCaptivePortal() {
      if (!dnsStarted)
        return;

      dnsServer.stop();
      dnsStarted = false;
    }

    void startAccessPoint() {
      if (accessPointActive)
        return;

      apSsid = String(DEFAULT_HOSTNAME) + "-setup-" + String(ESP.getChipId(), HEX);

      // The station stays up so the configured network is still retried in the background
      if (Config::hasWifiCredentials())
        WiFi.mode(WIFI_AP_STA);

      WiFi.softAP(apSsid.c_str(), SETUP_AP_PASSWORD);
      accessPointActive = true;

      startCaptivePortal();

      Serial.printf("[wifi] setup access point \"%s\" at %s\n",
                    apSsid.c_str(), WiFi.softAPIP().toString().c_str());
    }

    void stopAccessPoint() {
      if (!accessPointActive)
        return;

      stopCaptivePortal();
      WiFi.softAPdisconnect(true);
      accessPointActive = false;
      WiFi.mode(WIFI_STA);
      Serial.println(F("[wifi] setup access point stopped"));
    }

    void startConnection() {
      const AppConfig &config = Config::get();

      WiFi.mode(accessPointActive ? WIFI_AP_STA : WIFI_STA);
      applyStaticIp();
      WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());

      connectStartedMs = millis();
      lastRetryMs = connectStartedMs;
      waitingForConnection = true;

      Serial.printf("[wifi] connecting to \"%s\"\n", config.wifiSsid.c_str());
    }

    void startMdns() {
      if (mdnsStarted)
        return;

      if (!MDNS.begin(host)) {
        Serial.println(F("[wifi] mDNS start failed"));
        return;
      }

      MDNS.addService("http", "tcp", Config::get().httpPort);
      mdnsStarted = true;
      Serial.printf("[wifi] reachable at http://%s.local:%u/\n", host.c_str(), Config::get().httpPort);
    }

    void stopMdns() {
      if (!mdnsStarted)
        return;

      MDNS.end();
      mdnsStarted = false;
    }

    void onConnected() {
      waitingForConnection = false;
      connected = true;

      // The responder was bound to the access point interface, it has to be restarted on the station one
      const bool rebindMdns = accessPointActive;
      stopAccessPoint();

      if (rebindMdns)
        stopMdns();

      startMdns();

      Serial.printf("[wifi] connected, IP %s, RSSI %d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
  }

  void begin() {
    host = sanitizeHostname(Config::get().deviceId);

    WiFi.persistent(false);

    // A warm reset leaves the radio with the association of the previous run, and the SDK auto connect it
    // starts on its own races the one below. Powering the interface down first gives the station a clean
    // start, otherwise the first attempt after a reboot often runs into the timeout and raises the setup
    // access point for nothing.
    WiFi.mode(WIFI_OFF);
    WiFi.setAutoReconnect(true);
    WiFi.hostname(host);

    if (!Config::hasWifiCredentials()) {
      Serial.println(F("[wifi] no credentials, starting setup access point"));
      WiFi.mode(WIFI_AP);
      startAccessPoint();
      startMdns();
      return;
    }

    WiFi.mode(WIFI_STA);

    // Applied once at boot, saving the settings restarts the device anyway
    if (Config::get().wifiNoSleep)
      WiFi.setSleepMode(WIFI_NONE_SLEEP);

    startConnection();
    Display::showMessage("WiFi", "connecting...");
  }

  void loop() {
    if (dnsStarted)
      dnsServer.processNextRequest();

    if (mdnsStarted)
      MDNS.update();

    // The station can come up long after the setup access point was raised, so the transition is watched on
    // the connection state itself rather than on the pending initial attempt.
    if (WiFi.status() == WL_CONNECTED) {
      if (!connected)
        onConnected();

      return;
    }

    if (connected) {
      connected = false;
      lastRetryMs = millis();
      Serial.println(F("[wifi] connection lost, retrying"));
    }

    if (!Config::hasWifiCredentials())
      return;

    // Give up on the initial attempt and expose the setup portal, while still retrying the configured network
    // in the background.
    if (waitingForConnection && millis() - connectStartedMs >= WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println(F("[wifi] connection timed out"));
      waitingForConnection = false;
      startAccessPoint();
      startMdns();
    }

    if (millis() - lastRetryMs >= WIFI_RETRY_INTERVAL_MS) {
      lastRetryMs = millis();
      WiFi.reconnect();
    }
  }

  bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }

  bool isAccessPointActive() {
    return accessPointActive;
  }

  String hostname() {
    return host;
  }

  String accessPointSsid() {
    return apSsid;
  }

  String ipAddress() {
    if (isConnected())
      return WiFi.localIP().toString();

    if (accessPointActive)
      return WiFi.softAPIP().toString();

    return String("0.0.0.0");
  }

  int8_t rssi() {
    return isConnected() ? static_cast<int8_t>(WiFi.RSSI()) : 0;
  }

  ScanState scan(ScanResult *results, uint8_t capacity, uint8_t &found) {
    found = 0;

    const int8_t state = WiFi.scanComplete();

    if (state == WIFI_SCAN_RUNNING)
      return ScanState::Running;

    // WIFI_SCAN_FAILED also covers "no scan was ever started", which is the first call
    if (state == WIFI_SCAN_FAILED) {
      // Asynchronous on purpose: a blocking scan holds the radio for a couple of seconds, and nothing else
      // in loop() runs meanwhile. Scanning from the access point briefly disturbs the clients joined to it,
      // which is acceptable given that is exactly when it gets used.
      //
      // Both outcomes are negative numbers, so the return value has to be compared and not just tested.
      if (WiFi.scanNetworks(true, false) == WIFI_SCAN_FAILED)
        return ScanState::Failed;

      return ScanState::Running;
    }

    const uint8_t count = static_cast<uint8_t>(state);

    for (uint8_t index = 0; index < count && found < capacity; ++index) {
      const String ssid = WiFi.SSID(index);

      // Hidden networks are left out of the scan, and one that still turns up without a name is nothing to
      // offer in a list. Typing the SSID by hand stays possible for those.
      if (ssid.length() == 0)
        continue;

      // The same network seen on two bands or two access points is one entry, the strongest kept. The list
      // is short enough that a linear look up costs less than sorting first.
      bool merged = false;
      for (uint8_t existing = 0; existing < found; ++existing) {
        if (results[existing].ssid != ssid)
          continue;

        merged = true;
        if (WiFi.RSSI(index) > results[existing].rssi)
          results[existing].rssi = static_cast<int8_t>(WiFi.RSSI(index));

        break;
      }

      if (merged)
        continue;

      results[found].ssid = ssid;
      results[found].rssi = static_cast<int8_t>(WiFi.RSSI(index));
      results[found].secured = WiFi.encryptionType(index) != ENC_TYPE_NONE;
      ++found;
    }

    // Strongest first, which is the order someone picking their own network expects
    for (uint8_t i = 1; i < found; ++i) {
      const ScanResult entry = results[i];
      uint8_t j = i;

      while (j > 0 && results[j - 1].rssi < entry.rssi) {
        results[j] = results[j - 1];
        --j;
      }

      results[j] = entry;
    }

    WiFi.scanDelete();

    return ScanState::Ready;
  }
}
