#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ── Low-level byte-swap helpers ───────────────────────────────────────────────
namespace amuse::io::detail {

inline uint16_t bswap16(uint16_t v) noexcept {
    return static_cast<uint16_t>((v >> 8u) | (v << 8u));
}
inline uint32_t bswap32(uint32_t v) noexcept {
    return ((v & 0xFF000000u) >> 24u) | ((v & 0x00FF0000u) >> 8u)
         | ((v & 0x0000FF00u) <<  8u) | ((v & 0x000000FFu) << 24u);
}
inline uint64_t bswap64(uint64_t v) noexcept {
    return ((v & UINT64_C(0xFF00000000000000)) >> 56u)
         | ((v & UINT64_C(0x00FF000000000000)) >> 40u)
         | ((v & UINT64_C(0x0000FF0000000000)) >> 24u)
         | ((v & UINT64_C(0x000000FF00000000)) >>  8u)
         | ((v & UINT64_C(0x00000000FF000000)) <<  8u)
         | ((v & UINT64_C(0x0000000000FF0000)) << 24u)
         | ((v & UINT64_C(0x000000000000FF00)) << 40u)
         | ((v & UINT64_C(0x00000000000000FF)) << 56u);
}

template <typename T>
inline T bswap(T val) noexcept {
    static_assert(std::is_arithmetic_v<T>, "bswap requires arithmetic type");
    if constexpr (sizeof(T) == 1) {
        return val;
    } else if constexpr (sizeof(T) == 2) {
        uint16_t tmp; std::memcpy(&tmp, &val, 2);
        tmp = bswap16(tmp);
        std::memcpy(&val, &tmp, 2); return val;
    } else if constexpr (sizeof(T) == 4) {
        uint32_t tmp; std::memcpy(&tmp, &val, 4);
        tmp = bswap32(tmp);
        std::memcpy(&val, &tmp, 4); return val;
    } else if constexpr (sizeof(T) == 8) {
        uint64_t tmp; std::memcpy(&tmp, &val, 8);
        tmp = bswap64(tmp);
        std::memcpy(&val, &tmp, 8); return val;
    }
    return val;
}

} // namespace amuse::io::detail

// ── SBig / SLittle ───────────────────────────────────────────────────────────
// SBig(x)    – swap bytes if the native platform is little-endian
// SLittle(x) – swap bytes if the native platform is big-endian

template <typename T>
inline T SBig(T v) noexcept {
    if constexpr (std::endian::native == std::endian::little)
        return amuse::io::detail::bswap(v);
    else
        return v;
}

template <typename T>
inline T SLittle(T v) noexcept {
    if constexpr (std::endian::native == std::endian::big)
        return amuse::io::detail::bswap(v);
    else
        return v;
}

// ── SBIG ─────────────────────────────────────────────────────────────────────
#define SBIG(q) static_cast<uint32_t>(                              \
    ((static_cast<uint32_t>(q) & 0x000000FFu) << 24u) |            \
    ((static_cast<uint32_t>(q) & 0x0000FF00u) <<  8u) |            \
    ((static_cast<uint32_t>(q) & 0x00FF0000u) >>  8u) |            \
    ((static_cast<uint32_t>(q) & 0xFF000000u) >> 24u))

// ── ROUND_UP_32 ───────────────────────────────────────────────────────────────
#define ROUND_UP_32(x) (((x) + 31u) & ~31u)

