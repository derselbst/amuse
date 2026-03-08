#pragma once

#include <map>
#include <string>
#include <vector>

namespace athena::io {

enum class YAMLNodeStyle { Block, Flow };

struct YAMLNode {
  enum class Type { Map, Sequence, Scalar };

  Type type = Type::Map;
  YAMLNodeStyle style = YAMLNodeStyle::Block;
  std::string scalar;
  std::map<std::string, YAMLNode> m_mapChildren;
  std::vector<YAMLNode> m_seqChildren;
};

} // namespace athena::io
