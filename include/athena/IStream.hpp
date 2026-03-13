#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

#include "athena/Types.hpp"

// ── Enumerations ─────────────────────────────────────────────────────────────
namespace athena {

enum class SeekOrigin : uint8_t { Begin, Current, End };

} // namespace athena

// ── Abstract stream interfaces ────────────────────────────────────────────────
namespace athena::io {

// ---------------------------------------------------------------------------
// IStreamReader
// ---------------------------------------------------------------------------
// Concrete implementations must override the three pure-virtual primitives:
//   readUBytesToBuf_raw() – raw byte read, returns bytes actually read
//   seek()                – reposition the stream cursor
//   position()            – return the current cursor position
//
// All higher-level typed read methods are implemented here in terms of those
// primitives (no additional virtual dispatch overhead).
// ---------------------------------------------------------------------------
class IStreamReader {
protected:
    bool m_hasError = false;

    // Raw read: fills buf with up to len bytes.  Returns bytes read.
    virtual uint64_t readUBytesToBuf_raw(void* buf, uint64_t len) = 0;

public:
    virtual ~IStreamReader() = default;

    virtual void seek(int64_t off, athena::SeekOrigin origin) = 0;
    virtual int64_t position() const = 0;

    bool hasError() const noexcept { return m_hasError; }

    // ── Bulk read ─────────────────────────────────────────────────────────
    void readUBytesToBuf(void* buf, uint64_t len) {
        if (readUBytesToBuf_raw(buf, len) != len)
            m_hasError = true;
    }
    void readBytesToBuf(void* buf, uint64_t len) {
        readUBytesToBuf(buf, len);
    }

    // ── Heap-allocated bulk read ──────────────────────────────────────────
    std::unique_ptr<uint8_t[]> readUBytes(uint64_t len) {
        auto buf = std::make_unique<uint8_t[]>(len);
        readUBytesToBuf(buf.get(), len);
        return buf;
    }

    // ── Scalar read helpers ───────────────────────────────────────────────
    uint8_t  readUByte()  { uint8_t  v = 0; readUBytesToBuf(&v, 1); return v; }
    int8_t   readByte()   { int8_t   v = 0; readUBytesToBuf(&v, 1); return v; }
    bool     readBool()   { return readUByte() != 0; }

    uint16_t readUint16Big()    { uint16_t v = 0; readUBytesToBuf(&v, 2); return SBig(v); }
    uint16_t readUint16Little() { uint16_t v = 0; readUBytesToBuf(&v, 2); return SLittle(v); }
    int16_t  readInt16Big()     { uint16_t v = 0; readUBytesToBuf(&v, 2); v = SBig(v);    int16_t r; std::memcpy(&r, &v, 2); return r; }
    int16_t  readInt16Little()  { uint16_t v = 0; readUBytesToBuf(&v, 2); v = SLittle(v); int16_t r; std::memcpy(&r, &v, 2); return r; }

    uint32_t readUint32Big()    { uint32_t v = 0; readUBytesToBuf(&v, 4); return SBig(v); }
    uint32_t readUint32Little() { uint32_t v = 0; readUBytesToBuf(&v, 4); return SLittle(v); }
    int32_t  readInt32Big()     { uint32_t v = 0; readUBytesToBuf(&v, 4); v = SBig(v);    int32_t r; std::memcpy(&r, &v, 4); return r; }
    int32_t  readInt32Little()  { uint32_t v = 0; readUBytesToBuf(&v, 4); v = SLittle(v); int32_t r; std::memcpy(&r, &v, 4); return r; }

    uint64_t readUint64Big()    { uint64_t v = 0; readUBytesToBuf(&v, 8); return SBig(v); }
    uint64_t readUint64Little() { uint64_t v = 0; readUBytesToBuf(&v, 8); return SLittle(v); }
    int64_t  readInt64Big()     { uint64_t v = 0; readUBytesToBuf(&v, 8); v = SBig(v);    int64_t r; std::memcpy(&r, &v, 8); return r; }
    int64_t  readInt64Little()  { uint64_t v = 0; readUBytesToBuf(&v, 8); v = SLittle(v); int64_t r; std::memcpy(&r, &v, 8); return r; }

    float  readFloat()  {
        uint32_t v = 0; readUBytesToBuf(&v, 4); v = SBig(v);
        float r; std::memcpy(&r, &v, 4); return r;
    }
    double readDouble() {
        uint64_t v = 0; readUBytesToBuf(&v, 8); v = SBig(v);
        double r; std::memcpy(&r, &v, 8); return r;
    }
};

// ---------------------------------------------------------------------------
// IStreamWriter
// ---------------------------------------------------------------------------
class IStreamWriter {
protected:
    bool m_hasError = false;

