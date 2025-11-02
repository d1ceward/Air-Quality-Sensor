// Local includes
#include "config.h"
#include "wifi.h"
#include "storage.h"

#define EEPROM_SIZE 128
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32

static char wifi_ssid[WIFI_SSID_MAXLEN + 1] = {0};
static char wifi_pass[WIFI_PASS_MAXLEN + 1] = {0};

namespace WifiConnect {
  void loadConfig() {
    // Initialize EEPROM
    Storage::begin(EEPROM_SIZE);

    // Read stored SSID and password
    String ssid = Storage::readString(WIFI_SSID_ADDR, WIFI_SSID_MAXLEN);
    String pass = Storage::readString(WIFI_PASS_ADDR, WIFI_PASS_MAXLEN);

    // Use default values if none are stored
    if (ssid.length() == 0)
      ssid = WIFI_SSID;
    if (pass.length() == 0)
      pass = WIFI_PASSWORD;

    // Update in-memory copies
    strncpy(wifi_ssid, ssid.c_str(), WIFI_SSID_MAXLEN);
    wifi_ssid[WIFI_SSID_MAXLEN] = '\0';
    strncpy(wifi_pass, pass.c_str(), WIFI_PASS_MAXLEN);
    wifi_pass[WIFI_PASS_MAXLEN] = '\0';
  }

  void saveConfig(const char* ssid, const char* pass) {
    // Write SSID and password to EEPROM
    Storage::writeString(WIFI_SSID_ADDR, ssid, WIFI_SSID_MAXLEN);
    Storage::writeString(WIFI_PASS_ADDR, pass, WIFI_PASS_MAXLEN);

    // Commit changes to EEPROM
    Storage::commit();

    // Update in-memory copies
    strncpy(wifi_ssid, ssid, WIFI_SSID_MAXLEN);
    wifi_ssid[WIFI_SSID_MAXLEN] = '\0';
    strncpy(wifi_pass, pass, WIFI_PASS_MAXLEN);
    wifi_pass[WIFI_PASS_MAXLEN] = '\0';
  }

  const char* getSSID() { return wifi_ssid; }
  const char* getPASS() { return wifi_pass; }

  void connect(const char* ssid, const char* pass) {
    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);

    // Set static IP configuration
    if (!WiFi.config(STATIC_IP, GATEWAY, SUBNET, DNS)) {
      Serial.println("STA Failed to configure");
    }

    // Set device hostname
    WiFi.setHostname(WIFI_HOSTNAME);

    // Begin WiFi connection
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    // Wait for connection with retries
    int wifi_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_attempts < MAX_WIFI_ATTEMPTS) {
        delay(WIFI_RETRY_DELAY_MS);
        Serial.print(".");
        ++wifi_attempts;
    }

    // Print connection result
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi connected. IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi connection failed.");
    }
  }
}
