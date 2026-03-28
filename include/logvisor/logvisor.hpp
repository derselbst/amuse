#pragma once

/**
 * Minimal logvisor compatibility shim.
 *
 * Provides just enough of the logvisor API for the amuse library
 * to compile without the real logvisor dependency.
 */

#include <cstdio>
#include <cstdlib>
#include <fmt/format.h>

namespace logvisor {

enum Level { Info, Warning, Error, Fatal };

struct Module {
  const char* m_name;
  explicit Module(const char* name) : m_name(name) {}

  template <typename S, typename... Args>
  void report(Level level, const S& format, Args&&... args) const {
    const char* prefix = "INFO";
    switch (level) {
    case Info:    prefix = "INFO"; break;
    case Warning: prefix = "WARNING"; break;
    case Error:   prefix = "ERROR"; break;
    case Fatal:   prefix = "FATAL"; break;
    }
    /* Use vformat with runtime string to avoid constexpr format-string issues */
    auto sv = static_cast<fmt::string_view>(format);
    std::string msg = fmt::vformat(sv, fmt::make_format_args(args...));
    fprintf(stderr, "[%s] %s: %s\n", prefix, m_name, msg.c_str());
    if (level == Fatal)
      abort();
  }
};

inline void RegisterConsoleLogger() {}
inline void RegisterStandardExceptions() {}

} // namespace logvisor

/**
 * FMT_CUSTOM_FORMATTER compatibility macro.
 *
 * Provides a fmt::formatter specialization for custom types,
 * compatible with fmt v9+.
 */
#ifndef FMT_CUSTOM_FORMATTER
#define FMT_CUSTOM_FORMATTER(type, fmtstr, expr)                               \
  template <>                                                                  \
  struct fmt::formatter<type> : fmt::formatter<decltype(std::declval<type>().id)> { \
    template <typename FormatContext>                                           \
    auto format(const type& obj, FormatContext& ctx) const {                   \
      return fmt::formatter<decltype(obj.id)>::format(expr, ctx);              \
    }                                                                          \
  };
#endif
