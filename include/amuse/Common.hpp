#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "athena/Types.hpp"
#include "athena/DNA.hpp"
#include <logvisor/logvisor.hpp>

#ifndef _WIN32
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <nowide/stackstring.hpp>
#endif

#if __SWITCH__
#undef _S

#include "switch_math.hpp"
#endif

#undef min
#undef max

constexpr float NativeSampleRate = 32000.0f;

namespace amuse {
struct NameDB;

using BigDNA = athena::io::DNA<std::endian::big>;
using LittleDNA = athena::io::DNA<std::endian::little>;
using BigDNAV = athena::io::DNAVYaml<std::endian::big>;
using LittleDNAV = athena::io::DNAVYaml<std::endian::little>;

// Bring Value<> and Seek<> into the amuse namespace so that DNA-derived
// structs can reference them without full qualification (matching the
// original athena API surface).
using athena::io::Value;
using athena::io::Seek;

/** Common ID structure statically tagging
 *  SoundMacros, Tables, Keymaps, Layers, Samples, SFX, Songs */
struct ObjectId {
  uint16_t id = 0xffff;
  constexpr ObjectId() noexcept = default;
  constexpr ObjectId(uint16_t idIn) noexcept : id(idIn) {}
  constexpr ObjectId& operator=(uint16_t idIn) noexcept {
    id = idIn;
    return *this;
  }
  constexpr bool operator==(const ObjectId& other) const noexcept { return id == other.id; }
  constexpr bool operator!=(const ObjectId& other) const noexcept { return !operator==(other); }
  constexpr bool operator<(const ObjectId& other) const noexcept { return id < other.id; }
  constexpr bool operator>(const ObjectId& other) const noexcept { return id > other.id; }
  static thread_local NameDB* CurNameDB;
};
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) ObjectIdDNA : BigDNA {
  AT_DECL_EXPLICIT_DNA_YAML
  void _read(athena::io::YAMLDocReader& r);
  void _write(athena::io::YAMLDocWriter& w);
  ObjectId id;
  constexpr ObjectIdDNA() noexcept = default;
  constexpr ObjectIdDNA(ObjectId idIn) noexcept : id(idIn) {}
  constexpr operator ObjectId() const noexcept { return id; }
};

#define DECL_ID_TYPE(type)                                                                                             \
  struct type : ObjectId {                                                                                             \
    using ObjectId::ObjectId;                                                                                          \
    constexpr type() noexcept = default;                                                                               \
    constexpr type(const ObjectId& idIn) noexcept : ObjectId(idIn) {}                                                  \
    static thread_local NameDB* CurNameDB;                                                                             \
  };                                                                                                                   \
  template <std::endian DNAEn>                                                                                      \
  struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) type##DNA : BigDNA {                         \
    AT_DECL_EXPLICIT_DNA_YAML                                                                                          \
    void _read(athena::io::YAMLDocReader& r);                                                                          \
    void _write(athena::io::YAMLDocWriter& w);                                                                         \
    type id;                                                                                                           \
    constexpr type##DNA() noexcept = default;                                                                          \
    constexpr type##DNA(type idIn) noexcept : id(idIn) {}                                                              \
    constexpr operator type() const noexcept { return id; }                                                            \
  };
DECL_ID_TYPE(SoundMacroId)
DECL_ID_TYPE(SampleId)
DECL_ID_TYPE(TableId)
DECL_ID_TYPE(KeymapId)
DECL_ID_TYPE(LayersId)
DECL_ID_TYPE(SongId)
DECL_ID_TYPE(SFXId)
DECL_ID_TYPE(GroupId)

/* MusyX has object polymorphism between Keymaps and Layers when
 * referenced by a song group's page object. When the upper bit is set,
 * this indicates a layer type. */
template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) PageObjectIdDNA : BigDNA {
  AT_DECL_EXPLICIT_DNA_YAML
  void _read(athena::io::YAMLDocReader& r);
  void _write(athena::io::YAMLDocWriter& w);
  ObjectId id;
  constexpr PageObjectIdDNA() noexcept = default;
  constexpr PageObjectIdDNA(ObjectId idIn) noexcept : id(idIn) {}
  constexpr operator ObjectId() const noexcept { return id; }
};

