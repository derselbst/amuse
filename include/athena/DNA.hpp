#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "athena/Types.hpp"

namespace athena {

enum class Endian { Big, Little };

enum class SeekOrigin { Begin, Current, End };

namespace io {
class YAMLDocReader;
class YAMLDocWriter;

class IStreamReader {
public:
  virtual ~IStreamReader() = default;
  virtual size_t readBytesToBuf(void* buf, size_t sz) = 0;
  virtual bool seek(int64_t offset, SeekOrigin origin) = 0;
  virtual int64_t position() const = 0;
  virtual void close() {}
  virtual bool hasError() const { return false; }

  std::unique_ptr<uint8_t[]> readUBytes(size_t sz) {
    auto data = std::make_unique<uint8_t[]>(sz);
    if (sz)
      readBytesToBuf(data.get(), sz);
    return data;
  }

  void readUBytesToBuf(void* buf, size_t sz) { readBytesToBuf(buf, sz); }

  uint8_t readUByte() {
    uint8_t val = 0;
    readBytesToBuf(&val, sizeof(val));
    return val;
  }

  uint16_t readUint16Big();
  uint16_t readUint16Little();
  uint32_t readUint32Big();
  uint32_t readUint32Little();
  uint64_t readUint64Big();
};

class IStreamWriter {
public:
  virtual ~IStreamWriter() = default;
  virtual void writeBytes(const void* buf, size_t sz) = 0;
  virtual bool seek(int64_t offset, SeekOrigin origin) = 0;
  virtual int64_t position() const = 0;
  virtual void close() {}
  virtual bool hasError() const { return false; }

  void writeUByte(uint8_t val) { writeBytes(&val, sizeof(val)); }
  void writeUBytes(const void* buf, size_t sz) { writeBytes(buf, sz); }

  void writeUint16Big(uint16_t val);
  void writeUint16Little(uint16_t val);
  void writeUint32Big(uint32_t val);
  void writeUint32Little(uint32_t val);
};

enum class PropType { None };

struct ReadOp {
  using Stream = IStreamReader;
};
struct WriteOp {
  using Stream = IStreamWriter;
};
struct BinarySizeOp {
  using Stream = size_t;
};
struct ReadYamlOp {
  using Stream = YAMLDocReader;
};
struct WriteYamlOp {
  using Stream = YAMLDocWriter;
};

namespace detail {
constexpr bool HostIsLittle() {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
  return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(_WIN32)
  return true;
#else
  return true;
#endif
}

template <typename T>
using ScalarStorage = std::conditional_t<std::is_enum_v<T>, std::underlying_type_t<T>, T>;

template <typename T>
constexpr size_t ScalarSize = sizeof(ScalarStorage<T>);

template <typename T>
ScalarStorage<T> ReadScalarFromBytes(const uint8_t* buf, Endian endian) {
  using Storage = ScalarStorage<T>;
  if constexpr (std::is_floating_point_v<T>) {
    using Unsigned = std::conditional_t<sizeof(Storage) == 4, uint32_t, uint64_t>;
    Unsigned value = 0;
    if (endian == Endian::Big) {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        value = (value << 8) | buf[i];
      }
    } else {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        value |= Unsigned(buf[i]) << (8 * i);
      }
    }
    Storage storage;
    std::memcpy(&storage, &value, sizeof(Storage));
    return storage;
  } else {
    using Unsigned = std::conditional_t<std::is_same_v<Storage, bool>, uint8_t, std::make_unsigned_t<Storage>>;
    Unsigned value = 0;
    if (endian == Endian::Big) {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        value = (value << 8) | buf[i];
      }
    } else {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        value |= Unsigned(buf[i]) << (8 * i);
      }
    }
    return static_cast<Storage>(value);
  }
}

template <typename T>
void WriteScalarToBytes(ScalarStorage<T> value, uint8_t* buf, Endian endian) {
  using Storage = ScalarStorage<T>;
  if constexpr (std::is_floating_point_v<T>) {
    using Unsigned = std::conditional_t<sizeof(Storage) == 4, uint32_t, uint64_t>;
    Unsigned out = 0;
    std::memcpy(&out, &value, sizeof(Storage));
    if (endian == Endian::Big) {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        buf[sizeof(Unsigned) - 1 - i] = uint8_t(out & 0xff);
        out >>= 8;
      }
    } else {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        buf[i] = uint8_t(out & 0xff);
        out >>= 8;
      }
    }
  } else {
    using Unsigned = std::conditional_t<std::is_same_v<Storage, bool>, uint8_t, std::make_unsigned_t<Storage>>;
    Unsigned out = static_cast<Unsigned>(value);
    if (endian == Endian::Big) {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        buf[sizeof(Unsigned) - 1 - i] = uint8_t(out & 0xff);
        out >>= 8;
      }
    } else {
      for (size_t i = 0; i < sizeof(Unsigned); ++i) {
        buf[i] = uint8_t(out & 0xff);
        out >>= 8;
      }
    }
  }
}

