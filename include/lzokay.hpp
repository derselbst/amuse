#pragma once

// lzokay compatibility shim – wraps liblzo2 to provide the lzokay API used
// by ContainerRegistry.cpp.  The real lzokay library is a header-only pure-C++
// LZO1X decompressor; this shim replicates its public interface on top of the
// widely-available liblzo2 system library.

#include <cstddef>
#include <cstdint>
#include <mutex>

#include <lzo/lzo1x.h>

namespace lzokay {

enum class EResult {
    Success          =  0,
    Error            = -1,
    InputOverrun     =  4,
    OutputOverrun    =  5,
    LookbehindOverrun=  6,
    EOFNotFound      =  7,
    InputNotConsumed =  8,
};

// Decompress LZO1X-compressed data.
// src          – pointer to compressed input bytes
// src_size     – number of compressed input bytes
// dst          – pointer to the output buffer
// dst_capacity – capacity of the output buffer in bytes
// dst_size     – receives the number of bytes written to dst
inline EResult decompress(const uint8_t* src, size_t src_size,
                           uint8_t* dst, size_t dst_capacity,
                           size_t& dst_size) {
    // lzo_init() must be called once before any LZO function.  Use
    // call_once to guarantee safe initialization in multi-threaded code.
    static std::once_flag s_init_flag;
    std::call_once(s_init_flag, []{ lzo_init(); });

    lzo_uint out_len = static_cast<lzo_uint>(dst_capacity);
    int r = lzo1x_decompress_safe(
        reinterpret_cast<const lzo_bytep>(src), static_cast<lzo_uint>(src_size),
        reinterpret_cast<lzo_bytep>(dst), &out_len,
        nullptr /* no work memory needed for decompression */);
    dst_size = static_cast<size_t>(out_len);

    // Map liblzo2 return codes to lzokay EResult values.
    switch (r) {
    case LZO_E_OK:                   return EResult::Success;
    case LZO_E_INPUT_OVERRUN:        return EResult::InputOverrun;
    case LZO_E_OUTPUT_OVERRUN:       return EResult::OutputOverrun;
    case LZO_E_LOOKBEHIND_OVERRUN:   return EResult::LookbehindOverrun;
    case LZO_E_EOF_NOT_FOUND:        return EResult::EOFNotFound;
    case LZO_E_INPUT_NOT_CONSUMED:   return EResult::InputNotConsumed;
    default:                         return EResult::Error;
    }
}

} // namespace lzokay