struct SoundMacroStep {
  uint16_t step = 0;
  constexpr operator uint16_t() const noexcept { return step; }
  constexpr SoundMacroStep() noexcept = default;
  constexpr SoundMacroStep(uint16_t idIn) noexcept : step(idIn) {}
  constexpr SoundMacroStep& operator=(uint16_t idIn) noexcept {
    step = idIn;
    return *this;
  }
};

template <std::endian DNAEn>
struct AT_SPECIALIZE_PARMS(std::endian::big, std::endian::little) SoundMacroStepDNA : BigDNA {
  AT_DECL_EXPLICIT_DNA_YAML
  SoundMacroStep step;
  constexpr SoundMacroStepDNA() noexcept = default;
  constexpr SoundMacroStepDNA(SoundMacroStep idIn) noexcept : step(idIn) {}
  constexpr operator SoundMacroStep() const noexcept { return step; }
};

struct LittleUInt24 : LittleDNA {
  AT_DECL_EXPLICIT_DNA_YAML
  uint32_t val{};
  constexpr operator uint32_t() const noexcept { return val; }
  constexpr LittleUInt24() noexcept = default;
  constexpr LittleUInt24(uint32_t valIn) noexcept : val(valIn) {}
  constexpr LittleUInt24& operator=(uint32_t valIn) noexcept {
    val = valIn;
    return *this;
  }
};

template <class T>
using ObjToken = std::shared_ptr<T>;

template <class T>
using IObjToken = std::shared_ptr<T>;

template <class Tp, class... _Args>
inline std::shared_ptr<Tp> MakeObj(_Args&&... args) {
  return std::make_shared<Tp>(std::forward<_Args>(args)...);
}

#ifndef PRISize
#ifdef _MSC_VER
#define PRISize "Iu"
#else
#define PRISize "zu"
#endif
#endif

#if _WIN32
static inline int Mkdir(const char* path, int) {
  const nowide::wstackstring str(path);
  return _wmkdir(str.get());
}

using Sstat = struct ::_stat64;
static inline int Stat(const char* path, Sstat* statout) {
  const nowide::wstackstring wpath(path);
  return _wstat64(wpath.get(), statout);
}
#else
static inline int Mkdir(const char* path, mode_t mode) { return mkdir(path, mode); }

typedef struct stat Sstat;
static inline int Stat(const char* path, Sstat* statout) { return stat(path, statout); }
#endif

#if _WIN32
int Rename(const char* oldpath, const char* newpath);
#else
inline int Rename(const char* oldpath, const char* newpath) { return rename(oldpath, newpath); }
#endif

inline int CompareCaseInsensitive(const char* a, const char* b) {
#if _WIN32
  return _stricmp(a, b);
#else
  return strcasecmp(a, b);
#endif
}

template <typename T>
constexpr T ClampFull(float in) noexcept {
  if (std::is_floating_point<T>()) {
    return in;
  } else {
    constexpr T MAX = std::numeric_limits<T>::max();
    constexpr T MIN = std::numeric_limits<T>::min();

    if (in < MIN)
      return MIN;
    else if (in > static_cast<float>(MAX))
      return MAX;
    else
      return in;
  }
}

inline const char* StrRChr(const char* str, char ch) {
  return strrchr(str, ch);
}

inline char* StrRChr(char* str, char ch) {
  return strrchr(str, ch);
}

inline int FSeek(FILE* fp, int64_t offset, int whence) {
#if _WIN32
  return _fseeki64(fp, offset, whence);
#elif __APPLE__ || __FreeBSD__
  return fseeko(fp, offset, whence);
#else
  return fseeko64(fp, offset, whence);
#endif
}

inline int64_t FTell(FILE* fp) {
#if _WIN32
  return _ftelli64(fp);
#elif __APPLE__ || __FreeBSD__
  return ftello(fp);
#else
  return ftello64(fp);
#endif
}

