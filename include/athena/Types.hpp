#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <bit>

// ── Primitive type aliases ────────────────────────────────────────────────────
using atUint8  = uint8_t;
using atUint16 = uint16_t;
using atUint32 = uint32_t;
using atUint64 = uint64_t;
using atInt8   = int8_t;
using atInt16  = int16_t;
using atInt32  = int32_t;
using atInt64  = int64_t;
using atFloat  = float;
using atDouble = double;

// ── SBig / SLittle ───────────────────────────────────────────────────────────
// SBig(x)    – swap bytes if the native platform is little-endian
// SLittle(x) – swap bytes if the native platform is big-endian

template <typename T>
constexpr inline T SBig(T v) noexcept {
    if constexpr(std::endian::native == std::endian::little)
    {
        return std::byteswap(v);
    }
    else
    {
        return v;
    }
}

template <typename T>
inline T SLittle(T v) noexcept {
    if constexpr(std::endian::native == std::endian::big)
    {
        return std::byteswap(v);
    }
    else
    {
        return v;
    }
}

// ── SBIG ─────────────────────────────────────────────────────────────────────
// Unconditionally byte-swaps a 4-char integer literal so that the in-memory
// representation (on any platform) stores the characters in big-endian order.
// Used to initialise magic-number fields in little-endian structs, e.g.:
//   Value<atUint32> riffMagic = SBIG('RIFF');
#define SBIG(q) static_cast<uint32_t>(                              \
    ((static_cast<uint32_t>(q) & 0x000000FFu) << 24u) |            \
    ((static_cast<uint32_t>(q) & 0x0000FF00u) <<  8u) |            \
    ((static_cast<uint32_t>(q) & 0x00FF0000u) >>  8u) |            \
    ((static_cast<uint32_t>(q) & 0xFF000000u) >> 24u))

// ── ROUND_UP_32 ───────────────────────────────────────────────────────────────
// Round x up to the nearest multiple of 32.
#define ROUND_UP_32(x) (((x) + 31u) & ~31u)
