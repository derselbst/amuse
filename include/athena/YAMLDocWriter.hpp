#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "athena/DNA.hpp"
#include "athena/YAMLDocCommon.hpp"

namespace athena::io {

class YAMLDocWriter {
public:
  class Scope {
    YAMLDocWriter* m_writer = nullptr;
    bool m_valid = false;

  public:
    Scope() = default;
    Scope(YAMLDocWriter* writer, bool valid) : m_writer(writer), m_valid(valid) {}
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&& other) noexcept : m_writer(other.m_writer), m_valid(other.m_valid) { other.m_writer = nullptr; }
    ~Scope() {
      if (m_writer && m_valid)
        m_writer->leave();
    }
    explicit operator bool() const { return m_valid; }
    void leave() {
      if (m_writer && m_valid) {
        m_writer->leave();
        m_valid = false;
      }
    }
  };

  YAMLDocWriter() = default;
  explicit YAMLDocWriter(std::string_view dnaType) {
    if (!dnaType.empty())
      writeString("DNAType", std::string(dnaType));
  }

  YAMLNode* getCurNode() { return m_cur; }

  Scope enterSubRecord(const char* name = nullptr) {
    if (!m_cur)
      return {};
    if (name && *name) {
      if (m_cur->type != YAMLNode::Type::Map)
        m_cur->type = YAMLNode::Type::Map;
      auto& child = m_cur->m_mapChildren[name];
      child.type = YAMLNode::Type::Map;
      m_stack.push_back(m_cur);
      m_cur = &child;
      return Scope(this, true);
    }

    if (m_cur->type != YAMLNode::Type::Sequence)
      m_cur->type = YAMLNode::Type::Sequence;
    m_cur->m_seqChildren.emplace_back();
    YAMLNode& child = m_cur->m_seqChildren.back();
    child.type = YAMLNode::Type::Map;
    m_stack.push_back(m_cur);
    m_cur = &child;
    return Scope(this, true);
  }

  Scope enterSubVector(const char* name) {
    if (!m_cur || !name || !*name)
      return {};
    if (m_cur->type != YAMLNode::Type::Map)
      m_cur->type = YAMLNode::Type::Map;
    auto& child = m_cur->m_mapChildren[name];
    child.type = YAMLNode::Type::Sequence;
    m_stack.push_back(m_cur);
    m_cur = &child;
    return Scope(this, true);
  }

  void setStyle(YAMLNodeStyle style) {
    if (m_cur)
      m_cur->style = style;
  }

  void writeString(const char* name, const std::string& value) { writeScalar(name, value); }
  void writeString(const std::string& value) { writeScalar(nullptr, value); }
  void writeUint16(uint16_t value) { writeScalar(nullptr, value); }
  void writeUint32(uint32_t value) { writeScalar(nullptr, value); }
  void writeInt16(int16_t value) { writeScalar(nullptr, value); }
  void writeInt32(int32_t value) { writeScalar(nullptr, value); }
  void writeBool(bool value) { writeScalar(nullptr, value); }
  void writeDouble(double value) { writeScalar(nullptr, value); }
  void writeFloat(float value) { writeScalar(nullptr, value); }