// ── Endian-aware stream I/O functions ─────────────────────────────────────────
namespace amuse::io {

// ── Bulk read/write ──────────────────────────────────────────────────────
inline void readBytes(std::istream& is, void* buf, size_t len) {
    is.read(static_cast<char*>(buf), static_cast<std::streamsize>(len));
}
inline void writeBytes(std::ostream& os, const void* buf, size_t len) {
    os.write(static_cast<const char*>(buf), static_cast<std::streamsize>(len));
}

// ── Heap-allocated bulk read ─────────────────────────────────────────────
inline std::unique_ptr<uint8_t[]> readUBytes(std::istream& is, size_t len) {
    auto buf = std::make_unique<uint8_t[]>(len);
    is.read(reinterpret_cast<char*>(buf.get()), static_cast<std::streamsize>(len));
    return buf;
}

// ── Single-byte scalars ──────────────────────────────────────────────────
inline uint8_t  readUByte(std::istream& is)  { uint8_t  v = 0; is.read(reinterpret_cast<char*>(&v), 1); return v; }
inline int8_t   readByte(std::istream& is)   { int8_t   v = 0; is.read(reinterpret_cast<char*>(&v), 1); return v; }
inline bool     readBool(std::istream& is)   { return readUByte(is) != 0; }

inline void writeUByte(std::ostream& os, uint8_t val)   { os.write(reinterpret_cast<const char*>(&val), 1); }
inline void writeByte(std::ostream& os, int8_t val)     { os.write(reinterpret_cast<const char*>(&val), 1); }
inline void writeBool(std::ostream& os, bool val)       { uint8_t b = val ? 1 : 0; os.write(reinterpret_cast<const char*>(&b), 1); }

// ── Big-endian read ──────────────────────────────────────────────────────
inline uint16_t readUint16Big(std::istream& is)    { uint16_t v = 0; is.read(reinterpret_cast<char*>(&v), 2); return SBig(v); }
inline int16_t  readInt16Big(std::istream& is)     { uint16_t v = 0; is.read(reinterpret_cast<char*>(&v), 2); v = SBig(v);    int16_t r; std::memcpy(&r, &v, 2); return r; }
inline uint32_t readUint32Big(std::istream& is)    { uint32_t v = 0; is.read(reinterpret_cast<char*>(&v), 4); return SBig(v); }
inline int32_t  readInt32Big(std::istream& is)     { uint32_t v = 0; is.read(reinterpret_cast<char*>(&v), 4); v = SBig(v);    int32_t r; std::memcpy(&r, &v, 4); return r; }
inline uint64_t readUint64Big(std::istream& is)    { uint64_t v = 0; is.read(reinterpret_cast<char*>(&v), 8); return SBig(v); }
inline int64_t  readInt64Big(std::istream& is)     { uint64_t v = 0; is.read(reinterpret_cast<char*>(&v), 8); v = SBig(v);    int64_t r; std::memcpy(&r, &v, 8); return r; }

inline float  readFloatBig(std::istream& is) {
    uint32_t v = 0; is.read(reinterpret_cast<char*>(&v), 4); v = SBig(v);
    float r; std::memcpy(&r, &v, 4); return r;
}
inline double readDoubleBig(std::istream& is) {
    uint64_t v = 0; is.read(reinterpret_cast<char*>(&v), 8); v = SBig(v);
    double r; std::memcpy(&r, &v, 8); return r;
}

// ── Big-endian write ─────────────────────────────────────────────────────
inline void writeUint16Big(std::ostream& os, uint16_t val)    { val = SBig(val);    os.write(reinterpret_cast<const char*>(&val), 2); }
inline void writeInt16Big(std::ostream& os, int16_t val)      { uint16_t v; std::memcpy(&v, &val, 2); v = SBig(v);    os.write(reinterpret_cast<const char*>(&v), 2); }
inline void writeUint32Big(std::ostream& os, uint32_t val)    { val = SBig(val);    os.write(reinterpret_cast<const char*>(&val), 4); }
inline void writeInt32Big(std::ostream& os, int32_t val)      { uint32_t v; std::memcpy(&v, &val, 4); v = SBig(v);    os.write(reinterpret_cast<const char*>(&v), 4); }
inline void writeUint64Big(std::ostream& os, uint64_t val)    { val = SBig(val);    os.write(reinterpret_cast<const char*>(&val), 8); }
inline void writeInt64Big(std::ostream& os, int64_t val)      { uint64_t v; std::memcpy(&v, &val, 8); v = SBig(v);    os.write(reinterpret_cast<const char*>(&v), 8); }

inline void writeFloatBig(std::ostream& os, float val)   { uint32_t v; std::memcpy(&v, &val, 4); v = SBig(v); os.write(reinterpret_cast<const char*>(&v), 4); }
inline void writeDoubleBig(std::ostream& os, double val) { uint64_t v; std::memcpy(&v, &val, 8); v = SBig(v); os.write(reinterpret_cast<const char*>(&v), 8); }

// ── Little-endian read ───────────────────────────────────────────────────
inline uint16_t readUint16Little(std::istream& is)    { uint16_t v = 0; is.read(reinterpret_cast<char*>(&v), 2); return SLittle(v); }
inline int16_t  readInt16Little(std::istream& is)     { uint16_t v = 0; is.read(reinterpret_cast<char*>(&v), 2); v = SLittle(v); int16_t r; std::memcpy(&r, &v, 2); return r; }
inline uint32_t readUint32Little(std::istream& is)    { uint32_t v = 0; is.read(reinterpret_cast<char*>(&v), 4); return SLittle(v); }
inline int32_t  readInt32Little(std::istream& is)     { uint32_t v = 0; is.read(reinterpret_cast<char*>(&v), 4); v = SLittle(v); int32_t r; std::memcpy(&r, &v, 4); return r; }
inline uint64_t readUint64Little(std::istream& is)    { uint64_t v = 0; is.read(reinterpret_cast<char*>(&v), 8); return SLittle(v); }
inline int64_t  readInt64Little(std::istream& is)     { uint64_t v = 0; is.read(reinterpret_cast<char*>(&v), 8); v = SLittle(v); int64_t r; std::memcpy(&r, &v, 8); return r; }

// ── Little-endian write ──────────────────────────────────────────────────
inline void writeUint16Little(std::ostream& os, uint16_t val)    { val = SLittle(val); os.write(reinterpret_cast<const char*>(&val), 2); }
inline void writeInt16Little(std::ostream& os, int16_t val)      { uint16_t v; std::memcpy(&v, &val, 2); v = SLittle(v); os.write(reinterpret_cast<const char*>(&v), 2); }
inline void writeUint32Little(std::ostream& os, uint32_t val)    { val = SLittle(val); os.write(reinterpret_cast<const char*>(&val), 4); }
inline void writeInt32Little(std::ostream& os, int32_t val)      { uint32_t v; std::memcpy(&v, &val, 4); v = SLittle(v); os.write(reinterpret_cast<const char*>(&v), 4); }
inline void writeUint64Little(std::ostream& os, uint64_t val)    { val = SLittle(val); os.write(reinterpret_cast<const char*>(&val), 8); }
inline void writeInt64Little(std::ostream& os, int64_t val)      { uint64_t v; std::memcpy(&v, &val, 8); v = SLittle(v); os.write(reinterpret_cast<const char*>(&v), 8); }

inline void writeFloatLittle(std::ostream& os, float val)   { uint32_t v; std::memcpy(&v, &val, 4); v = SLittle(v); os.write(reinterpret_cast<const char*>(&v), 4); }
inline void writeDoubleLittle(std::ostream& os, double val) { uint64_t v; std::memcpy(&v, &val, 8); v = SLittle(v); os.write(reinterpret_cast<const char*>(&v), 8); }

// ── Endian-generic read/write helpers ─────────────────────────────────────
// ReadVal<T, DNAE>({}, val, stream) — reads sizeof(T) bytes and byte-swaps
// according to DNAE (std::endian::big or std::endian::little).
// WriteVal<T, DNAE>({}, val, stream) — byte-swaps and writes sizeof(T) bytes.
// The first argument is an unused tag for template deduction convenience.
template <typename T, std::endian DNAE>
inline void ReadVal(T, T& val, std::istream& is) {
    static_assert(std::is_arithmetic_v<T>, "ReadVal requires arithmetic type");
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
    if constexpr (DNAE == std::endian::big)
        val = SBig(val);
    else
        val = SLittle(val);
}

template <typename T, std::endian DNAE>
inline void WriteVal(T, T valIn, std::ostream& os) {
    static_assert(std::is_arithmetic_v<std::remove_const_t<T>>, "WriteVal requires arithmetic type");
    auto val = valIn;
    if constexpr (DNAE == std::endian::big)
        val = SBig(val);
    else
        val = SLittle(val);
    os.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

// ── Alignment ────────────────────────────────────────────────────────────
inline void seekAlign32(std::ostream& os) {
    auto pos = os.tellp();
    auto aligned = static_cast<std::streamoff>((static_cast<uint64_t>(pos) + 31u) & ~31u);
    if (aligned > pos) {
        static const char zeros[32]{};
        os.write(zeros, aligned - pos);
    }
}

// ── MemoryInputStream ────────────────────────────────────────────────────
// Wraps a const byte buffer as a std::istream (seekable, read-only).
class MemoryInputStream : public std::istream {
    struct MemBuf : std::streambuf {
        MemBuf(const void* data, size_t len) {
            char* p = const_cast<char*>(static_cast<const char*>(data));
            setg(p, p, p + len);
        }
    protected:
        pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                         std::ios_base::openmode = std::ios_base::in) override {
            switch (dir) {
            case std::ios_base::beg: setg(eback(), eback() + off, egptr()); break;
            case std::ios_base::cur: setg(eback(), gptr() + off, egptr()); break;
            case std::ios_base::end: setg(eback(), egptr() + off, egptr()); break;
            default: break;
            }
            if (gptr() < eback() || gptr() > egptr())
                return pos_type(off_type(-1));
            return pos_type(gptr() - eback());
        }
        pos_type seekpos(pos_type pos, std::ios_base::openmode mode = std::ios_base::in) override {
            return seekoff(off_type(pos), std::ios_base::beg, mode);
        }
    };
    MemBuf m_buf;
public:
    MemoryInputStream(const void* data, size_t len)
        : std::istream(&m_buf), m_buf(data, len) {}
};

// ── VectorOutputStream ───────────────────────────────────────────────────
// Wraps a std::vector<uint8_t> as a std::ostream (seekable, write-only).
class VectorOutputStream : public std::ostream {
    struct VecBuf : std::streambuf {
        std::vector<uint8_t>& m_vec;
        size_t m_pos = 0;

