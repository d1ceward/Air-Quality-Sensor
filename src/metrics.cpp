#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

// Local includes
#include "app_config.h"
#include "battery.h"
#include "config.h"
#include "metrics.h"
#include "net_time.h"
#include "net_wifi.h"
#include "sensors.h"

namespace Metrics {
  namespace {
    constexpr size_t CHUNK_FLUSH_SIZE = 512;

    // Namespace prefixing every exported metric, so short names such as "temp" or "pressure" cannot collide
    // with another exporter on the same Prometheus.
    const __FlashStringHelper *metricPrefix() { return F("air_quality_"); }

    // Label values must escape backslashes, quotes and newlines
    String escapeLabel(const String &value) {
      String result;
      result.reserve(value.length() + 4);

      for (size_t i = 0; i < value.length(); ++i) {
        const char character = value[i];

        switch (character) {
          case '\\': result += F("\\\\"); break;
          case '"': result += F("\\\""); break;
          case '\n': result += F("\\n"); break;
          default: result += character; break;
        }
      }

      return result;
    }

    void flush(ESP8266WebServer &server, String &buffer, bool force = false) {
      if (buffer.length() == 0)
        return;

      if (!force && buffer.length() < CHUNK_FLUSH_SIZE)
        return;

      server.sendContent(buffer);
      buffer = "";
    }

    void appendMetric(ESP8266WebServer &server, String &buffer, const String &labels,
                      const __FlashStringHelper *name, const __FlashStringHelper *help,
                      const String &value) {
      buffer += F("# HELP ");
      buffer += metricPrefix();
      buffer += name;
      buffer += ' ';
      buffer += help;
      buffer += F("\n# TYPE ");
      buffer += metricPrefix();
      buffer += name;
      buffer += F(" gauge\n");
      buffer += metricPrefix();
      buffer += name;
      buffer += labels;
      buffer += ' ';
      buffer += value;
      buffer += '\n';

      flush(server, buffer);
    }
  }

  void sendPrometheus(ESP8266WebServer &server) {
    const Sensors::Snapshot readings = Sensors::snapshot();
    const AppConfig &config = Config::get();

    const String labels =
      String("{id=\"") + escapeLabel(config.deviceId) + "\",mac=\"" + WiFi.macAddress() + "\"}";

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, F("text/plain; version=0.0.4; charset=utf-8"), "");

    String buffer;
    buffer.reserve(CHUNK_FLUSH_SIZE + 256);

    // Names below are the suffixes; metricPrefix() namespaces each of them.
    if (readings.hasTvoc) {
      appendMetric(server, buffer, labels, F("tvoc"), F("TVOC value, in ppb"),
                   String(readings.tvocPpb));
      appendMetric(server, buffer, labels, F("eco2"), F("Equivalent CO2 estimated by the SGP30, in ppm"),
                   String(readings.eco2Ppm));
    }

    if (readings.hasCo2)
      appendMetric(server, buffer, labels, F("co2"), F("CO2 value, in ppm"),
                   String(readings.co2Ppm));

    if (readings.hasParticulate) {
      appendMetric(server, buffer, labels, F("pm25"), F("Particulate Matter PM2.5 value, in ug/m3"),
                   String(readings.particulate.pm25));
      appendMetric(server, buffer, labels, F("pm1"), F("Particulate Matter PM1.0 value, in ug/m3"),
                   String(readings.particulate.pm1));
      appendMetric(server, buffer, labels, F("pm10"), F("Particulate Matter PM10 value, in ug/m3"),
                   String(readings.particulate.pm10));
      appendMetric(server, buffer, labels, F("aqius"), F("US Air Quality Index value"),
                   String(readings.aqiUs));
      appendMetric(server, buffer, labels, F("particle_count_03"),
                   F("Particles above 0.3 um per 0.1 L of air"),
                   String(readings.particulate.count03));
      appendMetric(server, buffer, labels, F("particle_count_25"),
                   F("Particles above 2.5 um per 0.1 L of air"),
                   String(readings.particulate.count25));
    }

    if (readings.hasClimate) {
      appendMetric(server, buffer, labels, F("temp"), F("Temperature, in degrees Celsius"),
                   String(readings.temperatureC, 2));
      appendMetric(server, buffer, labels, F("hum"), F("Relative humidity, in percent"),
                   String(readings.humidity, 2));
    }

