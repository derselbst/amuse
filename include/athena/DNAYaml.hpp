#pragma once

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "athena/IStream.hpp"

namespace athena::io {

enum class YAMLNodeStyle { Default, Flow, Block };
class YAMLDocReader;
class YAMLDocWriter;

struct YAMLNode {
  enum class Type { Scalar, Map, Sequence };
  Type m_type = Type::Map;
  std::string m_scalarString;
  std::vector<std::pair<std::string, std::shared_ptr<YAMLNode>>> m_mapChildren;
  std::vector<std::shared_ptr<YAMLNode>> m_seqChildren;
};

namespace detail {

inline std::string_view trim(std::string_view in) {
  size_t b = 0;
  while (b < in.size() && (in[b] == ' ' || in[b] == '\t'))
    ++b;
  size_t e = in.size();
  while (e > b && (in[e - 1] == ' ' || in[e - 1] == '\t'))
    --e;
  return in.substr(b, e - b);
}

inline int indentOf(std::string_view line) {
  int i = 0;
  while (i < int(line.size()) && line[size_t(i)] == ' ')
    ++i;
  return i;
}

inline std::string unquote(std::string_view in) {
  in = trim(in);
  if (in.size() >= 2 && ((in.front() == '"' && in.back() == '"') || (in.front() == '\'' && in.back() == '\'')))
    return std::string(in.substr(1, in.size() - 2));
  return std::string(in);
}

inline std::shared_ptr<YAMLNode> makeScalar(std::string_view val) {
  auto node = std::make_shared<YAMLNode>();
  node->m_type = YAMLNode::Type::Scalar;
  node->m_scalarString = unquote(val);
  return node;
}

inline std::string toYamlScalar(std::string_view in) {
  if (in.empty())
    return "\"\"";
  bool quote = false;
  for (char c : in) {
    if (c == ':' || c == '#' || c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == '"' ||
        c == '\'' || c == '\n' || c == '\r' || c == '\t') {
      quote = true;
      break;
    }
  }
  if (!quote && in.front() != ' ' && in.back() != ' ')
    return std::string(in);
  std::string out;
  out.reserve(in.size() + 2);
  out.push_back('"');
  for (char c : in) {
    if (c == '"' || c == '\\')
      out.push_back('\\');
    out.push_back(c);
  }
  out.push_back('"');
  return out;
}

inline std::shared_ptr<YAMLNode> parseFlowMap(std::string_view v) {
  auto node = std::make_shared<YAMLNode>();
  node->m_type = YAMLNode::Type::Map;
  v = trim(v);
  if (v.size() < 2 || v.front() != '{' || v.back() != '}')
    return node;
  v = trim(v.substr(1, v.size() - 2));
  while (!v.empty()) {
    size_t colon = v.find(':');
    if (colon == std::string_view::npos)
      break;
    std::string key = unquote(v.substr(0, colon));
    v = trim(v.substr(colon + 1));
    size_t comma = v.find(',');
    std::string_view val;
    if (comma == std::string_view::npos) {
      val = v;
      v = {};
    } else {
      val = v.substr(0, comma);
      v = trim(v.substr(comma + 1));
    }
    node->m_mapChildren.emplace_back(std::move(key), makeScalar(val));
  }
  return node;
}

inline std::shared_ptr<YAMLNode> parseFlowSeq(std::string_view v) {
  auto node = std::make_shared<YAMLNode>();
  node->m_type = YAMLNode::Type::Sequence;
  v = trim(v);
  if (v.size() < 2 || v.front() != '[' || v.back() != ']')
    return node;
  v = trim(v.substr(1, v.size() - 2));
  while (!v.empty()) {
    size_t comma = v.find(',');
    std::string_view val;
    if (comma == std::string_view::npos) {
      val = v;
      v = {};
    } else {
      val = v.substr(0, comma);
      v = trim(v.substr(comma + 1));
    }
    node->m_seqChildren.push_back(makeScalar(val));
  }
  return node;
}

std::shared_ptr<YAMLNode> parseBlock(const std::vector<std::string>& lines, size_t& idx, int indent);

inline std::shared_ptr<YAMLNode> parseMap(const std::vector<std::string>& lines, size_t& idx, int indent) {
  auto node = std::make_shared<YAMLNode>();
  node->m_type = YAMLNode::Type::Map;
  while (idx < lines.size()) {
    std::string_view line = lines[idx];
    int lineIndent = indentOf(line);
    if (lineIndent < indent)
      break;
    if (lineIndent > indent) {
      ++idx;
      continue;
    }
    line = trim(line.substr(size_t(indent)));
    if (line.empty() || line.starts_with('#')) {
      ++idx;
      continue;
    }
    if (line.starts_with("- "))
      break;
    size_t colon = line.find(':');
    if (colon == std::string_view::npos) {
      ++idx;
      continue;
    }
    std::string key = unquote(line.substr(0, colon));
    std::string_view rest = trim(line.substr(colon + 1));
    ++idx;
    std::shared_ptr<YAMLNode> child;
    if (rest.empty()) {
      if (idx < lines.size() && indentOf(lines[idx]) > indent)
        child = parseBlock(lines, idx, indent + 2);
      else
        child = makeScalar({});
    } else if (rest.front() == '{' && rest.back() == '}') {
      child = parseFlowMap(rest);
    } else if (rest.front() == '[' && rest.back() == ']') {
      child = parseFlowSeq(rest);
    } else {
      child = makeScalar(rest);
    }
    node->m_mapChildren.emplace_back(std::move(key), std::move(child));
  }
  return node;
}

inline std::shared_ptr<YAMLNode> parseSeq(const std::vector<std::string>& lines, size_t& idx, int indent) {
  auto node = std::make_shared<YAMLNode>();
  node->m_type = YAMLNode::Type::Sequence;
  while (idx < lines.size()) {
    std::string_view line = lines[idx];
    int lineIndent = indentOf(line);
    if (lineIndent < indent)
      break;
    if (lineIndent != indent) {
      ++idx;
      continue;
    }
    line = trim(line.substr(size_t(indent)));
    if (!line.starts_with("- "))
      break;
    std::string_view rest = trim(line.substr(2));
    ++idx;
    if (rest.empty()) {
      if (idx < lines.size() && indentOf(lines[idx]) > indent)
        node->m_seqChildren.push_back(parseBlock(lines, idx, indent + 2));
      else
        node->m_seqChildren.push_back(makeScalar({}));
      continue;
    }
    if (rest.front() == '{' && rest.back() == '}')
      node->m_seqChildren.push_back(parseFlowMap(rest));
    else if (rest.front() == '[' && rest.back() == ']')
      node->m_seqChildren.push_back(parseFlowSeq(rest));
    else
      node->m_seqChildren.push_back(makeScalar(rest));
  }
  return node;
}

inline std::shared_ptr<YAMLNode> parseBlock(const std::vector<std::string>& lines, size_t& idx, int indent) {
  while (idx < lines.size()) {
    std::string_view line = lines[idx];
    int lineIndent = indentOf(line);
    if (lineIndent < indent)
      return std::make_shared<YAMLNode>();
    line = trim(line.substr(size_t(lineIndent)));
    if (line.empty() || line.starts_with('#')) {
      ++idx;
      continue;
    }
    if (line.starts_with("- "))
      return parseSeq(lines, idx, indent);
    return parseMap(lines, idx, indent);
  }
  return std::make_shared<YAMLNode>();
}

inline void emitNode(const YAMLNode& node, std::string& out, int indent);

inline void emitIndent(std::string& out, int indent) {
  out.append(size_t(indent), ' ');
}

inline void emitMap(const YAMLNode& node, std::string& out, int indent) {
  for (const auto& kv : node.m_mapChildren) {
    emitIndent(out, indent);
    out.append(kv.first);
    out.append(":");
    if (kv.second->m_type == YAMLNode::Type::Scalar) {
      out.push_back(' ');
      out.append(toYamlScalar(kv.second->m_scalarString));
      out.push_back('\n');
    } else {
      out.push_back('\n');
      emitNode(*kv.second, out, indent + 2);
    }
  }
}

inline void emitSeq(const YAMLNode& node, std::string& out, int indent) {
  for (const auto& ent : node.m_seqChildren) {
    emitIndent(out, indent);
    out.append("-");
    if (ent->m_type == YAMLNode::Type::Scalar) {
      out.push_back(' ');
      out.append(toYamlScalar(ent->m_scalarString));
      out.push_back('\n');
    } else {
      out.push_back('\n');
      emitNode(*ent, out, indent + 2);
    }
  }
}

inline void emitNode(const YAMLNode& node, std::string& out, int indent) {
  if (node.m_type == YAMLNode::Type::Map)
    emitMap(node, out, indent);
  else if (node.m_type == YAMLNode::Type::Sequence)
    emitSeq(node, out, indent);
  else {
    emitIndent(out, indent);
    out.append(toYamlScalar(node.m_scalarString));
    out.push_back('\n');
  }
}

template <typename T>
inline T parseIntegral(std::string_view in, T fallback = 0) {
  in = trim(in);
  if (in.empty())
    return fallback;
  T out = 0;
  auto [ptr, ec] = std::from_chars(in.data(), in.data() + in.size(), out);
  if (ec == std::errc() && ptr == in.data() + in.size())
    return out;
  return static_cast<T>(std::strtoll(std::string(in).c_str(), nullptr, 0));
}

inline float parseFloat(std::string_view in) { return std::strtof(std::string(trim(in)).c_str(), nullptr); }
inline double parseDouble(std::string_view in) { return std::strtod(std::string(trim(in)).c_str(), nullptr); }

struct ReaderSubGuard {
  ::athena::io::YAMLDocReader* m_reader = nullptr;
  bool m_valid = false;
  explicit ReaderSubGuard(::athena::io::YAMLDocReader* reader, bool valid) : m_reader(reader), m_valid(valid) {}
  ReaderSubGuard(const ReaderSubGuard&) = delete;
  ReaderSubGuard& operator=(const ReaderSubGuard&) = delete;
  ReaderSubGuard(ReaderSubGuard&& other) noexcept : m_reader(other.m_reader), m_valid(other.m_valid) {
    other.m_reader = nullptr;
    other.m_valid = false;
  }
  ~ReaderSubGuard();
  explicit operator bool() const noexcept { return m_valid; }
  void leave() noexcept;
};

struct WriterSubGuard {
  ::athena::io::YAMLDocWriter* m_writer = nullptr;
  bool m_valid = false;
  explicit WriterSubGuard(::athena::io::YAMLDocWriter* writer, bool valid) : m_writer(writer), m_valid(valid) {}
  WriterSubGuard(const WriterSubGuard&) = delete;
  WriterSubGuard& operator=(const WriterSubGuard&) = delete;
  WriterSubGuard(WriterSubGuard&& other) noexcept : m_writer(other.m_writer), m_valid(other.m_valid) {
    other.m_writer = nullptr;
    other.m_valid = false;
  }
  ~WriterSubGuard();
  explicit operator bool() const noexcept { return m_valid; }
  void leave() noexcept;
};

} // namespace detail

class YAMLDocReader {
  struct Frame {
    YAMLNode* node = nullptr;
    size_t seqIdx = 0;
  };

