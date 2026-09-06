#include "display_schedule.h"

namespace DisplaySchedule {
  bool isNight(uint8_t hour, uint8_t startHour, uint8_t endHour) {
    // Equal bounds would otherwise mean either a full day or nothing at all
    if (startHour == endHour)
      return false;

    if (hour > 23 || startHour > 23 || endHour > 23)
      return false;

    // 22 to 7 wraps past midnight, 1 to 5 does not
    if (startHour > endHour)
      return hour >= startHour || hour < endHour;

    return hour >= startHour && hour < endHour;
  }
}