inline uint16_t ReadUint16(IStreamReader& reader, Endian endian) {
  uint8_t buf[sizeof(uint16_t)] = {};
  reader.readBytesToBuf(buf, sizeof(buf));
  return ReadScalarFromBytes<uint16_t>(buf, endian);
}

inline uint32_t ReadUint32(IStreamReader& reader, Endian endian) {
  uint8_t buf[sizeof(uint32_t)] = {};
  reader.readBytesToBuf(buf, sizeof(buf));
  return ReadScalarFromBytes<uint32_t>(buf, endian);
}

inline uint64_t ReadUint64(IStreamReader& reader, Endian endian) {
  uint8_t buf[sizeof(uint64_t)] = {};
  reader.readBytesToBuf(buf, sizeof(buf));
  return ReadScalarFromBytes<uint64_t>(buf, endian);
}

inline void WriteUint16(IStreamWriter& writer, uint16_t val, Endian endian) {
  uint8_t buf[sizeof(uint16_t)] = {};
  WriteScalarToBytes<uint16_t>(val, buf, endian);
  writer.writeBytes(buf, sizeof(buf));
}

inline void WriteUint32(IStreamWriter& writer, uint32_t val, Endian endian) {
  uint8_t buf[sizeof(uint32_t)] = {};
  WriteScalarToBytes<uint32_t>(val, buf, endian);
  writer.writeBytes(buf, sizeof(buf));
}

inline void WriteUint64(IStreamWriter& writer, uint64_t val, Endian endian) {
  uint8_t buf[sizeof(uint64_t)] = {};
  WriteScalarToBytes<uint64_t>(val, buf, endian);
  writer.writeBytes(buf, sizeof(buf));
}

template <typename T, Endian E>
struct Value;

template <typename T>
struct IsValue : std::false_type {};

template <typename T, Endian E>
struct IsValue<Value<T, E>> : std::true_type {};

template <size_t N, SeekOrigin O>
struct Seek;

template <typename T>
struct IsSeek : std::false_type {};

template <size_t N, SeekOrigin O>
struct IsSeek<Seek<N, O>> : std::true_type {};

template <typename T>
struct IsDna : std::false_type {};

template <typename T>
inline constexpr bool IsDnaV = std::is_base_of_v<athena::io::DNA<Endian::Big>, std::decay_t<T>> ||
                               std::is_base_of_v<athena::io::DNA<Endian::Little>, std::decay_t<T>>;

template <class Op, class T>
void EnumerateField(typename Op::Stream& stream, const char* name, T& field);

template <Endian DNAE, typename T>
void ReadValue(IStreamReader& stream, T& field) {
  if constexpr (std::is_array_v<T>) {
    for (auto& entry : field)
      ReadValue<DNAE>(stream, entry);
  } else if constexpr (IsValueV<T>) {
    uint8_t buf[ScalarSize<typename T::ValueType>] = {};
    stream.readBytesToBuf(buf, sizeof(buf));
    field = static_cast<typename T::ValueType>(ReadScalarFromBytes<typename T::ValueType>(buf, T::endian));
  } else if constexpr (IsDnaV<T>) {
    field.template Enumerate<ReadOp>(stream);
  } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
    uint8_t buf[ScalarSize<T>] = {};
    stream.readBytesToBuf(buf, sizeof(buf));
    field = static_cast<T>(ReadScalarFromBytes<T>(buf, DNAE));
  }
}

template <Endian DNAE, typename T>
void WriteValue(IStreamWriter& stream, const T& field) {
  if constexpr (std::is_array_v<T>) {
    for (const auto& entry : field)
      WriteValue<DNAE>(stream, entry);
  } else if constexpr (IsValueV<T>) {
    uint8_t buf[ScalarSize<typename T::ValueType>] = {};
    WriteScalarToBytes<typename T::ValueType>(static_cast<typename T::ValueType>(field), buf, T::endian);
    stream.writeBytes(buf, sizeof(buf));
  } else if constexpr (IsDnaV<T>) {
    auto& ref = const_cast<T&>(field);
    ref.template Enumerate<WriteOp>(stream);
  } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
    uint8_t buf[ScalarSize<T>] = {};
    WriteScalarToBytes<T>(static_cast<ScalarStorage<T>>(field), buf, DNAE);
    stream.writeBytes(buf, sizeof(buf));
  }
}

} // namespace detail

