#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "athena/DNA.hpp"

namespace athena::io {

class MemoryWriter final : public IStreamWriter {
public:
  MemoryWriter(void* data, size_t size) : m_data(static_cast<uint8_t*>(data)), m_size(size) {}

  void writeBytes(const void* buf, size_t sz) override {
    if (!buf || !m_data)
      return;
    size_t remaining = m_size - m_pos;
    size_t toWrite = std::min(sz, remaining);
    if (toWrite) {
      std::memcpy(m_data + m_pos, buf, toWrite);
      m_pos += toWrite;
    }
    if (toWrite < sz)
      m_error = true;
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
  uint8_t* m_data = nullptr;
  size_t m_size = 0;
  size_t m_pos = 0;
  bool m_error = false;
};

} // namespace athena::io