  std::shared_ptr<YAMLNode> m_root = std::make_shared<YAMLNode>();
  std::vector<Frame> m_stack;

  YAMLNode* curNode() { return m_stack.empty() ? m_root.get() : m_stack.back().node; }
  const YAMLNode* curNode() const { return m_stack.empty() ? m_root.get() : m_stack.back().node; }

  YAMLNode* mapChild(std::string_view key) {
    YAMLNode* cur = curNode();
    if (!cur || cur->m_type != YAMLNode::Type::Map)
      return nullptr;
    for (auto& kv : cur->m_mapChildren)
      if (kv.first == key)
        return kv.second.get();
    return nullptr;
  }

  YAMLNode* nextSeqChild() {
    if (m_stack.empty())
      return nullptr;
    Frame& fr = m_stack.back();
    if (!fr.node || fr.node->m_type != YAMLNode::Type::Sequence || fr.seqIdx >= fr.node->m_seqChildren.size())
      return nullptr;
    return fr.node->m_seqChildren[fr.seqIdx++].get();
  }

  std::string scalar(std::string_view key = {}) {
    YAMLNode* node = nullptr;
    if (!key.empty())
      node = mapChild(key);
    else {
      YAMLNode* cur = curNode();
      if (cur && cur->m_type == YAMLNode::Type::Scalar)
        node = cur;
      else if (cur && cur->m_type == YAMLNode::Type::Sequence)
        node = nextSeqChild();
    }
    if (!node || node->m_type != YAMLNode::Type::Scalar)
      return {};
    return node->m_scalarString;
  }

