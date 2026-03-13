#include "amuse/Common.hpp"

#include <fmt/format.h>
#ifndef _WIN32
#include <cstdio>
#include <memory>
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <logvisor/logvisor.hpp>

using namespace std::literals;

namespace amuse {
static logvisor::Module Log("amuse");

bool Copy(const char* from, const char* to) {
#if _WIN32
  const nowide::wstackstring wfrom(from);
  const nowide::wstackstring wto(to);
  return CopyFileW(wfrom.get(), wto.get(), FALSE) != 0;
#else
  FILE* fi = fopen(from, "rb");
  if (fi == nullptr)
    return false;
  FILE* fo = fopen(to, "wb");
  if (fo == nullptr) {
    fclose(fi);
    return false;
  }
  std::unique_ptr<uint8_t[]> buf(new uint8_t[65536]);
  size_t readSz = 0;
  while ((readSz = fread(buf.get(), 1, 65536, fi)))
    fwrite(buf.get(), 1, readSz, fo);
  fclose(fi);
  fclose(fo);
  struct stat theStat{};
  if (::stat(from, &theStat))
    return true;
#if __APPLE__
  struct timespec times[] = {theStat.st_atimespec, theStat.st_mtimespec};
#elif __SWITCH__
  struct timespec times[] = {theStat.st_atime, theStat.st_mtime};
#else
  struct timespec times[] = {theStat.st_atim, theStat.st_mtim};
#endif
  utimensat(AT_FDCWD, to, times, 0);
  return true;
#endif
}

#if _WIN32
int Rename(const char* oldpath, const char* newpath) {
  const nowide::wstackstring woldpath(oldpath);
  const nowide::wstackstring wnewpath(newpath);
  return MoveFileExW(woldpath.get(), wnewpath.get(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0;
}
#endif

#define DEFINE_ID_TYPE(type, typeName)                                                                                 \
  thread_local NameDB* type::CurNameDB = nullptr;                                                                      \
  template <>                                                                                                          \
  void type##DNA<std::endian::little>::read(std::istream& reader) {                                                  \
    id = amuse::io::readUint16Little(reader);                                                                          \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::little>::write(std::ostream& writer) const {                                           \
    amuse::io::writeUint16Little(writer, id.id);                                                                       \
  }                                                                                                                    \
  template <>                                                                                                          \
  size_t type##DNA<std::endian::little>::binarySize(size_t sz) const {                                               \
    sz += 2;                                                                                                           \
    return sz;                                                                                                         \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::little>::readYaml(amuse::io::YAMLDocReader& reader) {                                  \
    _read(reader);                                                                                                     \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter& writer) const {                           \
    _write(writer);                                                                                                    \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::big>::read(std::istream& reader) {                                                     \
    id = amuse::io::readUint16Big(reader);                                                                             \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::big>::write(std::ostream& writer) const {                                              \
    amuse::io::writeUint16Big(writer, id.id);                                                                          \
  }                                                                                                                    \
  template <>                                                                                                          \
  size_t type##DNA<std::endian::big>::binarySize(size_t sz) const {                                                  \
    sz += 2;                                                                                                           \
    return sz;                                                                                                         \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::big>::readYaml(amuse::io::YAMLDocReader& reader) {                                     \
    _read(reader);                                                                                                     \
  }                                                                                                                    \
  template <>                                                                                                          \
  void type##DNA<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter& writer) const {                              \
    _write(writer);                                                                                                    \
  }                                                                                                                    \
  template <std::endian DNAE>                                                                                        \
  void type##DNA<DNAE>::_read(amuse::io::YAMLDocReader& r) {                                                           \
    std::string name = r.readString();                                                                                 \
    if (!type::CurNameDB)                                                                                              \
      Log.report(logvisor::Fatal, FMT_STRING("Unable to resolve " typeName " name {}, no database present"), name);    \
    if (name.empty()) {                                                                                                \
      id.id = 0xffff;                                                                                                  \
      return;                                                                                                          \
    }                                                                                                                  \
    id = type::CurNameDB->resolveIdFromName(name);                                                                     \
  }                                                                                                                    \
  template <std::endian DNAE>                                                                                        \
  void type##DNA<DNAE>::_write(amuse::io::YAMLDocWriter& w) const {                                                          \
    if (!type::CurNameDB)                                                                                              \
      Log.report(logvisor::Fatal, FMT_STRING("Unable to resolve " typeName " ID {}, no database present"), id);        \
    if (id.id == 0xffff)                                                                                               \
      return;                                                                                                          \
    std::string_view name = type::CurNameDB->resolveNameFromId(id);                                                    \
    if (!name.empty())                                                                                                 \
      w.writeString(name);                                                                                             \
  }                                                                                                                    \
  template <std::endian DNAE>                                                                                        \
  std::string_view type##DNA<DNAE>::DNAType() {                                                                        \
    return "amuse::" #type "DNA"sv;                                                                                    \
  }                                                                                                                    \
  template struct type##DNA<std::endian::big>;                                                                       \
  template struct type##DNA<std::endian::little>;

DEFINE_ID_TYPE(ObjectId, "object")
DEFINE_ID_TYPE(SoundMacroId, "SoundMacro")
DEFINE_ID_TYPE(SampleId, "sample")
DEFINE_ID_TYPE(TableId, "table")
DEFINE_ID_TYPE(KeymapId, "keymap")
DEFINE_ID_TYPE(LayersId, "layers")
DEFINE_ID_TYPE(SongId, "song")
DEFINE_ID_TYPE(SFXId, "sfx")
DEFINE_ID_TYPE(GroupId, "group")

template <>
void PageObjectIdDNA<std::endian::little>::read(std::istream& reader) {
  id = amuse::io::readUint16Little(reader);
}
template <>
void PageObjectIdDNA<std::endian::little>::write(std::ostream& writer) const {
  amuse::io::writeUint16Little(writer, id.id);
}
template <>
size_t PageObjectIdDNA<std::endian::little>::binarySize(size_t sz) const {
  sz += 2;
  return sz;
}
template <>
void PageObjectIdDNA<std::endian::little>::readYaml(amuse::io::YAMLDocReader& reader) {
  _read(reader);
}
template <>
void PageObjectIdDNA<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter& writer) const {
  _write(writer);
}
template <>
void PageObjectIdDNA<std::endian::big>::read(std::istream& reader) {
  id = amuse::io::readUint16Big(reader);
}
template <>
void PageObjectIdDNA<std::endian::big>::write(std::ostream& writer) const {
  amuse::io::writeUint16Big(writer, id.id);
}
template <>
size_t PageObjectIdDNA<std::endian::big>::binarySize(size_t sz) const {
  sz += 2;
  return sz;
}
template <>
void PageObjectIdDNA<std::endian::big>::readYaml(amuse::io::YAMLDocReader& reader) {
  _read(reader);
}
template <>
void PageObjectIdDNA<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter& writer) const {
  _write(writer);
}
template <std::endian DNAE>
void PageObjectIdDNA<DNAE>::_read(amuse::io::YAMLDocReader& r) {
  std::string name = r.readString();
  if (!KeymapId::CurNameDB || !LayersId::CurNameDB)
    Log.report(logvisor::Fatal, FMT_STRING("Unable to resolve keymap or layers name {}, no database present"), name);
  if (name.empty()) {
    id.id = 0xffff;
    return;
  }
  auto search = KeymapId::CurNameDB->m_stringToId.find(name);
  if (search == KeymapId::CurNameDB->m_stringToId.cend()) {
    search = LayersId::CurNameDB->m_stringToId.find(name);
    if (search == LayersId::CurNameDB->m_stringToId.cend()) {
      search = SoundMacroId::CurNameDB->m_stringToId.find(name);
      if (search == SoundMacroId::CurNameDB->m_stringToId.cend()) {
        Log.report(logvisor::Error, FMT_STRING("Unable to resolve name {}"), name);
        id.id = 0xffff;
        return;
      }
    }
  }
  id = search->second;
}
template <std::endian DNAE>
void PageObjectIdDNA<DNAE>::_write(amuse::io::YAMLDocWriter& w) const {
  if (!KeymapId::CurNameDB || !LayersId::CurNameDB)
    Log.report(logvisor::Fatal, FMT_STRING("Unable to resolve keymap or layers ID {}, no database present"), id);
  if (id.id == 0xffff)
    return;
  if (id.id & 0x8000) {
    std::string_view name = LayersId::CurNameDB->resolveNameFromId(id);
    if (!name.empty())
      w.writeString(name);
  } else if (id.id & 0x4000) {
    std::string_view name = KeymapId::CurNameDB->resolveNameFromId(id);
    if (!name.empty())
      w.writeString(name);
  } else {
    std::string_view name = SoundMacroId::CurNameDB->resolveNameFromId(id);
    if (!name.empty())
      w.writeString(name);
  }
}
template <std::endian DNAE>
std::string_view PageObjectIdDNA<DNAE>::DNAType() {
  return "amuse::PageObjectIdDNA"sv;
}
template struct PageObjectIdDNA<std::endian::big>;
template struct PageObjectIdDNA<std::endian::little>;

