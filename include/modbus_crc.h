#pragma once

#include <stddef.h>
#include <stdint.h>

// CRC16 as used by Modbus RTU. The result is transmitted low byte first, so a
// frame ending with 0xD5 0xC5 carries the checksum 0xC5D5.
//
// Free of Arduino dependencies so it can be exercised on the host.
uint16_t modbusCrc16(const uint8_t *data, size_t length);
