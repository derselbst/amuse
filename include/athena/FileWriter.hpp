#pragma once

#include <fstream>
#include <string>

#include "athena/DNA.hpp"

namespace athena::io {

class FileWriter final : public IStreamWriter {
public:
  explicit FileWriter(std::string_view path, bool truncate = true) {
    std::ios::openmode mode = std::ios::binary | std::ios::out;
    if (truncate)
      mode |= std::ios::trunc;
    else
      mode |= std::ios::in;
    m_stream.open(std::string(path), mode);
    m_error = !m_stream.good();
  }

  void writeBytes(const void* buf, size_t sz) override {
    if (!m_stream.good()) {
      m_error = true;
      return;
    }
    m_stream.write(reinterpret_cast<const char*>(buf), static_cast<std::streamsize>(sz));
    if (!m_stream.good())
      m_error = true;
  }

  bool seek(int64_t offset, SeekOrigin origin) override {
    if (!m_stream.good()) {
      m_error = true;
      return false;
    }
    std::ios_base::seekdir dir = std::ios_base::beg;
    switch (origin) {
    case SeekOrigin::Begin:
      dir = std::ios_base::beg;
      break;
    case SeekOrigin::Current:
      dir = std::ios_base::cur;
      break;
    case SeekOrigin::End:
      dir = std::ios_base::end;
      break;
    }
    m_stream.seekp(offset, dir);
    if (!m_stream.good()) {
      m_error = true;
      return false;
    }
    return true;
  }

  int64_t position() const override { return static_cast<int64_t>(m_stream.tellp()); }

  void close() override { m_stream.close(); }

  bool hasError() const override { return m_error || !m_stream.good(); }

private:
  std::fstream m_stream;
  bool m_error = false;
};

} // namespace athena::io
