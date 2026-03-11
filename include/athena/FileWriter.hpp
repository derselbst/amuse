#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include "athena/IStream.hpp"

namespace athena::io {

// ---------------------------------------------------------------------------
// FileWriter – writes to a file using stdio.
// ---------------------------------------------------------------------------
class FileWriter : public IStreamWriter {
    FILE* m_file = nullptr;

protected:
    void writeUBytesToBuf_raw(const void* buf, uint64_t len) override {
        if (!m_file) { m_hasError = true; return; }
        if (std::fwrite(buf, 1, len, m_file) != len)
            m_hasError = true;
    }

public:
    explicit FileWriter(std::string_view path, bool overwrite = true) {
        const char* mode = overwrite ? "wb" : "ab";
#ifdef _WIN32
        if (path.size() > static_cast<size_t>(INT_MAX)) { m_hasError = true; return; }
        int wlen = MultiByteToWideChar(CP_UTF8, 0, path.data(),
                                       static_cast<int>(path.size()), nullptr, 0);
        std::wstring wpath(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, path.data(),
                            static_cast<int>(path.size()), wpath.data(), wlen);
        const wchar_t* wmode = overwrite ? L"wb" : L"ab";
        m_file = _wfopen(wpath.c_str(), wmode);
#else
        std::string p(path);
        m_file = std::fopen(p.c_str(), mode);
#endif
        if (!m_file)
            m_hasError = true;
    }

    ~FileWriter() override {
        if (m_file)
            std::fclose(m_file);
    }

    void close() {
        if (m_file) {
            if (std::fclose(m_file) != 0)
                m_hasError = true;
            m_file = nullptr;
        }
    }

    // Non-copyable, movable
    FileWriter(const FileWriter&) = delete;
    FileWriter& operator=(const FileWriter&) = delete;
    FileWriter(FileWriter&& o) noexcept : m_file(o.m_file) {
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