    if (readings.hasPressure) {
      appendMetric(server, buffer, labels, F("pressure"),
                   F("Atmospheric pressure measured at the station, in hPa"),
                   String(readings.pressureHpa, 2));
      appendMetric(server, buffer, labels, F("pressure_sea_level"),
                   F("Atmospheric pressure reduced to sea level, in hPa"),
                   String(readings.seaLevelPressureHpa, 2));
      appendMetric(server, buffer, labels, F("altitude"),
                   F("Altitude derived from the standard atmosphere, in meters"),
                   String(readings.altitudeM, 1));
    }

    if (Battery::hasReading()) {
      appendMetric(server, buffer, labels, F("battery_volts"), F("Battery cell voltage, in volts"),
                   String(Battery::volts(), 3));
      appendMetric(server, buffer, labels, F("battery_percent"),
                   F("Battery state of charge, in percent"), String(Battery::percent()));
    }

    appendMetric(server, buffer, labels, F("wifi_rssi"), F("WiFi signal strength, in dBm"),
                 String(NetWifi::rssi()));
    appendMetric(server, buffer, labels, F("uptime_seconds"), F("Time since boot, in seconds"),
                 String(millis() / 1000UL));
    appendMetric(server, buffer, labels, F("heap_free_bytes"), F("Free heap memory, in bytes"),
                 String(ESP.getFreeHeap()));

    flush(server, buffer, true);
    server.sendContent("");
  }

  String toJson() {
    const Sensors::Snapshot readings = Sensors::snapshot();
    const AppConfig &config = Config::get();

    JsonDocument document;
    document["deviceId"] = config.deviceId;
    document["version"] = PROJECT_VERSION;
    document["uptimeSeconds"] = millis() / 1000UL;
    document["freeHeap"] = ESP.getFreeHeap();
    document["time"] = NetTime::isSynced() ? NetTime::isoTimestamp() : String("");
    document["timezone"] = NetTime::isSynced() ? NetTime::zoneAbbreviation() : String("");

    JsonObject network = document["network"].to<JsonObject>();
    network["connected"] = NetWifi::isConnected();
    network["accessPoint"] = NetWifi::isAccessPointActive();
    network["ssid"] = NetWifi::isConnected() ? WiFi.SSID() : NetWifi::accessPointSsid();
    network["ip"] = NetWifi::ipAddress();
    network["mac"] = WiFi.macAddress();
    network["rssi"] = NetWifi::rssi();
    network["hostname"] = NetWifi::hostname();

    JsonObject values = document["readings"].to<JsonObject>();

    JsonObject co2 = values["co2"].to<JsonObject>();
    co2["valid"] = readings.hasCo2;
    co2["value"] = readings.co2Ppm;

    JsonObject pm25 = values["pm25"].to<JsonObject>();
    pm25["valid"] = readings.hasParticulate;
    pm25["value"] = readings.particulate.pm25;
    pm25["pm1"] = readings.particulate.pm1;
    pm25["pm10"] = readings.particulate.pm10;
    pm25["aqius"] = readings.aqiUs;
    pm25["fanRunning"] = readings.fanRunning;

    JsonObject tvoc = values["tvoc"].to<JsonObject>();
    tvoc["valid"] = readings.hasTvoc;
    tvoc["value"] = readings.tvocPpb;
    tvoc["eco2"] = readings.eco2Ppm;

    JsonObject pressure = values["pressure"].to<JsonObject>();
    pressure["valid"] = readings.hasPressure;
    pressure["value"] = serialized(String(readings.pressureHpa, 2));
    pressure["seaLevel"] = serialized(String(readings.seaLevelPressureHpa, 2));
    pressure["altitude"] = serialized(String(readings.altitudeM, 1));
    pressure["temperature"] = serialized(String(readings.barometerTemperatureC, 2));

    JsonObject battery = values["battery"].to<JsonObject>();
    battery["enabled"] = Battery::isEnabled();
    battery["valid"] = Battery::hasReading();
    battery["volts"] = serialized(String(Battery::volts(), 3));
    battery["percent"] = Battery::percent();

    JsonObject climate = values["climate"].to<JsonObject>();
    climate["valid"] = readings.hasClimate;
    climate["temperature"] = serialized(String(readings.temperatureC, 2));
    climate["humidity"] = serialized(String(readings.humidity, 1));

    String output;
    serializeJson(document, output);

    return output;
  }
}