  void push(YAMLNode* node) { m_stack.push_back({node, 0}); }
  void pop() {
    if (!m_stack.empty())
      m_stack.pop_back();
  }

  friend struct detail::ReaderSubGuard;

public:
  YAMLDocReader() { m_root->m_type = YAMLNode::Type::Map; }

  bool parse(IStreamReader* r) {
    m_root = std::make_shared<YAMLNode>();
    m_root->m_type = YAMLNode::Type::Map;
    m_stack.clear();
    if (!r || r->hasError())
      return false;
    const int64_t start = r->position();
    r->seek(0, athena::SeekOrigin::End);
    const int64_t end = r->position();
    if (end < start)
      return false;
    r->seek(start, athena::SeekOrigin::Begin);
    auto bytes = r->readUBytes(uint64_t(end - start));
    if (r->hasError())
      return false;
    std::string src(reinterpret_cast<const char*>(bytes.get()), size_t(end - start));
    std::vector<std::string> lines;
    size_t cur = 0;
    while (cur <= src.size()) {
      size_t nl = src.find('\n', cur);
      if (nl == std::string::npos)
        nl = src.size();
      std::string line = src.substr(cur, nl - cur);
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      lines.push_back(std::move(line));
      if (nl == src.size())
        break;
      cur = nl + 1;
    }
    size_t idx = 0;
    m_root = detail::parseBlock(lines, idx, 0);
    if (!m_root)
      m_root = std::make_shared<YAMLNode>();
    if (m_root->m_type == YAMLNode::Type::Scalar) {
      auto wrap = std::make_shared<YAMLNode>();
      wrap->m_type = YAMLNode::Type::Map;
      wrap->m_mapChildren.emplace_back("value", m_root);
      m_root = std::move(wrap);
    }
    return true;
  }