template <typename T, Endian E>
struct Value {
  using ValueType = T;
  using StorageType = detail::ScalarStorage<T>;
  static constexpr Endian endian = E;
  StorageType value{};

  constexpr Value() = default;
  constexpr Value(T v) noexcept { *this = v; }

  constexpr operator T() const noexcept { return static_cast<T>(value); }

  constexpr Value& operator=(T v) noexcept {
    value = static_cast<StorageType>(v);
    return *this;
  }

  template <Endian E2>
  constexpr Value(const Value<T, E2>& other) noexcept : value(other.value) {}

  template <Endian E2>
  constexpr Value& operator=(const Value<T, E2>& other) noexcept {
    value = other.value;
    return *this;
  }
};

template <size_t N, SeekOrigin O>
struct Seek {
  std::array<uint8_t, N> pad{};
};

template <Endian DNAE>
struct DNA {
  using Read = ReadOp;
  using Write = WriteOp;
  using BinarySize = BinarySizeOp;
  using ReadYaml = ReadYamlOp;
  using WriteYaml = WriteYamlOp;

  template <typename T, Endian E = DNAE>
  using Value = io::Value<T, E>;
  template <size_t N, SeekOrigin O = SeekOrigin::Current>
  using Seek = io::Seek<N, O>;

  void read(IStreamReader& reader) { static_cast<void>(this->template Enumerate<Read>(reader)); }
  void write(IStreamWriter& writer) { static_cast<void>(this->template Enumerate<Write>(writer)); }
  void binarySize(size_t& sz) { static_cast<void>(this->template Enumerate<BinarySize>(sz)); }
  void read(YAMLDocReader& reader) { static_cast<void>(this->template Enumerate<ReadYaml>(reader)); }
  void write(YAMLDocWriter& writer) { static_cast<void>(this->template Enumerate<WriteYaml>(writer)); }
};

template <Endian DNAE>
struct DNAVYaml : DNA<DNAE> {};

template <PropType>
struct Read {
  template <typename T, Endian DNAE>
  static void Do(const T&, T& out, IStreamReader& reader) {
    detail::ReadValue<DNAE>(reader, out);
  }
};

template <PropType>
struct Write {
  template <typename T, Endian DNAE>
  static void Do(const T&, const T& out, IStreamWriter& writer) {
    detail::WriteValue<DNAE>(writer, out);
  }
};

} // namespace io
} // namespace athena

#define AT_SPECIALIZE_PARMS(...)

#define AT_DECL_DNA                                                                                                \
  template <class Op>                                                                                              \
  void Enumerate(typename Op::Stream& stream);                                                                     \
  static std::string_view DNAType() { return {}; }

#define AT_DECL_DNA_YAML AT_DECL_DNA

#define AT_DECL_DNA_YAMLV                                                                                          \
  template <class Op>                                                                                              \
  void Enumerate(typename Op::Stream& stream) { EnumerateSoundMacroCmd(*this, stream); }                           \
  static std::string_view DNAType() { return {}; }

#define AT_DECL_EXPLICIT_DNA_YAML                                                                                  \
  template <class Op>                                                                                              \
  void Enumerate(typename Op::Stream& stream);                                                                     \
  static std::string_view DNAType();

#define AT_DECL_EXPLICIT_DNA_YAMLV AT_DECL_EXPLICIT_DNA_YAML

namespace athena::io::detail {

template <typename T, Endian E>
inline constexpr bool IsValueV = IsValue<T>::value;

template <typename T>
inline constexpr bool IsSeekV = IsSeek<T>::value;

template <typename T>
struct SeekSize;

template <size_t N, SeekOrigin O>
struct SeekSize<Seek<N, O>> {
  static constexpr size_t Value = N;
};

template <class Op, class T>
void EnumerateArray(typename Op::Stream& stream, const char* name, T& field) {
  for (auto& entry : field)
    EnumerateField<Op>(stream, name, entry);
}

template <class Op, class T>
void EnumerateArray(typename Op::Stream& stream, const char* name, T* field, size_t size) {
  for (size_t i = 0; i < size; ++i)
    EnumerateField<Op>(stream, name, field[i]);
}

template <typename T>
inline size_t BinarySizeOf(const T& field) {
  if constexpr (std::is_array_v<T>) {
    size_t sz = 0;
    for (const auto& entry : field)
      sz += BinarySizeOf(entry);
    return sz;
  } else if constexpr (IsSeekV<T>) {
    return SeekSize<T>::Value;
  } else if constexpr (IsValueV<T>) {
    return sizeof(typename T::StorageType);
  } else if constexpr (IsDnaV<T>) {
    size_t sz = 0;
    auto& ref = const_cast<T&>(field);
    ref.template Enumerate<BinarySizeOp>(sz);
    return sz;
  } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
    return sizeof(ScalarStorage<T>);
  } else {
    return sizeof(T);
  }
}

