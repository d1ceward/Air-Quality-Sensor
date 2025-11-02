#include <EEPROM.h>

// Local includes
#include "config.h"
#include "storage.h"

namespace Storage {
  static bool initialized = false;

  // Initialize EEPROM with given size
  void begin() {
    if (!initialized) {
      EEPROM.begin(EEPROM_SIZE);
      initialized = true;
    }
  }

  // Write a string to EEPROM at specified address
  void writeString(int addr, const String& value, size_t maxLen) {
    size_t i = 0;
    for (; i < value.length() && i < maxLen - 1; ++i)
      EEPROM.write(addr + i, value[i]);

    // Null-terminate if space allows
    if (i < maxLen)
      EEPROM.write(addr + i, 0);

    // Fill remaining space with zeros if any
    for (++i; i < maxLen; ++i)
      EEPROM.write(addr + i, 0);
  }

  // Read a string from EEPROM at specified address
  String readString(int addr, size_t maxLen) {
    String value;
    for (size_t i = 0; i < maxLen; ++i) {
      char c = EEPROM.read(addr + i);
      if (c == 0xFF || c == 0)
        break;
      value += c;
    }

    return value;
  }

  void commit() {
    EEPROM.commit();
  }
}
