#pragma once

#include <Arduino.h>

#include "log_ring.h"

// Drop-in for Serial on the output side: every byte still goes to the serial port, and a copy lands in a ring
// buffer the web UI reads back at /logs.
class ConsoleOutput : public Print {
 public:
  size_t write(uint8_t byte) override;
  size_t write(const uint8_t *buffer, size_t size) override;

  const LogRing::Buffer &ring() const { return ring_; }

 private:
  LogRing::Buffer ring_;
};

extern ConsoleOutput Console;
