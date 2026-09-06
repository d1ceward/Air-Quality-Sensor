#include <Arduino.h>

// Local includes
#include "app_config.h"
#include "config.h"
#include "net_time.h"

namespace NetTime {
  namespace {
    // Anything before 2020 means SNTP has not answered yet
    constexpr time_t MIN_VALID_EPOCH = 1577836800;

    const char *const WEEKDAYS[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    String format(const char *pattern) {
      struct tm timeInfo;
      if (!localTime(timeInfo))
        return String("--");

      char buffer[32];
      strftime(buffer, sizeof(buffer), pattern, &timeInfo);

      return String(buffer);
    }
  }

  void begin() {
    const AppConfig &config = Config::get();
    configTime(config.timezone.c_str(), config.ntpServer.c_str());
    Serial.printf("[time] SNTP server %s, timezone %s\n",
                  config.ntpServer.c_str(), config.timezone.c_str());
  }

  bool isSynced() {
    return time(nullptr) >= MIN_VALID_EPOCH;
  }

  bool localTime(struct tm &result) {
    const time_t now = time(nullptr);
    if (now < MIN_VALID_EPOCH)
      return false;

    localtime_r(&now, &result);
    return true;
  }

  String formattedDate() {
    return format("%d/%m");
  }

  String formattedYear() {
    return format("%Y");
  }

  String formattedWeekday() {
    struct tm timeInfo;
    if (!localTime(timeInfo))
      return String("--");

    return String(WEEKDAYS[timeInfo.tm_wday % 7]);
  }

  String formattedClock() {
    return format("%H:%M");
  }

  String isoTimestamp() {
    return format("%Y-%m-%dT%H:%M:%S");
  }
}
