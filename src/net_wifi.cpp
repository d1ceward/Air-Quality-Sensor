#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "display.h"
#include "net_wifi.h"

namespace NetWifi {
  namespace {
    String apSsid;
    String host;
    bool accessPointActive = false;
    bool mdnsStarted = false;
    uint32_t connectStartedMs = 0;
    uint32_t lastRetryMs = 0;
    bool waitingForConnection = false;
    bool connected = false;

    // Hostnames only accept letters, digits and hyphens
    String sanitizeHostname(const String &value) {
      String result;
      result.reserve(value.length());

      for (size_t i = 0; i < value.length(); ++i) {
        const char character = value[i];
        if (isAlphaNumeric(character) || character == '-')
          result += character;
        else if (character == ' ' || character == '_')
          result += '-';
      }

      if (result.length() == 0)
        result = DEFAULT_HOSTNAME;

      return result;
    }

    void applyStaticIp() {
      const AppConfig &config = Config::get();
      if (!config.useStaticIp)
        return;

      if (!WiFi.config(config.ip, config.gateway, config.subnet, config.dns))
        Serial.println(F("[wifi] static IP configuration rejected"));
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

      Serial.printf("[wifi] setup access point \"%s\" at %s\n",
                    apSsid.c_str(), WiFi.softAPIP().toString().c_str());
    }

    void stopAccessPoint() {
      if (!accessPointActive)
        return;

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
    startConnection();
    Display::showMessage("WiFi", "connecting...");
  }

  void loop() {
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
}
