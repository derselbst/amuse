#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "athena/Types.hpp"
#include "athena/IStream.hpp"
#include "athena/DNAYaml.hpp"

// =============================================================================
// athena DNA serialization framework – compatibility shim
//
// This header replicates the public API of the original athena DNA system
// without depending on the real athena library or the atdna code-generation
// tool.  It provides:
//
//   • athena::io::DNA<E>       – base for endian-tagged binary structs
//   • athena::io::DNAVYaml<E>  – adds polymorphic (virtual) YAML read/write
//   • Value<T, E>              – endian-aware scalar/enum field wrapper
//   • Seek<N, Origin>          – zero-size stream seek marker (used by atdna)
//   • Read<PropType> / Write<PropType>
//                              – low-level dispatch helpers
//   • AT_DECL_DNA / AT_DECL_DNA_YAML / AT_DECL_DNA_YAMLV
//     AT_DECL_EXPLICIT_DNA_YAML / AT_DECL_EXPLICIT_DNA_YAMLV
//   • AT_SPECIALIZE_PARMS(...)
//
// Enumerate<Op> bodies are declared but NOT defined here; they are provided
// by either hand-written .cpp files (e.g. Common.cpp) or by atdna-generated
// translation units.
// =============================================================================

namespace athena::io {

// ── Shared operation tag types ────────────────────────────────────────────────
// All DNA<E> specialisations alias the SAME op-tag types so that
// BigDNA::Read == LittleDNA::Read.  This means a single Enumerate<BigDNA::Read>
// specialisation is found regardless of whether the struct inherits from
// BigDNA or LittleDNA.
struct DNAOpRead        { using io_type = athena::io::IStreamReader; };
struct DNAOpWrite       { using io_type = athena::io::IStreamWriter; };
struct DNAOpBinarySize  { using io_type = size_t;                    };
struct DNAOpReadYaml    { using io_type = athena::io::YAMLDocReader; };
struct DNAOpWriteYaml   { using io_type = athena::io::YAMLDocWriter; };

// ── Operation tag types ───────────────────────────────────────────────────────
template <athena::Endian E>
struct DNA {
    using Read        = DNAOpRead;
    using Write       = DNAOpWrite;
    using BinarySize  = DNAOpBinarySize;
    using ReadYaml    = DNAOpReadYaml;
    using WriteYaml   = DNAOpWriteYaml;
};

// ── DNAVYaml – adds virtual binary + YAML read/write ─────────────────────────
template <athena::Endian E>
struct DNAVYaml : DNA<E> {
    virtual void read (athena::io::IStreamReader& r)        = 0;
    virtual void write(athena::io::IStreamWriter& w) const  = 0;
    virtual void read (athena::io::YAMLDocReader& r)        = 0;
    virtual void write(athena::io::YAMLDocWriter& w) const  = 0;
    virtual ~DNAVYaml() = default;
};

// ── PropType / Read<P>::Do / Write<P>::Do ─────────────────────────────────────
// Used for ad-hoc endian-aware I/O outside of Enumerate:
//   athena::io::Read<PropType::None>::Do<uint32_t, Endian::Big>({}, val, r);

enum class PropType { None };

template <PropType P>
struct Read {
    // Scalar types
    template <typename T, athena::Endian E,
              std::enable_if_t<!std::is_array_v<T>, int> = 0>
    static void Do(std::string_view /*key*/, T& val, athena::io::IStreamReader& r) {
        if constexpr (std::is_same_v<T, bool>) {
            val = r.readBool();
        } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>) {
            val = static_cast<T>(r.readUByte());
        } else if constexpr (sizeof(T) == 2) {
            if constexpr (E == athena::Endian::Big) {
                uint16_t v = r.readUint16Big();   std::memcpy(&val, &v, 2);
            } else {
                uint16_t v = r.readUint16Little(); std::memcpy(&val, &v, 2);
            }
        } else if constexpr (sizeof(T) == 4) {
            if constexpr (E == athena::Endian::Big) {
                uint32_t v = r.readUint32Big();   std::memcpy(&val, &v, 4);
            } else {
                uint32_t v = r.readUint32Little(); std::memcpy(&val, &v, 4);
            }
        } else if constexpr (sizeof(T) == 8) {
            if constexpr (E == athena::Endian::Big) {
                uint64_t v = r.readUint64Big();   std::memcpy(&val, &v, 8);
            } else {
                uint64_t v = r.readUint64Little(); std::memcpy(&val, &v, 8);
            }
        }
    }

