#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "athena/DNA.hpp"

namespace athena::io {

class MemoryReader final : public IStreamReader {
public:
  MemoryReader(const void* data, size_t size) : m_data(static_cast<const uint8_t*>(data)), m_size(size) {}

  size_t readBytesToBuf(void* buf, size_t sz) override {
    if (!buf || !m_data)
      return 0;
    size_t remaining = m_size - m_pos;
    size_t toRead = std::min(sz, remaining);
    if (toRead) {
      std::memcpy(buf, m_data + m_pos, toRead);
      m_pos += toRead;
    }
    if (toRead < sz)
      m_error = true;
    return toRead;
  }

  bool seek(int64_t offset, SeekOrigin origin) override {
    size_t newPos = m_pos;
    switch (origin) {
    case SeekOrigin::Begin:
      newPos = static_cast<size_t>(offset);
      break;
    case SeekOrigin::Current:
      newPos = static_cast<size_t>(static_cast<int64_t>(m_pos) + offset);
      break;
    case SeekOrigin::End:
      newPos = static_cast<size_t>(static_cast<int64_t>(m_size) + offset);
      break;
    }
    if (newPos > m_size) {
      m_error = true;
      return false;
    }
    m_pos = newPos;
    return true;
  }

  int64_t position() const override { return static_cast<int64_t>(m_pos); }
  bool hasError() const override { return m_error; }

private:
  const uint8_t* m_data = nullptr;
  size_t m_size = 0;
  size_t m_pos = 0;
  bool m_error = false;
};

} // namespace athena::io
