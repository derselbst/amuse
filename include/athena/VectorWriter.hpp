#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "athena/IStream.hpp"

namespace athena::io {

// ---------------------------------------------------------------------------
// VectorWriter – writes into an internal std::vector<uint8_t>.
// Supports seeking and random-access writes (the vector grows as needed).
// ---------------------------------------------------------------------------
class VectorWriter : public IStreamWriter {
    std::vector<uint8_t> m_buf;
    int64_t              m_pos = 0;

protected:
    void writeUBytesToBuf_raw(const void* buf, uint64_t len) override {
        uint64_t end = static_cast<uint64_t>(m_pos) + len;
        if (end > m_buf.size())
            m_buf.resize(end, 0);
        std::memcpy(m_buf.data() + m_pos, buf, len);
        m_pos += static_cast<int64_t>(len);
    }

public:
    VectorWriter() = default;

    void seek(int64_t off, athena::SeekOrigin origin) override {
        int64_t newPos = m_pos;
        switch (origin) {
        case athena::SeekOrigin::Begin:   newPos = off; break;
        case athena::SeekOrigin::Current: newPos += off; break;
        case athena::SeekOrigin::End:     newPos = static_cast<int64_t>(m_buf.size()) + off; break;
        }
        if (newPos < 0)
            m_hasError = true;
        else
            m_pos = newPos;
    }

    int64_t position() const override { return m_pos; }

    const std::vector<uint8_t>& data() const noexcept { return m_buf; }
    std::vector<uint8_t>&       data()       noexcept { return m_buf; }
};

} // namespace athena::io
