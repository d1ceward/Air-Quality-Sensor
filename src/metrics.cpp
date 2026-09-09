#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

// Local includes
#include "app_config.h"
#include "battery.h"
#include "config.h"
#include "metrics.h"
#include "net_time.h"
#include "net_wifi.h"
#include "prom_text.h"
#include "sensors.h"
#include "uptime.h"

namespace Metrics {
  namespace {
    constexpr size_t CHUNK_FLUSH_SIZE = 512;

    // Namespace prefixing every exported metric, so short names such as "temp" or "pressure" cannot collide
    // with another exporter on the same Prometheus.
    const __FlashStringHelper *metricPrefix() { return F("air_quality_"); }

    // A device id long enough to fill this is already unusable as a hostname
    constexpr size_t MAX_LABEL_VALUE = 96;

    // Escaping itself is in PromText so it can be exercised on the host; this only bridges it to String
    String escapeLabel(const String &value) {
      char escaped[MAX_LABEL_VALUE];
      PromText::escapeLabelValue(value.c_str(), escaped, sizeof(escaped));

      return String(escaped);
    }

    void flush(ESP8266WebServer &server, String &buffer, bool force = false) {
      if (buffer.length() == 0)
        return;

      if (!force && buffer.length() < CHUNK_FLUSH_SIZE)
        return;

      server.sendContent(buffer);
      buffer = "";
    }

    // The HELP and TYPE lines of a metric. Prometheus rejects a scrape that repeats them for the same name,
    // so a family with several series emits this once and then only the sample lines.
    void appendHeader(String &buffer, const __FlashStringHelper *name,
                      const __FlashStringHelper *help, const __FlashStringHelper *type) {
      buffer += F("# HELP ");
      buffer += metricPrefix();
      buffer += name;
      buffer += ' ';
      buffer += help;
      buffer += F("\n# TYPE ");
      buffer += metricPrefix();
      buffer += name;
      buffer += ' ';
      buffer += type;
      buffer += '\n';
    }

    void appendSample(String &buffer, const __FlashStringHelper *name, const String &labels,
                      const String &value) {
      buffer += metricPrefix();
      buffer += name;
      buffer += labels;
      buffer += ' ';
      buffer += value;
      buffer += '\n';
    }

    // Same labels as every other metric plus the sensor this series is about
    String sensorLabels(const String &labelBody, const char *sensor) {
      return "{" + labelBody + ",sensor=\"" + sensor + "\"}";
    }

    void appendMetric(ESP8266WebServer &server, String &buffer, const String &labels,
                      const __FlashStringHelper *name, const __FlashStringHelper *help,
                      const String &value) {
      appendHeader(buffer, name, help, F("gauge"));
      appendSample(buffer, name, labels, value);

      flush(server, buffer);
    }
  }