    // Raw write: writes len bytes from buf.
    virtual void writeUBytesToBuf_raw(const void* buf, uint64_t len) = 0;

public:
    virtual ~IStreamWriter() = default;

    virtual void seek(int64_t off, athena::SeekOrigin origin) = 0;
    virtual int64_t position() const = 0;

    bool hasError() const noexcept { return m_hasError; }

    // ── Bulk write ────────────────────────────────────────────────────────
    void writeBytes(const void* data, uint64_t len)  { writeUBytesToBuf_raw(data, len); }
    void writeUBytes(const void* data, uint64_t len) { writeUBytesToBuf_raw(data, len); }

    // ── Scalar write helpers ──────────────────────────────────────────────
    void writeUByte(uint8_t val)   { writeUBytesToBuf_raw(&val, 1); }
    void writeByte(int8_t val)     { writeUBytesToBuf_raw(&val, 1); }
    void writeBool(bool val)       { uint8_t b = val ? 1 : 0; writeUBytesToBuf_raw(&b, 1); }

    void writeUint16Big(uint16_t val)    { val = SBig(val);    writeUBytesToBuf_raw(&val, 2); }
    void writeUint16Little(uint16_t val) { val = SLittle(val); writeUBytesToBuf_raw(&val, 2); }
    void writeInt16Big(int16_t val)      { uint16_t v; std::memcpy(&v, &val, 2); v = SBig(v);    writeUBytesToBuf_raw(&v, 2); }
    void writeInt16Little(int16_t val)   { uint16_t v; std::memcpy(&v, &val, 2); v = SLittle(v); writeUBytesToBuf_raw(&v, 2); }

    void writeUint32Big(uint32_t val)    { val = SBig(val);    writeUBytesToBuf_raw(&val, 4); }
    void writeUint32Little(uint32_t val) { val = SLittle(val); writeUBytesToBuf_raw(&val, 4); }
    void writeInt32Big(int32_t val)      { uint32_t v; std::memcpy(&v, &val, 4); v = SBig(v);    writeUBytesToBuf_raw(&v, 4); }
    void writeInt32Little(int32_t val)   { uint32_t v; std::memcpy(&v, &val, 4); v = SLittle(v); writeUBytesToBuf_raw(&v, 4); }

    void writeUint64Big(uint64_t val)    { val = SBig(val);    writeUBytesToBuf_raw(&val, 8); }
    void writeUint64Little(uint64_t val) { val = SLittle(val); writeUBytesToBuf_raw(&val, 8); }
    void writeInt64Big(int64_t val)      { uint64_t v; std::memcpy(&v, &val, 8); v = SBig(v);    writeUBytesToBuf_raw(&v, 8); }
    void writeInt64Little(int64_t val)   { uint64_t v; std::memcpy(&v, &val, 8); v = SLittle(v); writeUBytesToBuf_raw(&v, 8); }

    void writeFloat(float val)   { uint32_t v; std::memcpy(&v, &val, 4); v = SBig(v);    writeUBytesToBuf_raw(&v, 4); }
    void writeDouble(double val) { uint64_t v; std::memcpy(&v, &val, 8); v = SBig(v);    writeUBytesToBuf_raw(&v, 8); }

    // ── Alignment helpers ─────────────────────────────────────────────────
    void seekAlign32() {
        int64_t pos = position();
        int64_t aligned = static_cast<int64_t>(ROUND_UP_32(static_cast<uint64_t>(pos)));
        if (aligned > pos) {
            static const uint8_t zeros[32]{};
            int64_t pad = aligned - pos;
            writeUBytesToBuf_raw(zeros, static_cast<uint64_t>(pad));
        }
    }
};

} // namespace athena::io

// ── athena::utility – endian constants ───────────────────────────────────────
// Mirrors the real athena Utility.hpp: SystemEndian is the native platform
// endianness; NotSystemEndian is its opposite.  Used as template arguments for
// readCmds<E>/writeCmds<E> where game data (big-endian) differs from host.
namespace athena::utility {
constexpr ::std::endian SystemEndian    = std::endian::native == std::endian::big ? std::endian::big : std::endian::little;
constexpr ::std::endian NotSystemEndian = std::endian::native == std::endian::big ? std::endian::little : std::endian::big;
} // namespace athena::utility
