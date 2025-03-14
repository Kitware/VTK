#ifndef DIY_LOG_HPP
#define DIY_LOG_HPP

#ifndef DIY_USE_SPDLOG

#include <memory>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)

namespace diy
{

namespace spd
{
    struct logger
    {
        // logger.info(cppformat_string, arg1, arg2, arg3, ...) call style
        template <typename... Args> void trace(const char*, const Args&...)    {}
        template <typename... Args> void debug(const char*, const Args&...)    {}
        template <typename... Args> void info(const char*, const Args&...)     {}
        template <typename... Args> void warn(const char*, const Args&...)     {}
        template <typename... Args> void error(const char*, const Args&...)    {}
        template <typename... Args> void critical(const char*, const Args&...) {}
    };
}

inline
std::shared_ptr<spd::logger>
get_logger()
{
    return std::make_shared<spd::logger>();
}

inline
std::shared_ptr<spd::logger>
create_logger(std::string)
{
    return std::make_shared<spd::logger>();
}

template<class... Args>
std::shared_ptr<spd::logger>
set_logger(Args...)
{
    return std::make_shared<spd::logger>();
}

}   // diy

#else // DIY_USE_SPDLOG

#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

namespace diy
{

namespace spd = ::spdlog;

inline
std::shared_ptr<spd::logger>
get_logger()
{
    auto log = spd::get("diy");
    if (!log)
    {
        auto null_sink = std::make_shared<spd::sinks::null_sink_mt> ();
        log = std::make_shared<spd::logger>("null_logger", null_sink);
    }
    return log;
}

inline
std::shared_ptr<spd::logger>
create_logger(std::string log_level)
{
    auto log = spd::stderr_logger_mt("diy");
    int lvl = spd::level::from_str(log_level);
    log->set_level(static_cast<spd::level::level_enum>(lvl));
    return log;
}

template<class... Args>
std::shared_ptr<spd::logger>
set_logger(Args... args)
{
    auto log = std::make_shared<spdlog::logger>("diy", args...);
    return log;
}

}   // diy
#endif


#endif // DIY_LOG_HPP
