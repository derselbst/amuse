#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include "athena/IStream.hpp"

namespace athena::io {

// ---------------------------------------------------------------------------
// FileReader – reads from a file using stdio.
// The extra constructor parameters (blockSize, overwrite) are accepted for
// API compatibility but are ignored.
// ---------------------------------------------------------------------------
class FileReader : public IStreamReader {
    FILE* m_file = nullptr;

protected:
    uint64_t readUBytesToBuf_raw(void* buf, uint64_t len) override {
        if (!m_file) { m_hasError = true; return 0; }
        return static_cast<uint64_t>(std::fread(buf, 1, len, m_file));
    }

public:
    explicit FileReader(std::string_view path,
                        int32_t  /*blockSize*/ = 32 * 1024,
                        bool     /*overwrite*/  = false) {
#ifdef _WIN32
        // Convert UTF-8 path to wide string for Windows
        if (path.size() > static_cast<size_t>(INT_MAX)) { m_hasError = true; return; }
        int wlen = MultiByteToWideChar(CP_UTF8, 0, path.data(),
                                       static_cast<int>(path.size()), nullptr, 0);
        std::wstring wpath(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, path.data(),
                            static_cast<int>(path.size()), wpath.data(), wlen);
        m_file = _wfopen(wpath.c_str(), L"rb");
#else
        std::string p(path);
        m_file = std::fopen(p.c_str(), "rb");
#endif
        if (!m_file)
            m_hasError = true;
    }

    ~FileReader() override {
        if (m_file)
            std::fclose(m_file);
    }

    void close() {
        if (m_file) {
            std::fclose(m_file);
            m_file = nullptr;
        }
    }

    // Non-copyable, movable
    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;
    FileReader(FileReader&& o) noexcept : m_file(o.m_file) {
        m_hasError = o.m_hasError;
        o.m_file = nullptr;
    }

    void seek(int64_t off, athena::SeekOrigin origin) override {
        if (!m_file) { m_hasError = true; return; }
        int whence = SEEK_SET;
        switch (origin) {
        case athena::SeekOrigin::Begin:   whence = SEEK_SET; break;
        case athena::SeekOrigin::Current: whence = SEEK_CUR; break;
        case athena::SeekOrigin::End:     whence = SEEK_END; break;
        }
#ifdef _WIN32
        if (_fseeki64(m_file, static_cast<__int64>(off), whence) != 0)
            m_hasError = true;
#else
        if (fseeko(m_file, static_cast<off_t>(off), whence) != 0)
            m_hasError = true;
#endif
    }

    int64_t position() const override {
        if (!m_file) return -1;
#ifdef _WIN32
        return static_cast<int64_t>(_ftelli64(m_file));
#else
        return static_cast<int64_t>(ftello(m_file));
#endif
    }
};

} // namespace athena::io
