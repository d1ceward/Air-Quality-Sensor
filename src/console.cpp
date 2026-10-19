#include "console.h"

ConsoleOutput Console;

size_t ConsoleOutput::write(uint8_t byte) {
  ring_.write(byte);

  return Serial.write(byte);
}

size_t ConsoleOutput::write(const uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; ++i)
    ring_.write(buffer[i]);

  return Serial.write(buffer, size);
}
