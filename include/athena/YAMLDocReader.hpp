#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "athena/DNA.hpp"
#include "athena/YAMLDocCommon.hpp"

namespace athena::io {

class YAMLDocReader {
public:
  class Scope {
    YAMLDocReader* m_reader = nullptr;
    bool m_valid = false;

  public:
    Scope() = default;
    Scope(YAMLDocReader* reader, bool valid) : m_reader(reader), m_valid(valid) {}
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&& other) noexcept : m_reader(other.m_reader), m_valid(other.m_valid) { other.m_reader = nullptr; }
    ~Scope() {
      if (m_reader && m_valid)
        m_reader->leave();
    }
    explicit operator bool() const { return m_valid; }
    void leave() {
      if (m_reader && m_valid) {
        m_reader->leave();
        m_valid = false;
      }
    }
  };

  bool parse(IStreamReader* reader) {
    if (!reader)
      return false;
    std::string content;
    char buf[4096];
    while (true) {
      size_t read = reader->readBytesToBuf(buf, sizeof(buf));
      if (!read)
        break;
      content.append(buf, read);
    }

    m_root = YAMLNode{};
    m_root.type = YAMLNode::Type::Map;
    m_stack.clear();
    m_stack.push_back({&m_root, 0, 0});

    std::vector<std::string> lines;
    std::istringstream in(content);
    std::string line;
    while (std::getline(in, line))
      lines.push_back(line);

    for (size_t i = 0; i < lines.size(); ++i) {
      auto rawLine = StripComment(lines[i]);
      if (rawLine.empty())
        continue;
      size_t indent = CountIndent(rawLine);
      std::string_view trimmed = TrimLeft(rawLine);
      if (trimmed.empty())
        continue;

      while (m_stack.size() > 1 && indent <= m_stack.back().indent)
        m_stack.pop_back();

      YAMLNode* current = m_stack.back().node;
      if (StartsWith(trimmed, "- ")) {
        if (current->type != YAMLNode::Type::Sequence)
          current->type = YAMLNode::Type::Sequence;
        std::string_view item = TrimLeft(trimmed.substr(2));
        YAMLNode entry = ParseInlineNode(item);
        current->m_seqChildren.push_back(entry);
        if (entry.type != YAMLNode::Type::Scalar && item.empty()) {
          m_stack.push_back({&current->m_seqChildren.back(), indent + 2, 0});
        }
        continue;
      }

      auto [key, value] = SplitKeyValue(trimmed);
      if (key.empty())
        continue;

      YAMLNode child;
      if (value.empty()) {
        bool nextIsSeq = NextLineIsSequence(lines, i + 1, indent);
        child.type = nextIsSeq ? YAMLNode::Type::Sequence : YAMLNode::Type::Map;
        current->m_mapChildren[key] = std::move(child);
        m_stack.push_back({&current->m_mapChildren[key], indent + 2, 0});
      } else {
        child = ParseInlineNode(value);
        current->m_mapChildren[key] = std::move(child);
      }
    }

    m_cur = &m_root;
    m_stack.clear();
    m_seqStack.clear();
    return true;
  }

  YAMLNode* getCurNode() { return m_cur; }

  Scope enterSubRecord(const char* name = nullptr) {
    if (!m_cur)
      return {};
    if (name && *name) {
      if (m_cur->type != YAMLNode::Type::Map)
        return {};
      auto it = m_cur->m_mapChildren.find(name);
      if (it == m_cur->m_mapChildren.end())
        return {};
      m_stack.push_back({m_cur, 0, 0});
      m_cur = &it->second;
      return Scope(this, true);
    }
    if (m_cur->type != YAMLNode::Type::Sequence)
      return {};
    if (m_seqStack.empty() || m_seqStack.back().node != m_cur)
      return {};
    auto& frame = m_seqStack.back();
    if (frame.index >= m_cur->m_seqChildren.size())
      return {};
    m_stack.push_back({m_cur, 0, 0});
    m_cur = &m_cur->m_seqChildren[frame.index++];
    return Scope(this, true);
  }

