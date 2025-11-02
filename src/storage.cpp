#include <EEPROM.h>

// Local includes
#include "storage.h"

namespace Storage {
  static bool initialized = false;

  // Initialize EEPROM with given size
  void begin(size_t size) {
    if (!initialized) {
      EEPROM.begin(size);

      initialized = true;
    }
  }

  // Write a string to EEPROM at specified address
  void writeString(int addr, const String& value, size_t maxLen) {
    for (size_t i = 0; i < maxLen; i++) {
      char c = (i < value.length()) ? value[i] : 0;

      EEPROM.write(addr + i, c);
    }
  }

  String readString(int addr, size_t maxLen) {
    String value;
    for (size_t i = 0; i < maxLen; i++) {
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
