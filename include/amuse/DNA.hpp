#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "amuse/DNAYaml.hpp"

// =============================================================================
// amuse DNA serialization framework
//
// Simplified replacement for the athena DNA system.  Provides:
//
//   • amuse::Endian                – Big/Little tag for data format endianness
//   • BigDNA / LittleDNA          – empty base for non-polymorphic structs
//   • BigDNAV / LittleDNAV        – virtual base for polymorphic DNA structs
//   • AT_DECL_DNA / AT_DECL_DNA_YAML / AT_DECL_DNA_YAMLV
//     AT_DECL_EXPLICIT_DNA_YAML / AT_DECL_EXPLICIT_DNA_YAMLV
//   • AT_SPECIALIZE_PARMS(...)    – no-op annotation
//   • Value<T, E>                 – transparent alias for T
//
// Serialization methods use std::istream / std::ostream directly.
// =============================================================================

namespace amuse {

enum class Endian : uint8_t { Big, Little };

// ── Non-polymorphic DNA base ──────────────────────────────────────────────────
struct BigDNA {
    virtual ~BigDNA() = default;
};
using LittleDNA = BigDNA;

// ── Polymorphic DNA base with virtual read/write ─────────────────────────────
struct BigDNAV {
    virtual void read(std::istream& is) = 0;
    virtual void write(std::ostream& os) const = 0;
    virtual void readYaml(amuse::io::YAMLDocReader& r) = 0;
    virtual void writeYaml(amuse::io::YAMLDocWriter& w) const = 0;
    virtual ~BigDNAV() = default;
};
using LittleDNAV = BigDNAV;

// ── Value<T, E> – transparent alias ──────────────────────────────────────────
template <typename T, amuse::Endian E = amuse::Endian::Big>
using Value = T;

// ── SeekOrigin ───────────────────────────────────────────────────────────────
enum class SeekOrigin : uint8_t { Begin, Current, End };

// ── Seek<N, Origin> ──────────────────────────────────────────────────────────
// Zero-size field marker used by DNA structs to represent stream seek
// operations.  The actual seek is performed in generated Enumerate bodies.
template <int64_t N, amuse::SeekOrigin Origin>
struct Seek {};

} // namespace amuse

// =============================================================================
// AT_SPECIALIZE_PARMS  – annotation consumed by atdna; ignored here.
// =============================================================================
#define AT_SPECIALIZE_PARMS(...) /* consumed by atdna */

// =============================================================================
// AT_DECL_DNA
//
// Declares binary serialization methods on a struct that inherits from
// BigDNA / LittleDNA.
// =============================================================================
#define AT_DECL_DNA                                                             \
    void read(std::istream& __is);                                             \
    void write(std::ostream& __os) const;                                      \
    size_t binarySize(size_t __sz = 0) const;                                  \
    static std::string_view DNAType();

// =============================================================================
// AT_DECL_DNA_YAML
//
// Extends AT_DECL_DNA with non-virtual YAML read/write.
// =============================================================================
#define AT_DECL_DNA_YAML                                                        \
    AT_DECL_DNA                                                                 \
    void readYaml(amuse::io::YAMLDocReader& __r);                              \
    void writeYaml(amuse::io::YAMLDocWriter& __w) const;

// =============================================================================
// AT_DECL_DNA_YAMLV
//
// Declares virtual binary + YAML read/write for polymorphic types.
// =============================================================================
#define AT_DECL_DNA_YAMLV                                                       \
    void read(std::istream& __is) override;                                    \
    void write(std::ostream& __os) const override;                             \
    size_t binarySize(size_t __sz = 0) const;                                  \
    void readYaml(amuse::io::YAMLDocReader& __r) override;                     \
    void writeYaml(amuse::io::YAMLDocWriter& __w) const override;              \
    static std::string_view DNAType();

// =============================================================================
// AT_DECL_EXPLICIT_DNA_YAML / AT_DECL_EXPLICIT_DNA_YAMLV
// =============================================================================
#define AT_DECL_EXPLICIT_DNA_YAML  AT_DECL_DNA_YAML
#define AT_DECL_EXPLICIT_DNA_YAMLV  AT_DECL_DNA_YAMLV