    // Array types – read each element with endian correction
    template <typename T, athena::Endian E,
              std::enable_if_t<std::is_array_v<T>, int> = 0>
    static void Do(std::string_view key, T& val, athena::io::IStreamReader& r) {
        using Elem = std::remove_all_extents_t<T>;
        constexpr size_t N = sizeof(T) / sizeof(Elem);
        for (size_t i = 0; i < N; ++i)
            Do<Elem, E>(key, val[i], r);
    }
};

template <PropType P>
struct Write {
    // Scalar types
    template <typename T, athena::Endian E,
              std::enable_if_t<!std::is_array_v<T>, int> = 0>
    static void Do(std::string_view /*key*/, const T& val, athena::io::IStreamWriter& w) {
        if constexpr (std::is_same_v<T, bool>) {
            w.writeBool(val);
        } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>) {
            w.writeUByte(static_cast<uint8_t>(val));
        } else if constexpr (sizeof(T) == 2) {
            uint16_t v; std::memcpy(&v, &val, 2);
            if constexpr (E == athena::Endian::Big) w.writeUint16Big(v);
            else                                     w.writeUint16Little(v);
        } else if constexpr (sizeof(T) == 4) {
            uint32_t v; std::memcpy(&v, &val, 4);
            if constexpr (E == athena::Endian::Big) w.writeUint32Big(v);
            else                                     w.writeUint32Little(v);
        } else if constexpr (sizeof(T) == 8) {
            uint64_t v; std::memcpy(&v, &val, 8);
            if constexpr (E == athena::Endian::Big) w.writeUint64Big(v);
            else                                     w.writeUint64Little(v);
        }
    }

    // Array types – write each element with endian correction
    template <typename T, athena::Endian E,
              std::enable_if_t<std::is_array_v<T>, int> = 0>
    static void Do(std::string_view key, const T& val, athena::io::IStreamWriter& w) {
        using Elem = std::remove_all_extents_t<T>;
        constexpr size_t N = sizeof(T) / sizeof(Elem);
        for (size_t i = 0; i < N; ++i)
            Do<Elem, E>(key, val[i], w);
    }
};

// ── Value<T, E> ───────────────────────────────────────────────────────────────
// A transparent alias for T.  The endianness tag E is retained in the
// template signature so that existing declarations (Value<uint32_t>,
// Value<Combine>, etc.) continue to compile unchanged, but at runtime
// Value<T,E> IS T — no wrapper, no implicit-conversion friction.
// Endian-aware I/O is handled explicitly in the Enumerate<Op> bodies in the
// atdna_*.cpp files.

template <typename T, athena::Endian E = athena::Endian::Big>
using Value = T;

// ── Seek<N, Origin> ───────────────────────────────────────────────────────────
// Zero-size field marker.  atdna translates it into a seek() call inside
// the generated Enumerate specialisations.

template <int64_t N, athena::SeekOrigin Origin>
struct Seek {};

} // namespace athena::io

// =============================================================================
// AT_SPECIALIZE_PARMS  – annotation consumed by atdna; ignored here.
// =============================================================================
#define AT_SPECIALIZE_PARMS(...) /* consumed by atdna */

// =============================================================================
// Internal helper: const_cast away constness to call non-const Enumerate
// from const write/binarySize methods.
// =============================================================================
#define _AT_MUTABLE_THIS \
    const_cast<std::remove_cv_t<std::remove_reference_t<decltype(*this)>>*>(this)