  std::string readString(std::string_view key = {}) { return scalar(key); }
  uint8_t readUint8(std::string_view key = {}) { return detail::parseIntegral<uint8_t>(scalar(key)); }
  uint16_t readUint16(std::string_view key = {}) { return detail::parseIntegral<uint16_t>(scalar(key)); }
  uint32_t readUint32(std::string_view key = {}) { return detail::parseIntegral<uint32_t>(scalar(key)); }
  uint64_t readUint64(std::string_view key = {}) { return detail::parseIntegral<uint64_t>(scalar(key)); }
  int8_t readInt8(std::string_view key = {}) { return detail::parseIntegral<int8_t>(scalar(key)); }
  int16_t readInt16(std::string_view key = {}) { return detail::parseIntegral<int16_t>(scalar(key)); }
  int32_t readInt32(std::string_view key = {}) { return detail::parseIntegral<int32_t>(scalar(key)); }
  int64_t readInt64(std::string_view key = {}) { return detail::parseIntegral<int64_t>(scalar(key)); }
  float readFloat(std::string_view key = {}) { return detail::parseFloat(scalar(key)); }
  double readDouble(std::string_view key = {}) { return detail::parseDouble(scalar(key)); }
  bool readBool(std::string_view key = {}) {
    std::string val = scalar(key);
    if (val == "true" || val == "True" || val == "1")
      return true;
    if (val == "false" || val == "False" || val == "0")
      return false;
    return !val.empty();
  }