template <>
void SoundMacroStepDNA<std::endian::little>::read(std::istream& reader) {
  step = amuse::io::readUint16Little(reader);
}
template <>
void SoundMacroStepDNA<std::endian::little>::write(std::ostream& writer) const {
  amuse::io::writeUint16Little(writer, step);
}
template <>
size_t SoundMacroStepDNA<std::endian::little>::binarySize(size_t sz) const {
  sz += 2;
  return sz;
}
template <>
void SoundMacroStepDNA<std::endian::little>::readYaml(amuse::io::YAMLDocReader& reader) {
  step = reader.readUint16();
}
template <>
void SoundMacroStepDNA<std::endian::little>::writeYaml(amuse::io::YAMLDocWriter& writer) const {
  writer.writeUint16(step);
}
template <>
void SoundMacroStepDNA<std::endian::big>::read(std::istream& reader) {
  step = amuse::io::readUint16Big(reader);
}
template <>
void SoundMacroStepDNA<std::endian::big>::write(std::ostream& writer) const {
  amuse::io::writeUint16Big(writer, step);
}
template <>
size_t SoundMacroStepDNA<std::endian::big>::binarySize(size_t sz) const {
  sz += 2;
  return sz;
}
template <>
void SoundMacroStepDNA<std::endian::big>::readYaml(amuse::io::YAMLDocReader& reader) {
  step = reader.readUint16();
}
template <>
void SoundMacroStepDNA<std::endian::big>::writeYaml(amuse::io::YAMLDocWriter& writer) const {
  writer.writeUint16(step);
}
template <std::endian DNAE>
std::string_view SoundMacroStepDNA<DNAE>::DNAType() {
  return "amuse::SoundMacroStepDNA"sv;
}
template struct SoundMacroStepDNA<std::endian::big>;
template struct SoundMacroStepDNA<std::endian::little>;