  template <typename T>
  void enumerate(const char* name, const T& value) {
    if constexpr (detail::IsValue<T>::value) {
      writeScalar(name, static_cast<typename T::ValueType>(value));
    } else if constexpr (detail::IsDnaV<T>) {
      if (auto scope = enterSubRecord(name)) {
        auto& ref = const_cast<T&>(value);
        ref.template Enumerate<WriteYamlOp>(*this);
      }
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
      writeScalar(name, value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      writeScalar(name, value);
    } else if constexpr (IsVector<T>::value) {
      writeVector(name, value);
    }
  }

  void finish(IStreamWriter* writer) {
    if (!writer)
      return;
    std::ostringstream out;
    RenderNode(out, m_root, 0);
    const std::string data = out.str();
    writer->writeBytes(data.data(), data.size());
  }

private:
  YAMLNode m_root;
  YAMLNode* m_cur = &m_root;
  std::vector<YAMLNode*> m_stack;

  void leave() {
    if (!m_stack.empty()) {
      m_cur = m_stack.back();
      m_stack.pop_back();
    }
  }

  template <typename T>
  struct IsVector : std::false_type {};

  template <typename T, typename Alloc>
  struct IsVector<std::vector<T, Alloc>> : std::true_type {};

  template <typename T>
  void writeScalar(const char* name, const T& value) {
    YAMLNode node;
    node.type = YAMLNode::Type::Scalar;
    node.scalar = ToString(value);
    if (name && *name) {
      if (m_cur->type != YAMLNode::Type::Map)
        m_cur->type = YAMLNode::Type::Map;
      m_cur->m_mapChildren[name] = std::move(node);
    } else if (m_cur) {
      *m_cur = std::move(node);
    }
  }

  template <typename T>
  static std::string ToString(const T& value) {
    if constexpr (std::is_same_v<T, bool>) {
      return value ? "true" : "false";
    } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
      return std::to_string(static_cast<long long>(value));
    } else if constexpr (std::is_enum_v<T>) {
      return std::to_string(static_cast<long long>(value));
    } else if constexpr (std::is_floating_point_v<T>) {
      std::ostringstream out;
      out << value;
      return out.str();
    } else {
      return QuoteString(value);
    }
  }

  static std::string QuoteString(const std::string& value) {
    if (value.find_first_of("\t\n\r :{}[],") == std::string::npos)
      return value;
    std::string out = "\"";
    for (char c : value) {
      if (c == '\"')
        out += "\\\"";
      else
        out += c;
    }
    out += "\"";
    return out;
  }

  template <typename T>
  void writeVector(const char* name, const std::vector<T>& value) {
    YAMLNode node;
    node.type = YAMLNode::Type::Sequence;
    for (const auto& entry : value) {
      YAMLNode child;
      child.type = YAMLNode::Type::Scalar;
      child.scalar = ToString(entry);
      node.m_seqChildren.push_back(std::move(child));
    }
    if (name && *name) {
      if (m_cur->type != YAMLNode::Type::Map)
        m_cur->type = YAMLNode::Type::Map;
      m_cur->m_mapChildren[name] = std::move(node);
    }
  }

  static void RenderNode(std::ostringstream& out, const YAMLNode& node, int indent) {
    const std::string pad(indent, ' ');
    switch (node.type) {
    case YAMLNode::Type::Scalar:
      out << pad << node.scalar << "\n";
      break;
    case YAMLNode::Type::Sequence:
      for (const auto& entry : node.m_seqChildren) {
        if (entry.type == YAMLNode::Type::Scalar) {
          out << pad << "- " << entry.scalar << "\n";
        } else if (entry.type == YAMLNode::Type::Map && entry.style == YAMLNodeStyle::Flow) {
          out << pad << "- ";
          RenderFlowMap(out, entry);
          out << "\n";
        } else {
          out << pad << "-" << "\n";
          RenderNode(out, entry, indent + 2);
        }
      }
      break;
    case YAMLNode::Type::Map:
      if (node.style == YAMLNodeStyle::Flow) {
        out << pad;
        RenderFlowMap(out, node);
        out << "\n";
        break;
      }
      for (const auto& entry : node.m_mapChildren) {
        out << pad << entry.first << ":";
        if (entry.second.type == YAMLNode::Type::Scalar) {
          out << " " << entry.second.scalar << "\n";
        } else if (entry.second.type == YAMLNode::Type::Map && entry.second.style == YAMLNodeStyle::Flow) {
          out << " ";
          RenderFlowMap(out, entry.second);
          out << "\n";
        } else {
          out << "\n";
          RenderNode(out, entry.second, indent + 2);
        }
      }
      break;
    }
  }

  static void RenderFlowMap(std::ostringstream& out, const YAMLNode& node) {
    out << "{";
    bool first = true;
    for (const auto& entry : node.m_mapChildren) {
      if (!first)
        out << ", ";
      first = false;
      out << entry.first << ": " << entry.second.scalar;
    }
    out << "}";
  }
};

} // namespace athena::io