  template <typename T>
  void enumerate(std::string_view key, std::vector<T>& vec) {
    size_t count = 0;
    if (auto g = enterSubVector(key, count)) {
      vec.resize(count);
      for (size_t i = 0; i < count; ++i) {
        if constexpr (std::is_same_v<T, std::string>)
          vec[i] = readString();
        else if constexpr (std::is_same_v<T, bool>)
          vec[i] = readBool();
        else if constexpr (std::is_integral_v<T>)
          vec[i] = detail::parseIntegral<T>(scalar());
        else if constexpr (std::is_floating_point_v<T>)
          vec[i] = static_cast<T>(detail::parseDouble(scalar()));
      }
    }
  }

  template <typename T>
  void enumerate(std::vector<T>& vec) {
    size_t count = 0;
    if (auto g = enterSubVector({}, count)) {
      vec.resize(count);
      for (size_t i = 0; i < count; ++i) {
        if constexpr (std::is_same_v<T, std::string>)
          vec[i] = readString();
        else if constexpr (std::is_same_v<T, bool>)
          vec[i] = readBool();
        else if constexpr (std::is_integral_v<T>)
          vec[i] = detail::parseIntegral<T>(scalar());
        else if constexpr (std::is_floating_point_v<T>)
          vec[i] = static_cast<T>(detail::parseDouble(scalar()));
      }
    }
  }

  detail::ReaderSubGuard enterSubRecord(std::string_view key = {}) {
    YAMLNode* tgt = nullptr;
    if (!key.empty())
      tgt = mapChild(key);
    else
      tgt = nextSeqChild();
    if (!tgt || tgt->m_type != YAMLNode::Type::Map)
      return detail::ReaderSubGuard(this, false);
    push(tgt);
    return detail::ReaderSubGuard(this, true);
  }

  detail::ReaderSubGuard enterSubVector(std::string_view key, size_t& sizeOut) {
    YAMLNode* tgt = nullptr;
    if (!key.empty())
      tgt = mapChild(key);
    else
      tgt = nextSeqChild();
    if (!tgt || tgt->m_type != YAMLNode::Type::Sequence) {
      sizeOut = 0;
      return detail::ReaderSubGuard(this, false);
    }
    sizeOut = tgt->m_seqChildren.size();
    push(tgt);
    return detail::ReaderSubGuard(this, true);
  }

  YAMLNode* getCurNode() { return curNode(); }
  YAMLNode* getRootNode() { return m_root.get(); }
};

class YAMLDocWriter {
  YAMLNode m_root;
  std::vector<YAMLNode*> m_stack;
  YAMLNodeStyle m_style = YAMLNodeStyle::Default;

  YAMLNode* curNode() { return m_stack.empty() ? &m_root : m_stack.back(); }

  std::shared_ptr<YAMLNode> makeScalar(std::string_view val) const {
    auto n = std::make_shared<YAMLNode>();
    n->m_type = YAMLNode::Type::Scalar;
    n->m_scalarString = std::string(val);
    return n;
  }

  std::shared_ptr<YAMLNode> makeMap() const {
    auto n = std::make_shared<YAMLNode>();
    n->m_type = YAMLNode::Type::Map;
    return n;
  }

  std::shared_ptr<YAMLNode> makeSeq() const {
    auto n = std::make_shared<YAMLNode>();
    n->m_type = YAMLNode::Type::Sequence;
    return n;
  }

  void addNode(std::string_view key, std::shared_ptr<YAMLNode> node) {
    YAMLNode* cur = curNode();
    if (!cur)
      return;
    if (cur->m_type == YAMLNode::Type::Map) {
      if (key.empty())
        return;
      cur->m_mapChildren.emplace_back(std::string(key), std::move(node));
    } else if (cur->m_type == YAMLNode::Type::Sequence) {
      if (!key.empty()) {
        auto m = makeMap();
        m->m_mapChildren.emplace_back(std::string(key), std::move(node));
        cur->m_seqChildren.push_back(std::move(m));
      } else {
        cur->m_seqChildren.push_back(std::move(node));
      }
    }
  }

  void push(YAMLNode* node) { m_stack.push_back(node); }
  void pop() {
    if (!m_stack.empty())
      m_stack.pop_back();
  }