// =============================================================================
// AT_DECL_DNA
//
// Declares binary-only serialisation API on a struct that inherits from
// BigDNA / LittleDNA.
//
// read(), write(), binarySize() are intentionally declared as member function
// TEMPLATES (with a dummy unused type parameter).  This defers their
// instantiation for non-template derived classes, avoiding the
// "specialization after instantiation" error that would otherwise occur when
// a separate translation unit (e.g. an atdna-generated file) both includes
// the header and provides explicit Enumerate<Op> specialisations.
//
// Callers use them identically to non-template functions:
//   obj.read(r);   obj.write(w);   obj.binarySize();
// =============================================================================
#define AT_DECL_DNA                                                             \
    template <class Op>                                                         \
    void Enumerate(typename Op::io_type& io);                                  \
                                                                                \
    template <class _ImplDummy = void>                                         \
    void read(athena::io::IStreamReader& r) {                                   \
        Enumerate<athena::io::DNAOpRead>(r);                                   \
    }                                                                           \
    template <class _ImplDummy = void>                                         \
    void write(athena::io::IStreamWriter& w) const {                            \
        _AT_MUTABLE_THIS->template Enumerate<athena::io::DNAOpWrite>(w);       \
    }                                                                           \
    template <class _ImplDummy = void>                                         \
    size_t binarySize(size_t sz = 0) const {                                   \
        _AT_MUTABLE_THIS->template Enumerate<athena::io::DNAOpBinarySize>(sz); \
        return sz;                                                              \
    }                                                                           \
    static std::string_view DNAType();

// =============================================================================
// AT_DECL_DNA_YAML
//
// Extends AT_DECL_DNA with non-virtual YAML read/write.
// Same deferred-instantiation template trick is used for YAML methods.
// =============================================================================
#define AT_DECL_DNA_YAML                                                        \
    AT_DECL_DNA                                                                 \
                                                                                \
    template <class _ImplDummy = void>                                         \
    void read(athena::io::YAMLDocReader& r) {                                   \
        Enumerate<athena::io::DNAOpReadYaml>(r);                               \
    }                                                                           \
    template <class _ImplDummy = void>                                         \
    void write(athena::io::YAMLDocWriter& w) const {                            \
        _AT_MUTABLE_THIS->template Enumerate<athena::io::DNAOpWriteYaml>(w);   \
    }

// =============================================================================
// AT_DECL_DNA_YAMLV
//
// Declares virtual binary + YAML read/write for polymorphic types
// (e.g. SoundMacro::ICmd and its sub-commands).
//
// Virtual functions cannot be templates, so the methods are DECLARED ONLY
// here.  Their definitions (which call Enumerate<Op>) must be provided
// externally – either by atdna-generated translation units or by hand-written
// .cpp files.  This avoids the "specialization after instantiation" diagnostic
// that GCC/Clang emit for non-template classes with inline bodies that call
// member function templates, when explicit specialisations are provided in the
// same translation unit.
// =============================================================================
#define AT_DECL_DNA_YAMLV                                                       \
    template <class Op>                                                         \
    void Enumerate(typename Op::io_type& io);                                  \
                                                                                \
    void read(athena::io::IStreamReader& r) override;                           \
    void write(athena::io::IStreamWriter& w) const override;                    \
    size_t binarySize(size_t sz = 0) const;                                    \
    virtual void read(athena::io::YAMLDocReader& r);                            \
    virtual void write(athena::io::YAMLDocWriter& w) const;                     \
    static std::string_view DNAType();

// =============================================================================
// AT_DECL_EXPLICIT_DNA_YAML
//
// Same expansion as AT_DECL_DNA_YAML (uses the deferred-template trick).
// The "explicit" convention signals that the developer provides the
// Enumerate<ReadYaml>/Enumerate<WriteYaml> bodies manually (typically
// delegating to _read()/_write() helpers in a .cpp).
// =============================================================================
#define AT_DECL_EXPLICIT_DNA_YAML  AT_DECL_DNA_YAML

// =============================================================================
// AT_DECL_EXPLICIT_DNA_YAMLV
//
// Same expansion as AT_DECL_DNA_YAMLV (virtual, declare-only).
// =============================================================================
#define AT_DECL_EXPLICIT_DNA_YAMLV  AT_DECL_DNA_YAMLV
