#pragma once

#include <Arduino.h>
#include <time.h>

// SNTP time, with the timezone and DST rules handled by the C library through
// a POSIX TZ string (no fixed UTC offset to update twice a year).
namespace NetTime {
  void begin();

  bool isSynced();
  bool localTime(struct tm &result);

  String formattedDate();     // DD/MM
  String formattedYear();     // YYYY
  String formattedWeekday();  // Mon, Tue, ...
  String formattedClock();    // HH:MM
  String isoTimestamp();      // YYYY-MM-DDTHH:MM:SS
}
