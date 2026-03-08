#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "athena/DNA.hpp"

namespace athena::io {

class VectorWriter final : public IStreamWriter {
public:
  VectorWriter() = default;

  void writeBytes(const void* buf, size_t sz) override {
    if (!buf || !sz)
      return;
    if (m_pos + sz > m_data.size())
      m_data.resize(m_pos + sz);
    std::memcpy(m_data.data() + m_pos, buf, sz);
    m_pos += sz;
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
      newPos = static_cast<size_t>(static_cast<int64_t>(m_data.size()) + offset);
      break;
    }
    if (newPos > m_data.size())
      m_data.resize(newPos);
    m_pos = newPos;
    return true;
  }

  int64_t position() const override { return static_cast<int64_t>(m_pos); }

  const std::vector<uint8_t>& data() const { return m_data; }
  std::vector<uint8_t>& data() { return m_data; }

private:
  std::vector<uint8_t> m_data;
  size_t m_pos = 0;
};

} // namespace athena::io