  Scope enterSubVector(const char* name, size_t& count) {
    if (!m_cur || !name || !*name)
      return {};
    if (m_cur->type != YAMLNode::Type::Map)
      return {};
    auto it = m_cur->m_mapChildren.find(name);
    if (it == m_cur->m_mapChildren.end())
      return {};
    if (it->second.type != YAMLNode::Type::Sequence)
      return {};
    count = it->second.m_seqChildren.size();
    m_stack.push_back({m_cur, 0, 0});
    m_cur = &it->second;
    m_seqStack.push_back({m_cur, 0});
    return Scope(this, true);
  }

  std::string readString(const char* name = nullptr) const {
    const YAMLNode* node = ResolveNode(name);
    if (!node)
      return {};
    return node->scalar;
  }

  uint16_t readUint16(const char* name = nullptr) const { return ReadScalar<uint16_t>(name); }
  uint32_t readUint32(const char* name = nullptr) const { return ReadScalar<uint32_t>(name); }
  int16_t readInt16(const char* name = nullptr) const { return ReadScalar<int16_t>(name); }
  int32_t readInt32(const char* name = nullptr) const { return ReadScalar<int32_t>(name); }
  uint8_t readUint8(const char* name = nullptr) const { return ReadScalar<uint8_t>(name); }
  int8_t readInt8(const char* name = nullptr) const { return ReadScalar<int8_t>(name); }
  bool readBool(const char* name = nullptr) const { return ReadScalar<bool>(name); }
  double readDouble(const char* name = nullptr) const { return ReadScalar<double>(name); }
  float readFloat(const char* name = nullptr) const { return ReadScalar<float>(name); }

  template <typename T>
  void enumerate(const char* name, T& value) {
    if constexpr (detail::IsValue<T>::value) {
      value = static_cast<typename T::ValueType>(ReadScalar<typename T::ValueType>(name));
    } else if constexpr (detail::IsDnaV<T>) {
      if (auto scope = enterSubRecord(name))
        value.template Enumerate<ReadYamlOp>(*this);
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
      value = static_cast<T>(ReadScalar<T>(name));
    } else if constexpr (std::is_same_v<T, std::string>) {
      value = readString(name);
    } else if constexpr (IsVector<T>::value) {
      ReadVector(name, value);
    }
  }

private:
  struct StackFrame {
    YAMLNode* node;
    size_t indent;
    size_t index;
  };

  struct SequenceFrame {
    YAMLNode* node;
    size_t index;
  };

  YAMLNode m_root;
  YAMLNode* m_cur = nullptr;
  std::vector<StackFrame> m_stack;
  std::vector<SequenceFrame> m_seqStack;

  void leave() {
    if (!m_stack.empty()) {
      YAMLNode* leaving = m_cur;
      m_cur = m_stack.back().node;
      m_stack.pop_back();
      if (!m_seqStack.empty() && m_seqStack.back().node == leaving)
        m_seqStack.pop_back();
    }
  }