ObjectId NameDB::generateId(Type tp) const {
  uint16_t maxMatch = 0;
  if (tp == Type::Layer) {
    maxMatch = 0x8000;
  } else if (tp == Type::Keymap) {
    maxMatch = 0x4000;
  }
  for (const auto& p : m_idToString) {
    if (p.first.id >= maxMatch) {
      maxMatch = p.first.id + 1;
    }
  }
  return maxMatch;
}

std::string NameDB::generateName(ObjectId id, Type tp) {
  switch (tp) {
  case Type::SoundMacro:
    return fmt::format(FMT_STRING("macro{}"), id);
  case Type::Table:
    return fmt::format(FMT_STRING("table{}"), id);
  case Type::Keymap:
    return fmt::format(FMT_STRING("keymap{}"), id);
  case Type::Layer:
    return fmt::format(FMT_STRING("layers{}"), id);
  case Type::Song:
    return fmt::format(FMT_STRING("song{}"), id);
  case Type::SFX:
    return fmt::format(FMT_STRING("sfx{}"), id);
  case Type::Group:
    return fmt::format(FMT_STRING("group{}"), id);
  case Type::Sample:
    return fmt::format(FMT_STRING("sample{}"), id);
  default:
    return fmt::format(FMT_STRING("obj{}"), id);
  }
}

std::string NameDB::generateDefaultName(Type tp) const { return generateName(generateId(tp), tp); }

std::string_view NameDB::registerPair(std::string_view str, ObjectId id) {
  auto string = std::string(str);
  m_stringToId.insert_or_assign(string, id);
  return m_idToString.emplace(id, std::move(string)).first->second;
}

std::string_view NameDB::resolveNameFromId(ObjectId id) const {
  auto search = m_idToString.find(id);
  if (search == m_idToString.cend()) {
    Log.report(logvisor::Error, FMT_STRING("Unable to resolve ID {}"), id);
    return ""sv;
  }
  return search->second;
}

ObjectId NameDB::resolveIdFromName(std::string_view str) const {
  auto search = m_stringToId.find(std::string(str));
  if (search == m_stringToId.cend()) {
    Log.report(logvisor::Error, FMT_STRING("Unable to resolve name {}"), str);
    return {};
  }
  return search->second;
}

void NameDB::remove(ObjectId id) {
  auto search = m_idToString.find(id);
  if (search == m_idToString.cend())
    return;
  auto search2 = m_stringToId.find(search->second);
  if (search2 == m_stringToId.cend())
    return;
  m_idToString.erase(search);
  m_stringToId.erase(search2);
}

void NameDB::rename(ObjectId id, std::string_view str) {
  auto search = m_idToString.find(id);
  if (search == m_idToString.cend())
    return;
  if (search->second == str)
    return;
  auto search2 = m_stringToId.find(search->second);
  if (search2 == m_stringToId.cend())
    return;
#if __APPLE__
  std::swap(m_stringToId[std::string(str)], search2->second);
  m_stringToId.erase(search2);
#else
  auto nh = m_stringToId.extract(search2);
  nh.key() = str;
  m_stringToId.insert(std::move(nh));
#endif
  search->second = str;
}

void LittleUInt24::read(std::istream& reader) {
  union {
    uint32_t val;
    char bytes[4];
  } data = {};
  amuse::io::readBytes(reader, data.bytes, 3);
  val = SLittle(data.val);
}

void LittleUInt24::write(std::ostream& writer) const {
  union {
    uint32_t val;
    char bytes[4];
  } data;
  data.val = SLittle(val);
  amuse::io::writeBytes(writer, data.bytes, 3);
}

size_t LittleUInt24::binarySize(size_t sz) const {
  sz += 3;
  return sz;
}

void LittleUInt24::readYaml(amuse::io::YAMLDocReader& reader) {
  val = reader.readUint32();
}

void LittleUInt24::writeYaml(amuse::io::YAMLDocWriter& writer) const {
  writer.writeUint32(val);
}

std::string_view LittleUInt24::DNAType() { return "amuse::LittleUInt24"sv; }

} // namespace amuse
