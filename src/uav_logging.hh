#pragma once
#include "reactor-cpp/logging.hh"

namespace uav {
    // Runtime log level (set via --log_level at launch):
    //   0 = off
    //   1 = ERROR only
    //   2 = WARN  + ERROR
    //   3 = INFO  + WARN + ERROR  (default)
    //   4 = DEBUG + all
    inline int g_log_level = 3;
}

// Usage: LF_INFO << "message";
// These macros add a runtime check on top of the compile-time level set
// by the LF 'logging:' target property (currently: INFO).
// LF_DEBUG is a no-op unless the project is recompiled with logging: DEBUG.
// Note: logging: DEBUG also enables verbose reactor-cpp internal messages
// (Scheduler, Worker threads) which are unrelated to application logs.
#define LF_DEBUG if (::uav::g_log_level >= 4) reactor::log::Debug()
#define LF_INFO  if (::uav::g_log_level >= 3) reactor::log::Info()
#define LF_WARN  if (::uav::g_log_level >= 2) reactor::log::Warn()
#define LF_ERROR if (::uav::g_log_level >= 1) reactor::log::Error()