inline FILE* FOpen(const char* path, const char* mode) {
#if _WIN32
  const nowide::wstackstring wpath(path);
  const nowide::wshort_stackstring wmode(mode);
  FILE* fp = _wfopen(wpath.get(), wmode.get());
  if (!fp)
    return nullptr;
#else
  FILE* fp = fopen(path, mode);
  if (!fp)
    return nullptr;
#endif
  return fp;
}

inline void Unlink(const char* file) {
#if _WIN32
  const nowide::wstackstring wfile(file);
  _wunlink(wfile.get());
#else
  unlink(file);
#endif
}

bool Copy(const char* from, const char* to);

/** Versioned data format to interpret */
enum class DataFormat { GCN, N64, PC };

/** Meta-type for selecting GameCube (MusyX 2.0) data formats */
struct GCNDataTag {};

/** Meta-type for selecting N64 (MusyX 1.0) data formats */
struct N64DataTag {};

/** Meta-type for selecting PC (MusyX 1.0) data formats */
struct PCDataTag {};

template <class T>
inline std::vector<std::pair<typename T::key_type, std::reference_wrapper<typename T::mapped_type>>>
SortUnorderedMap(T& um) {
  std::vector<std::pair<typename T::key_type, std::reference_wrapper<typename T::mapped_type>>> ret;
  ret.reserve(um.size());
  for (auto& p : um)
    ret.emplace_back(p.first, p.second);
  std::sort(ret.begin(), ret.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  return ret;
}

template <class T>
inline std::vector<std::pair<typename T::key_type, std::reference_wrapper<const typename T::mapped_type>>>
SortUnorderedMap(const T& um) {
  std::vector<std::pair<typename T::key_type, std::reference_wrapper<const typename T::mapped_type>>> ret;
  ret.reserve(um.size());
  for (const auto& p : um)
    ret.emplace_back(p.first, p.second);
  std::sort(ret.begin(), ret.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  return ret;
}

template <class T>
inline std::vector<typename T::key_type> SortUnorderedSet(T& us) {
  std::vector<typename T::key_type> ret;
  ret.reserve(us.size());
  for (auto& p : us)
    ret.emplace_back(p);
  std::sort(ret.begin(), ret.end());
  return ret;
}

template <class T>
inline std::vector<typename T::key_type> SortUnorderedSet(const T& us) {
  std::vector<typename T::key_type> ret;
  ret.reserve(us.size());
  for (const auto& p : us)
    ret.emplace_back(p);
  std::sort(ret.begin(), ret.end());
  return ret;
}
} // namespace amuse

namespace std {
#define DECL_ID_HASH(type)                                                                                             \
  template <>                                                                                                          \
  struct hash<amuse::type> {                                                                                           \
    size_t operator()(const amuse::type& val) const noexcept { return val.id; }                                        \
  };
DECL_ID_HASH(ObjectId)
DECL_ID_HASH(SoundMacroId)
DECL_ID_HASH(SampleId)
DECL_ID_HASH(TableId)
DECL_ID_HASH(KeymapId)
DECL_ID_HASH(LayersId)
DECL_ID_HASH(SongId)
DECL_ID_HASH(SFXId)
DECL_ID_HASH(GroupId)

} // namespace std

namespace amuse {
struct NameDB {
  enum class Type { SoundMacro, Table, Keymap, Layer, Song, SFX, Group, Sample };

  std::unordered_map<std::string, ObjectId> m_stringToId;
  std::unordered_map<ObjectId, std::string> m_idToString;

  ObjectId generateId(Type tp) const;
  static std::string generateName(ObjectId id, Type tp);
  std::string generateDefaultName(Type tp) const;
  std::string_view registerPair(std::string_view str, ObjectId id);
  std::string_view resolveNameFromId(ObjectId id) const;
  ObjectId resolveIdFromName(std::string_view str) const;
  void remove(ObjectId id);
  void rename(ObjectId id, std::string_view str);
};
} // namespace amuse

FMT_CUSTOM_FORMATTER(amuse::ObjectId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::SoundMacroId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::SampleId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::TableId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::KeymapId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::LayersId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::SongId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::SFXId, "{:04X}", obj.id)
FMT_CUSTOM_FORMATTER(amuse::GroupId, "{:04X}", obj.id)
