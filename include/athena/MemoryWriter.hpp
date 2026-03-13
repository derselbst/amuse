#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "athena/IStream.hpp"

namespace athena::io {

// ---------------------------------------------------------------------------
// MemoryWriter – writes into a caller-owned fixed-size byte buffer.
// ---------------------------------------------------------------------------
class MemoryWriter : public IStreamWriter {
    uint8_t* m_data   = nullptr;
    uint64_t m_length = 0;
    int64_t  m_pos    = 0;

protected:
    void writeUBytesToBuf_raw(const void* buf, uint64_t len) override {
        if (m_pos < 0 || static_cast<uint64_t>(m_pos) + len > m_length) {
            m_hasError = true;
            return;
        }
        std::memcpy(m_data + m_pos, buf, len);
        m_pos += static_cast<int64_t>(len);
    }

public:
    MemoryWriter(uint8_t* data, uint64_t length)
        : m_data(data), m_length(length) {}

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

// ---------------------------------------------------------------------------
// MemoryCopyWriter – like MemoryWriter but owns its own growing buffer.
// Useful when you don't have a pre-allocated destination.
// ---------------------------------------------------------------------------
class MemoryCopyWriter : public IStreamWriter {
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
    MemoryCopyWriter() = default;
    explicit MemoryCopyWriter(const void* data, uint64_t length)
        : m_buf(static_cast<const uint8_t*>(data),
                static_cast<const uint8_t*>(data) + length) {}

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
