#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "athena/IStream.hpp"

// ---------------------------------------------------------------------------
// Stub YAML implementation for athena::io.
//
// The full athena YAML layer depends on libyaml.  When building without it we
// provide:
//   • YAMLNodeStyle  – style hint enum (all methods are no-ops)
//   • YAMLNode       – minimal node type with m_mapChildren
//   • YAMLDocReader  – returns default/zero values; parse() always fails
//   • YAMLDocWriter  – accepts all write calls as no-ops; finish() is no-op
//
// The RAII guards returned by enterSubRecord/enterSubVector:
//   • Reader guards  convert to false  → YAML-reading blocks are skipped
//   • Writer guards  convert to true   → write blocks execute, but all writes
//                                        are no-ops so nothing is emitted
// ---------------------------------------------------------------------------

namespace athena::io {

// ── Style hint ───────────────────────────────────────────────────────────────
enum class YAMLNodeStyle { Default, Flow, Block };

// ── Minimal YAML node ────────────────────────────────────────────────────────
struct YAMLNode {
    std::string m_scalarString;
    std::vector<std::pair<std::string, std::shared_ptr<YAMLNode>>> m_mapChildren;
};

// ── RAII guard helpers ────────────────────────────────────────────────────────
namespace detail {

struct ReaderSubGuard {
    explicit ReaderSubGuard(bool valid) : m_valid(valid) {}
    explicit operator bool() const noexcept { return m_valid; }
    void leave() noexcept { m_valid = false; }
private:
    bool m_valid;
};

struct WriterSubGuard {
    explicit WriterSubGuard(bool valid) : m_valid(valid) {}
    explicit operator bool() const noexcept { return m_valid; }
    void leave() noexcept {}
private:
    bool m_valid;
};

} // namespace detail

// ── YAMLDocReader ─────────────────────────────────────────────────────────────
class YAMLDocReader {
    YAMLNode m_dummy;
public:
    YAMLDocReader() = default;

    // Always returns false – caller skips YAML processing.
    bool parse(IStreamReader* /*r*/) { return false; }

    // Scalar reads – all return default/zero values.
    std::string   readString(std::string_view /*key*/ = {}) { return {}; }
    uint8_t       readUint8 (std::string_view /*key*/ = {}) { return 0; }
    uint16_t      readUint16(std::string_view /*key*/ = {}) { return 0; }
    uint32_t      readUint32(std::string_view /*key*/ = {}) { return 0; }
    uint64_t      readUint64(std::string_view /*key*/ = {}) { return 0; }
    int8_t        readInt8  (std::string_view /*key*/ = {}) { return 0; }
    int16_t       readInt16 (std::string_view /*key*/ = {}) { return 0; }
    int32_t       readInt32 (std::string_view /*key*/ = {}) { return 0; }
    int64_t       readInt64 (std::string_view /*key*/ = {}) { return 0; }
    float         readFloat (std::string_view /*key*/ = {}) { return 0.0f; }
    double        readDouble(std::string_view /*key*/ = {}) { return 0.0; }
    bool          readBool  (std::string_view /*key*/ = {}) { return false; }

    // Vector/array helpers used via the enumerate() pattern.
    template <typename T>
    void enumerate(std::string_view /*key*/, std::vector<T>& /*vec*/) {}
    template <typename T>
    void enumerate(std::vector<T>& /*vec*/) {}

    // Sub-record guard – converts to false so YAML blocks are skipped.
    detail::ReaderSubGuard enterSubRecord(std::string_view /*key*/ = {}) {
        return detail::ReaderSubGuard{false};
    }

    // Sub-vector guard – sets sizeOut to 0 so loops do not execute.
    detail::ReaderSubGuard enterSubVector(std::string_view /*key*/, size_t& sizeOut) {
        sizeOut = 0;
        return detail::ReaderSubGuard{false};
    }

    // Returns pointer to a dummy (empty) node – m_mapChildren is empty.
    YAMLNode* getCurNode() { return &m_dummy; }

    // Returns pointer to the root node (same as the dummy in the stub).
    YAMLNode* getRootNode() { return &m_dummy; }
};

// ── YAMLDocWriter ─────────────────────────────────────────────────────────────
class YAMLDocWriter {
public:
    explicit YAMLDocWriter(std::string_view /*type*/ = {}) {}

    void setStyle(YAMLNodeStyle /*style*/) {}

    // All scalar writes are no-ops.
    void writeString(std::string_view /*key*/, std::string_view /*val*/) {}
    void writeString(std::string_view /*val*/)                           {}

    void writeUint8 (std::string_view /*key*/, uint8_t  /*val*/) {}
    void writeUint8 (uint8_t  /*val*/)                           {}
    void writeUint16(std::string_view /*key*/, uint16_t /*val*/) {}
    void writeUint16(uint16_t /*val*/)                           {}
    void writeUint32(std::string_view /*key*/, uint32_t /*val*/) {}
    void writeUint32(uint32_t /*val*/)                           {}
    void writeUint64(std::string_view /*key*/, uint64_t /*val*/) {}
    void writeUint64(uint64_t /*val*/)                           {}

    void writeInt8 (std::string_view /*key*/, int8_t  /*val*/) {}
    void writeInt8 (int8_t  /*val*/)                           {}
    void writeInt16(std::string_view /*key*/, int16_t /*val*/) {}
    void writeInt16(int16_t /*val*/)                           {}
    void writeInt32(std::string_view /*key*/, int32_t /*val*/) {}
    void writeInt32(int32_t /*val*/)                           {}
    void writeInt64(std::string_view /*key*/, int64_t /*val*/) {}
    void writeInt64(int64_t /*val*/)                           {}

    void writeFloat (std::string_view /*key*/, float  /*val*/) {}
    void writeFloat (float  /*val*/)                           {}
    void writeDouble(std::string_view /*key*/, double /*val*/) {}
    void writeDouble(double /*val*/)                           {}

    void writeBool(std::string_view /*key*/, bool /*val*/) {}
    void writeBool(bool /*val*/)                           {}

    // Vector/array helpers used via the enumerate() pattern.
    template <typename T>
    void enumerate(std::string_view /*key*/, const std::vector<T>& /*vec*/) {}
    template <typename T>
    void enumerate(const std::vector<T>& /*vec*/) {}

    // Sub-record guard – converts to true so writer lambdas execute (all
    // writes inside are no-ops).
    detail::WriterSubGuard enterSubRecord(std::string_view /*key*/ = {}) {
        return detail::WriterSubGuard{true};
    }

    // Sub-vector guard.
    detail::WriterSubGuard enterSubVector(std::string_view /*key*/ = {}) {
        return detail::WriterSubGuard{true};
    }

    // Flush to a stream writer – no-op in the stub.
    void finish(IStreamWriter* /*w*/) {}
};

} // namespace athena::io