        explicit VecBuf(std::vector<uint8_t>& vec) : m_vec(vec) {}
    protected:
        std::streamsize xsputn(const char_type* s, std::streamsize n) override {
            size_t end = m_pos + static_cast<size_t>(n);
            if (end > m_vec.size())
                m_vec.resize(end);
            std::memcpy(m_vec.data() + m_pos, s, static_cast<size_t>(n));
            m_pos += static_cast<size_t>(n);
            return n;
        }
        int_type overflow(int_type ch) override {
            if (ch != traits_type::eof()) {
                if (m_pos >= m_vec.size())
                    m_vec.push_back(static_cast<uint8_t>(ch));
                else
                    m_vec[m_pos] = static_cast<uint8_t>(ch);
                ++m_pos;
            }
            return ch;
        }
        pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                         std::ios_base::openmode = std::ios_base::out) override {
            switch (dir) {
            case std::ios_base::beg: m_pos = static_cast<size_t>(off); break;
            case std::ios_base::cur: m_pos = static_cast<size_t>(static_cast<std::streamoff>(m_pos) + off); break;
            case std::ios_base::end: m_pos = static_cast<size_t>(static_cast<std::streamoff>(m_vec.size()) + off); break;
            default: break;
            }
            return pos_type(off_type(m_pos));
        }
        pos_type seekpos(pos_type pos, std::ios_base::openmode mode = std::ios_base::out) override {
            return seekoff(off_type(pos), std::ios_base::beg, mode);
        }
    };
    std::vector<uint8_t> m_vec;
    VecBuf m_buf;
public:
    VectorOutputStream() : std::ostream(&m_buf), m_buf(m_vec) {}
    const std::vector<uint8_t>& data() const noexcept { return m_vec; }
    std::vector<uint8_t>& data() noexcept { return m_vec; }
};

} // namespace amuse::io