  void sendPrometheus(ESP8266WebServer &server) {
    const Sensors::Snapshot readings = Sensors::snapshot();
    const AppConfig &config = Config::get();

    // Kept without the braces so a per sensor label can be appended to it
    const String labelBody =
      String("id=\"") + escapeLabel(config.deviceId) + "\",mac=\"" + WiFi.macAddress() + "\"";
    const String labels = "{" + labelBody + "}";

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
    // Uptime::seconds() counts the millis() wraps, which a plain millis() / 1000 does not: it drops back to
    // zero every 49.7 days and turns a healthy device into an apparent reboot on the dashboard.
    appendMetric(server, buffer, labels, F("uptime_seconds"), F("Time since boot, in seconds"),
                 String(Uptime::seconds()));
    appendMetric(server, buffer, labels, F("heap_free_bytes"), F("Free heap memory, in bytes"),
                 String(ESP.getFreeHeap()));
    appendMetric(server, buffer, labels, F("heap_fragmentation_percent"),
                 F("Heap fragmentation, in percent"), String(ESP.getHeapFragmentation()));

    // A reading that is not available leaves its metric out entirely, which on its own does not say whether
    // the sensor is dead or the whole device was unreachable. This family is always present, so an alert can
    // be written on the value rather than on absent().
    appendHeader(buffer, F("sensor_up"),
                 F("1 when the sensor has a fresh reading, 0 when it is absent or silent"), F("gauge"));
    appendSample(buffer, F("sensor_up"), sensorLabels(labelBody, "co2"), readings.hasCo2 ? "1" : "0");
    appendSample(buffer, F("sensor_up"), sensorLabels(labelBody, "tvoc"), readings.hasTvoc ? "1" : "0");
    appendSample(buffer, F("sensor_up"), sensorLabels(labelBody, "particulate"),
                 readings.hasParticulate ? "1" : "0");
    appendSample(buffer, F("sensor_up"), sensorLabels(labelBody, "climate"),
                 readings.hasClimate ? "1" : "0");
    appendSample(buffer, F("sensor_up"), sensorLabels(labelBody, "pressure"),
                 readings.hasPressure ? "1" : "0");
    flush(server, buffer);

    // Told apart from sensor_up so a sensor that was never wired can be filtered out of the alerts
    appendHeader(buffer, F("sensor_detected"),
                 F("1 when the sensor has answered at least once since boot"), F("gauge"));
    appendSample(buffer, F("sensor_detected"), sensorLabels(labelBody, "co2"),
                 readings.co2Available ? "1" : "0");
    appendSample(buffer, F("sensor_detected"), sensorLabels(labelBody, "tvoc"),
                 readings.tvocAvailable ? "1" : "0");
    appendSample(buffer, F("sensor_detected"), sensorLabels(labelBody, "particulate"),
                 readings.particulateAvailable ? "1" : "0");
    appendSample(buffer, F("sensor_detected"), sensorLabels(labelBody, "climate"),
                 readings.climateAvailable ? "1" : "0");
    appendSample(buffer, F("sensor_detected"), sensorLabels(labelBody, "pressure"),
                 readings.pressureAvailable ? "1" : "0");

    flush(server, buffer, true);
    server.sendContent("");
  }

  String toJson() {
    const Sensors::Snapshot readings = Sensors::snapshot();
    const AppConfig &config = Config::get();

    JsonDocument document;
    document["deviceId"] = config.deviceId;
    document["version"] = PROJECT_VERSION;
    document["uptimeSeconds"] = Uptime::seconds();
    document["freeHeap"] = ESP.getFreeHeap();
    document["heapFragmentation"] = ESP.getHeapFragmentation();
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

    // Surfaced so the web UI can warn about a device still carrying the password that ships with the
    // firmware, which is public, or none at all
    JsonObject security = document["security"].to<JsonObject>();
    security["adminPasswordDefault"] = Config::usesDefaultAdminPassword();
    security["adminPasswordEmpty"] = Config::hasNoAdminPassword();

    JsonObject values = document["readings"].to<JsonObject>();

    JsonObject co2 = values["co2"].to<JsonObject>();
    co2["valid"] = readings.hasCo2;
    co2["detected"] = readings.co2Available;
    co2["value"] = readings.co2Ppm;

    JsonObject pm25 = values["pm25"].to<JsonObject>();
    pm25["valid"] = readings.hasParticulate;
    pm25["detected"] = readings.particulateAvailable;
    pm25["value"] = readings.particulate.pm25;
    pm25["pm1"] = readings.particulate.pm1;
    pm25["pm10"] = readings.particulate.pm10;
    pm25["aqius"] = readings.aqiUs;
    pm25["fanRunning"] = readings.fanRunning;

    JsonObject tvoc = values["tvoc"].to<JsonObject>();
    tvoc["valid"] = readings.hasTvoc;
    tvoc["detected"] = readings.tvocAvailable;
    tvoc["value"] = readings.tvocPpb;
    tvoc["eco2"] = readings.eco2Ppm;

    JsonObject pressure = values["pressure"].to<JsonObject>();
    pressure["valid"] = readings.hasPressure;
    pressure["detected"] = readings.pressureAvailable;
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
    climate["detected"] = readings.climateAvailable;
    climate["temperature"] = serialized(String(readings.temperatureC, 2));
    climate["humidity"] = serialized(String(readings.humidity, 1));

    String output;
    serializeJson(document, output);

    return output;
  }
}