  friend struct detail::WriterSubGuard;

public:
  explicit YAMLDocWriter(std::string_view /*type*/ = {}) {
    m_root.m_type = YAMLNode::Type::Map;
    m_stack.push_back(&m_root);
  }

  void setStyle(YAMLNodeStyle style) { m_style = style; }

  void writeString(std::string_view key, std::string_view val) { addNode(key, makeScalar(val)); }
  void writeString(std::string_view val) { addNode({}, makeScalar(val)); }

  void writeUint8(std::string_view key, uint8_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeUint8(uint8_t val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeUint16(std::string_view key, uint16_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeUint16(uint16_t val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeUint32(std::string_view key, uint32_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeUint32(uint32_t val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeUint64(std::string_view key, uint64_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeUint64(uint64_t val) { addNode({}, makeScalar(std::to_string(val))); }

  void writeInt8(std::string_view key, int8_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeInt8(int8_t val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeInt16(std::string_view key, int16_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeInt16(int16_t val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeInt32(std::string_view key, int32_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeInt32(int32_t val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeInt64(std::string_view key, int64_t val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeInt64(int64_t val) { addNode({}, makeScalar(std::to_string(val))); }

  void writeFloat(std::string_view key, float val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeFloat(float val) { addNode({}, makeScalar(std::to_string(val))); }
  void writeDouble(std::string_view key, double val) { addNode(key, makeScalar(std::to_string(val))); }
  void writeDouble(double val) { addNode({}, makeScalar(std::to_string(val))); }

  void writeBool(std::string_view key, bool val) { addNode(key, makeScalar(val ? "true" : "false")); }
  void writeBool(bool val) { addNode({}, makeScalar(val ? "true" : "false")); }

  template <typename T>
  void enumerate(std::string_view key, const std::vector<T>& vec) {
    if (auto g = enterSubVector(key))
      for (const T& v : vec)
        if constexpr (std::is_same_v<T, std::string>)
          writeString(v);
        else if constexpr (std::is_same_v<T, bool>)
          writeBool(v);
        else if constexpr (std::is_integral_v<T>)
          addNode({}, makeScalar(std::to_string(v)));
        else if constexpr (std::is_floating_point_v<T>)
          addNode({}, makeScalar(std::to_string(v)));
  }

  template <typename T>
  void enumerate(const std::vector<T>& vec) {
    if (auto g = enterSubVector())
      for (const T& v : vec)
        if constexpr (std::is_same_v<T, std::string>)
          writeString(v);
        else if constexpr (std::is_same_v<T, bool>)
          writeBool(v);
        else if constexpr (std::is_integral_v<T>)
          addNode({}, makeScalar(std::to_string(v)));
        else if constexpr (std::is_floating_point_v<T>)
          addNode({}, makeScalar(std::to_string(v)));
  }

  detail::WriterSubGuard enterSubRecord(std::string_view key = {}) {
    auto n = makeMap();
    YAMLNode* raw = n.get();
    addNode(key, std::move(n));
    push(raw);
    return detail::WriterSubGuard(this, true);
  }

  detail::WriterSubGuard enterSubVector(std::string_view key = {}) {
    auto n = makeSeq();
    YAMLNode* raw = n.get();
    addNode(key, std::move(n));
    push(raw);
    return detail::WriterSubGuard(this, true);
  }

  void finish(IStreamWriter* w) {
    if (!w)
      return;
    std::string out;
    out.reserve(2048);
    detail::emitNode(m_root, out, 0);
    w->writeUBytes(out.data(), out.size());
  }
};

inline detail::ReaderSubGuard::~ReaderSubGuard() { leave(); }
inline void detail::ReaderSubGuard::leave() noexcept {
  if (m_valid && m_reader) {
    m_reader->pop();
    m_valid = false;
  }
}

inline detail::WriterSubGuard::~WriterSubGuard() { leave(); }
inline void detail::WriterSubGuard::leave() noexcept {
  if (m_valid && m_writer) {
    m_writer->pop();
    m_valid = false;
  }
}

} // namespace athena::io
