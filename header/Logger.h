#pragma once
#include <cassert>

#define FMT_HEADER_ONLY
#include "../external/fmt/include/fmt/core.h"

class Logger
{
public:
    template <typename FmtStr, typename... Args>
    static void Info(const FmtStr& fmt, const Args&... args);

    template <typename FmtStr, typename... Args>
    static void Warning(const FmtStr& fmt, const Args&... args);

    template <typename FmtStr, typename... Args>
    static void Error(const FmtStr& fmt, const Args&... args);

    template <typename FmtStr, typename... Args>
    static void Critical(const FmtStr& fmt, const Args&... args);

private:
    static constexpr auto magenta = "\033[35m";
    static constexpr auto green = "\033[32m";
    static constexpr auto yellow = "\033[33m";
    static constexpr auto red = "\033[31m";
    static constexpr auto reset = "\033[0m";
};

template <typename FmtStr, typename... Args>
inline void Logger::Info(const FmtStr& fmt, const Args&... args)
{
    printf("[%sInfo%s] ", green, reset);
    fmt::print(fmt, args...);
    printf("\n");
}

template <typename FmtStr, typename... Args>
inline void Logger::Warning(const FmtStr& fmt, const Args&... args)
{
    printf("[%sWarning%s] ", yellow, reset);
    fmt::print(fmt, args...);
    printf("\n");
}

template <typename FmtStr, typename... Args>
inline void Logger::Error(const FmtStr& fmt, const Args&... args)
{
    printf("[%sError%s] ", red, reset);
    fmt::print(fmt, args...);
    printf("\n");
}

template <typename FmtStr, typename... Args>
inline void Logger::Critical(const FmtStr& fmt, const Args&... args)
{
    printf("[%sCritical Error%s] ", magenta, reset);
    fmt::print(fmt, args...);
    printf("\n");
    assert(false);
}
