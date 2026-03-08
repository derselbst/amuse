#pragma once

#include <cstdint>
#include <cstring>

#include "athena/IStream.hpp"

namespace athena::io {

// ---------------------------------------------------------------------------
// MemoryReader – reads from a caller-owned const byte buffer.
// ---------------------------------------------------------------------------
class MemoryReader : public IStreamReader {
    const uint8_t* m_data   = nullptr;
    uint64_t       m_length = 0;
    int64_t        m_pos    = 0;

protected:
    uint64_t readUBytesToBuf_raw(void* buf, uint64_t len) override {
        if (m_pos < 0 || static_cast<uint64_t>(m_pos) >= m_length) {
            m_hasError = true;
            return 0;
        }
        uint64_t available = m_length - static_cast<uint64_t>(m_pos);
        uint64_t toRead    = (len < available) ? len : available;
        std::memcpy(buf, m_data + m_pos, toRead);
        m_pos += static_cast<int64_t>(toRead);
        if (toRead < len)
            m_hasError = true;
        return toRead;
    }

public:
    MemoryReader(const void* data, uint64_t length)
        : m_data(static_cast<const uint8_t*>(data)), m_length(length) {}

    void seek(int64_t off, athena::SeekOrigin origin) override {
        int64_t newPos = m_pos;
        switch (origin) {
        case athena::SeekOrigin::Begin:   newPos = off; break;
        case athena::SeekOrigin::Current: newPos += off; break;
        case athena::SeekOrigin::End:     newPos = static_cast<int64_t>(m_length) + off; break;
        }
        if (newPos < 0 || static_cast<uint64_t>(newPos) > m_length)
            m_hasError = true;
        else
            m_pos = newPos;
    }

    int64_t position() const override { return m_pos; }
};

} // namespace athena::io
