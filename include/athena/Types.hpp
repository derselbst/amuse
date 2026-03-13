#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <bit>

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
//   Value<uint32_t> riffMagic = SBIG('RIFF');
#define FOURCC(_a, _b, _c, _d) \
    (std::endian::native == std::endian::big) ? \
    (uint32_t)(((uint32_t)(_a) << 24) | ((uint32_t)(_b) << 16) | ((uint32_t)(_c) << 8) | (uint32_t)(_d)) : \
    (uint32_t)(((uint32_t)(_d) << 24) | ((uint32_t)(_c) << 16) | ((uint32_t)(_b) << 8) | (uint32_t)(_a))

// ── ROUND_UP_32 ───────────────────────────────────────────────────────────────
// Round x up to the nearest multiple of 32.
#define ROUND_UP_32(x) (((x) + 31u) & ~31u)