template <class Op, class T>
void EnumerateField(typename Op::Stream& stream, const char* name, T& field) {
  if constexpr (std::is_same_v<Op, ReadOp>) {
    if constexpr (std::is_array_v<T>) {
      EnumerateArray<Op>(stream, name, field);
    } else if constexpr (IsSeekV<T>) {
      stream.seek(static_cast<int64_t>(SeekSize<T>::Value), SeekOrigin::Current);
    } else if constexpr (IsValueV<T>) {
      uint8_t buf[ScalarSize<typename T::ValueType>] = {};
      stream.readBytesToBuf(buf, sizeof(buf));
      field = static_cast<typename T::ValueType>(ReadScalarFromBytes<typename T::ValueType>(buf, T::endian));
    } else if constexpr (IsDnaV<T>) {
      field.template Enumerate<ReadOp>(stream);
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
      uint8_t buf[ScalarSize<T>] = {};
      stream.readBytesToBuf(buf, sizeof(buf));
      field = static_cast<T>(ReadScalarFromBytes<T>(buf, Endian::Big));
    }
  } else if constexpr (std::is_same_v<Op, WriteOp>) {
    if constexpr (std::is_array_v<T>) {
      EnumerateArray<Op>(stream, name, field);
    } else if constexpr (IsSeekV<T>) {
      std::array<uint8_t, SeekSize<T>::Value> zeros{};
      stream.writeBytes(zeros.data(), zeros.size());
    } else if constexpr (IsValueV<T>) {
      uint8_t buf[ScalarSize<typename T::ValueType>] = {};
      WriteScalarToBytes<typename T::ValueType>(static_cast<typename T::ValueType>(field), buf, T::endian);
      stream.writeBytes(buf, sizeof(buf));
    } else if constexpr (IsDnaV<T>) {
      field.template Enumerate<WriteOp>(stream);
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
      uint8_t buf[ScalarSize<T>] = {};
      WriteScalarToBytes<T>(static_cast<ScalarStorage<T>>(field), buf, Endian::Big);
      stream.writeBytes(buf, sizeof(buf));
    }
  } else if constexpr (std::is_same_v<Op, BinarySizeOp>) {
    stream += BinarySizeOf(field);
  } else if constexpr (std::is_same_v<Op, ReadYamlOp>) {
    if constexpr (IsSeekV<T>) {
      return;
    } else {
      stream.enumerate(name, field);
    }
  } else if constexpr (std::is_same_v<Op, WriteYamlOp>) {
    if constexpr (IsSeekV<T>) {
      return;
    } else {
      stream.enumerate(name, field);
    }
  }
}

} // namespace athena::io::detail

inline uint16_t athena::io::IStreamReader::readUint16Big() {
  return detail::ReadUint16(*this, Endian::Big);
}

inline uint16_t athena::io::IStreamReader::readUint16Little() {
  return detail::ReadUint16(*this, Endian::Little);
}

inline uint32_t athena::io::IStreamReader::readUint32Big() {
  return detail::ReadUint32(*this, Endian::Big);
}

inline uint32_t athena::io::IStreamReader::readUint32Little() {
  return detail::ReadUint32(*this, Endian::Little);
}

inline uint64_t athena::io::IStreamReader::readUint64Big() {
  return detail::ReadUint64(*this, Endian::Big);
}

inline void athena::io::IStreamWriter::writeUint16Big(uint16_t val) {
  detail::WriteUint16(*this, val, Endian::Big);
}

inline void athena::io::IStreamWriter::writeUint16Little(uint16_t val) {
  detail::WriteUint16(*this, val, Endian::Little);
}

inline void athena::io::IStreamWriter::writeUint32Big(uint32_t val) {
  detail::WriteUint32(*this, val, Endian::Big);
}

inline void athena::io::IStreamWriter::writeUint32Little(uint32_t val) {
  detail::WriteUint32(*this, val, Endian::Little);
}
