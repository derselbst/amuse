#pragma once

#include <fstream>
#include <string>

#include "athena/DNA.hpp"

namespace athena::io {

class FileReader final : public IStreamReader {
public:
  explicit FileReader(std::string_view path, size_t = 0, bool = true) {
    m_stream.open(std::string(path), std::ios::binary);
    m_error = !m_stream.good();
  }

  size_t readBytesToBuf(void* buf, size_t sz) override {
    if (!m_stream.good()) {
      m_error = true;
      return 0;
    }
    m_stream.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(sz));
    size_t count = static_cast<size_t>(m_stream.gcount());
    if (count < sz && !m_stream.eof())
      m_error = true;
    return count;
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
    m_stream.seekg(offset, dir);
    if (!m_stream.good()) {
      m_error = true;
      return false;
    }
    return true;
  }

  int64_t position() const override { return static_cast<int64_t>(m_stream.tellg()); }

  void close() override { m_stream.close(); }

  bool hasError() const override { return m_error || !m_stream.good(); }

private:
  std::ifstream m_stream;
  bool m_error = false;
};

} // namespace athena::io
