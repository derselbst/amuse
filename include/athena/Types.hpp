#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

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

// ── Endianness detection ──────────────────────────────────────────────────────
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define ATHENA_IS_LITTLE_ENDIAN 0
#elif defined(_MSC_VER) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#  define ATHENA_IS_LITTLE_ENDIAN 1
#else
#  define ATHENA_IS_LITTLE_ENDIAN 1 // assume LE for modern hardware
#endif

// ── Low-level byte-swap helpers ───────────────────────────────────────────────
namespace athena::detail {

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

// Type-generic byte-swap via memcpy to avoid UB with non-trivial types.
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

} // namespace athena::detail

// ── SBig / SLittle ───────────────────────────────────────────────────────────
// SBig(x)    – swap bytes if the native platform is little-endian
// SLittle(x) – swap bytes if the native platform is big-endian

template <typename T>
inline T SBig(T v) noexcept {
#if ATHENA_IS_LITTLE_ENDIAN
    return athena::detail::bswap(v);
#else
    return v;
#endif
}

template <typename T>
inline T SLittle(T v) noexcept {
#if !ATHENA_IS_LITTLE_ENDIAN
    return athena::detail::bswap(v);
#else
    return v;
#endif
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
