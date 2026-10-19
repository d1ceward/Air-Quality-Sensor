#include "log_ring.h"

namespace LogRing {
  void Buffer::clear() {
    total_ = 0;
  }

  void Buffer::write(uint8_t byte) {
    data_[total_ % CAPACITY] = static_cast<char>(byte);
    ++total_;
  }

  uint32_t Buffer::clamp(uint32_t offset) const {
    if (offset < oldest() || offset > total_)
      return oldest();

    return offset;
  }

  size_t Buffer::read(uint32_t offset, char *out, size_t outSize) const {
    offset = clamp(offset);

    size_t written = 0;
    while (offset < total_ && written < outSize)
      out[written++] = data_[offset++ % CAPACITY];

    return written;
  }
}