  static std::string StripComment(const std::string& line) {
    auto pos = line.find('#');
    std::string trimmed = line.substr(0, pos);
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back())))
      trimmed.pop_back();
    return trimmed;
  }

  static size_t CountIndent(const std::string& line) {
    size_t count = 0;
    while (count < line.size() && line[count] == ' ')
      ++count;
    return count;
  }

  static std::string_view TrimLeft(std::string_view in) {
    while (!in.empty() && std::isspace(static_cast<unsigned char>(in.front())))
      in.remove_prefix(1);
    return in;
  }

  static bool StartsWith(std::string_view in, std::string_view prefix) {
    return in.substr(0, prefix.size()) == prefix;
  }

  static std::pair<std::string, std::string_view> SplitKeyValue(std::string_view line) {
    auto pos = line.find(':');
    if (pos == std::string_view::npos)
      return {};
    std::string key(line.substr(0, pos));
    while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back())))
      key.pop_back();
    std::string_view value = line.substr(pos + 1);
    value = TrimLeft(value);
    return {key, value};
  }

  static bool NextLineIsSequence(const std::vector<std::string>& lines, size_t start, size_t indent) {
    for (size_t idx = start; idx < lines.size(); ++idx) {
      auto raw = StripComment(lines[idx]);
      if (raw.empty())
        continue;
      size_t nextIndent = CountIndent(raw);
      if (nextIndent <= indent)
        return false;
      std::string_view trimmed = TrimLeft(raw);
      return StartsWith(trimmed, "- ");
    }
    return false;
  }

  static YAMLNode ParseInlineNode(std::string_view value) {
    YAMLNode node;
    if (value.empty()) {
      node.type = YAMLNode::Type::Map;
      return node;
    }
    if (value.front() == '{' && value.back() == '}') {
      node.type = YAMLNode::Type::Map;
      node.style = YAMLNodeStyle::Flow;
      std::string_view inner = value.substr(1, value.size() - 2);
      ParseFlowMap(inner, node);
      return node;
    }
    node.type = YAMLNode::Type::Scalar;
    node.scalar = Unquote(value);
    return node;
  }

  static void ParseFlowMap(std::string_view input, YAMLNode& node) {
    size_t pos = 0;
    while (pos < input.size()) {
      auto nextComma = input.find(',', pos);
      std::string_view pair = (nextComma == std::string_view::npos) ? input.substr(pos) : input.substr(pos, nextComma - pos);
      auto [key, value] = SplitKeyValue(TrimLeft(pair));
      if (!key.empty()) {
        YAMLNode child = ParseInlineNode(value);
        node.m_mapChildren[key] = std::move(child);
      }
      if (nextComma == std::string_view::npos)
        break;
      pos = nextComma + 1;
    }
  }

  static std::string Unquote(std::string_view value) {
    value = TrimLeft(value);
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
      return std::string(value.substr(1, value.size() - 2));
    }
    return std::string(value);
  }

  const YAMLNode* ResolveNode(const char* name) const {
    if (!m_cur)
      return nullptr;
    if (!name || !*name)
      return m_cur;
    if (m_cur->type != YAMLNode::Type::Map)
      return nullptr;
    auto it = m_cur->m_mapChildren.find(name);
    if (it == m_cur->m_mapChildren.end())
      return nullptr;
    return &it->second;
  }

  template <typename T>
  T ReadScalar(const char* name) const {
    const YAMLNode* node = ResolveNode(name);
    if (!node)
      return {};
    std::string text = node->scalar;
    if constexpr (std::is_same_v<T, bool>) {
      std::string lower;
      lower.resize(text.size());
      std::transform(text.begin(), text.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
      if (lower == "true" || lower == "yes" || lower == "1")
        return true;
      return false;
    } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
      return static_cast<T>(std::strtoll(text.c_str(), nullptr, 0));
    } else if constexpr (std::is_floating_point_v<T>) {
      return static_cast<T>(std::strtod(text.c_str(), nullptr));
    } else {
      return {};
    }
  }

  template <typename T>
  struct IsVector : std::false_type {};

  template <typename T, typename Alloc>
  struct IsVector<std::vector<T, Alloc>> : std::true_type {};

  template <typename T>
  void ReadVector(const char* name, std::vector<T>& value) {
    const YAMLNode* node = ResolveNode(name);
    if (!node || node->type != YAMLNode::Type::Sequence)
      return;
    value.clear();
    value.reserve(node->m_seqChildren.size());
    for (const auto& entry : node->m_seqChildren) {
      if constexpr (std::is_same_v<T, uint8_t>) {
        value.push_back(static_cast<uint8_t>(std::strtoul(entry.scalar.c_str(), nullptr, 0)));
      } else if constexpr (std::is_integral_v<T>) {
        value.push_back(static_cast<T>(std::strtoll(entry.scalar.c_str(), nullptr, 0)));
      } else if constexpr (std::is_floating_point_v<T>) {
        value.push_back(static_cast<T>(std::strtod(entry.scalar.c_str(), nullptr)));
      }
    }
  }
};

} // namespace athena::io
