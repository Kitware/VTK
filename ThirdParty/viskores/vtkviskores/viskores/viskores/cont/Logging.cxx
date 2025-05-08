//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Logging.h>

#ifdef VISKORES_ENABLE_LOGGING

// disable MSVC warnings in loguru.hpp
#ifdef VISKORES_MSVC
#pragma warning(push)
#pragma warning(disable : 4722)
#endif // VISKORES_MSVC

#define LOGURU_USE_ANONYMOUS_NAMESPACE
#define LOGURU_WITH_STREAMS 1
#define LOGURU_SCOPE_TIME_PRECISION 6

#include <viskores/thirdparty/loguru/viskoresloguru/loguru.cpp>

#ifdef VISKORES_MSVC
#pragma warning(pop)
#endif // VISKORES_MSVC

#endif // VISKORES_ENABLE_LOGGING

#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace
{

// This won't be needed under C++14, as strongly typed enums are automatically
// hashed then. But for now...
struct LogHasher
{
  std::size_t operator()(viskores::cont::LogLevel level) const
  {
    return static_cast<std::size_t>(level);
  }
};

using LevelMapType = std::unordered_map<viskores::cont::LogLevel, std::string, LogHasher>;

static bool Initialized = false;
static LevelMapType LogLevelNames;

void setLogLevelName(viskores::cont::LogLevel level, const std::string& name) noexcept
{
  // if the log has been initialized, prevent modifications of the name map
  // to prevent race conditions.
  if (!Initialized)
  {
    LogLevelNames[level] = name;
  }
}

// Throws std::out_of_range if level not found.
const std::string& getLogLevelName(viskores::cont::LogLevel level)
{
  const LevelMapType& names = LogLevelNames;
  return names.at(static_cast<viskores::cont::LogLevel>(level));
}

#ifdef VISKORES_ENABLE_LOGGING
const char* verbosityToNameCallback(loguru::Verbosity v)
{
  try
  {
    // Calling c_str on const string&.
    return getLogLevelName(static_cast<viskores::cont::LogLevel>(v)).c_str();
  }
  catch (std::out_of_range&)
  {
    return nullptr;
  }
}

loguru::Verbosity nameToVerbosityCallback(const char* name)
{
  const LevelMapType& names = LogLevelNames;
  for (auto& kv : names)
  {
    if (kv.second == name)
    {
      return static_cast<loguru::Verbosity>(kv.first);
    }
  }
  return loguru::Verbosity_INVALID;
}
#endif // VISKORES_ENABLE_LOGGING

} // end anon namespace

namespace viskores
{
namespace cont
{

VISKORES_CONT
void InitLogging(int& argc,
                 char* argv[],
                 const std::string& loggingFlag,
                 const std::string& loggingEnv)
{
  SetLogLevelName(viskores::cont::LogLevel::Off, "Off");
  SetLogLevelName(viskores::cont::LogLevel::Fatal, "FATL");
  SetLogLevelName(viskores::cont::LogLevel::Error, "ERR");
  SetLogLevelName(viskores::cont::LogLevel::Warn, "WARN");
  SetLogLevelName(viskores::cont::LogLevel::Info, "Info");
  SetLogLevelName(viskores::cont::LogLevel::DevicesEnabled, "Dev");
  SetLogLevelName(viskores::cont::LogLevel::Perf, "Perf");
  SetLogLevelName(viskores::cont::LogLevel::MemCont, "MemC");
  SetLogLevelName(viskores::cont::LogLevel::MemExec, "MemE");
  SetLogLevelName(viskores::cont::LogLevel::MemTransfer, "MemT");
  SetLogLevelName(viskores::cont::LogLevel::KernelLaunches, "Kern");
  SetLogLevelName(viskores::cont::LogLevel::Cast, "Cast");


#ifdef VISKORES_ENABLE_LOGGING
  if (!Initialized)
  {
    loguru::set_verbosity_to_name_callback(&verbosityToNameCallback);
    loguru::set_name_to_verbosity_callback(&nameToVerbosityCallback);

    const char* envLevel = std::getenv(loggingEnv.c_str());
    if (envLevel != nullptr)
    {
      SetStderrLogLevel(envLevel);
    }
    else
    {
      // Set the default log level to warning
      SetStderrLogLevel(viskores::cont::LogLevel::Warn);
    }
    loguru::init(argc, argv, loggingFlag.c_str());
  }
#else  // VISKORES_ENABLE_LOGGING
  (void)argc;
  (void)argv;
  (void)loggingFlag;
#endif // VISKORES_ENABLE_LOGGING

  // Prevent LogLevelNames from being modified (makes thread safety easier)
  Initialized = true;
}

void InitLogging()
{
  int argc = 1;
  char dummy[1] = { '\0' };
  char* argv[2] = { dummy, nullptr };
  InitLogging(argc, argv);
}

VISKORES_CONT
void SetStderrLogLevel(const char* verbosity)
{
#ifdef VISKORES_ENABLE_LOGGING
  loguru::g_stderr_verbosity = loguru::get_verbosity_from_name(verbosity);
#else  // VISKORES_ENABLE_LOGGING
  (void)verbosity;
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT
void SetStderrLogLevel(LogLevel level)
{
#ifdef VISKORES_ENABLE_LOGGING
  loguru::g_stderr_verbosity = static_cast<loguru::Verbosity>(level);
#else  // VISKORES_ENABLE_LOGGING
  (void)level;
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT
viskores::cont::LogLevel GetStderrLogLevel()
{
#ifdef VISKORES_ENABLE_LOGGING
  return static_cast<viskores::cont::LogLevel>(loguru::g_stderr_verbosity);
#else  // VISKORES_ENABLE_LOGGING
  return viskores::cont::LogLevel::Off;
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT
void SetLogThreadName(const std::string& name)
{
#ifdef VISKORES_ENABLE_LOGGING
  loguru::set_thread_name(name.c_str());
#else  // VISKORES_ENABLE_LOGGING
  (void)name;
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT
std::string GetLogThreadName()
{
#ifdef VISKORES_ENABLE_LOGGING
  char buffer[128];
  loguru::get_thread_name(buffer, 128, false);
  return buffer;
#else  // VISKORES_ENABLE_LOGGING
  return "N/A";
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT
std::string GetLogErrorContext()
{
#ifdef VISKORES_ENABLE_LOGGING
  auto ctx = loguru::get_error_context();
  return ctx.c_str();
#else  // VISKORES_ENABLE_LOGGING
  return "N/A";
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT
std::string GetStackTrace(viskores::Int32 skip)
{
  (void)skip; // unsed when logging disabled.

  std::string result;

#ifdef VISKORES_ENABLE_LOGGING
  result = loguru::stacktrace(skip + 2).c_str();
#endif // VISKORES_ENABLE_LOGGING

  if (result.empty())
  {
    result = "(Stack trace unavailable)";
  }

  return result;
}


namespace
{
/// Convert a size in bytes to a human readable string (e.g. "64 bytes",
/// "1.44 MiB", "128 GiB", etc). @a prec controls the fixed point precision
/// of the stringified number.
inline VISKORES_CONT std::string HumanSize(viskores::UInt64 bytes, int prec = 2)
{
  viskores::UInt64 current = bytes;
  viskores::UInt64 previous = bytes;

  constexpr static const char* units[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "PiB" };

  //this way reduces the number of float divisions we do
  int i = 0;
  while (current > 1024)
  {
    previous = current;
    current = current >> 10; //shift up by 1024
    ++i;
  }

  const double bytesf =
    (i == 0) ? static_cast<double>(previous) : static_cast<double>(previous) / 1024.;
  std::ostringstream out;
  out << std::fixed << std::setprecision(prec) << bytesf << " " << units[i];
  return out.str();
}
}

VISKORES_CONT
std::string GetHumanReadableSize(viskores::UInt64 bytes, int prec)
{
  return HumanSize(bytes, prec);
}

VISKORES_CONT
std::string GetSizeString(viskores::UInt64 bytes, int prec)
{
  return HumanSize(bytes, prec) + " (" + std::to_string(bytes) + " bytes)";
}

VISKORES_CONT
void SetLogLevelName(LogLevel level, const std::string& name)
{
  if (Initialized)
  {
    VISKORES_LOG_F(LogLevel::Error, "SetLogLevelName called after InitLogging.");
    return;
  }
  setLogLevelName(level, name);
}

VISKORES_CONT
std::string GetLogLevelName(LogLevel level)
{
#ifdef VISKORES_ENABLE_LOGGING
  { // Check loguru lookup first:
    const char* name = loguru::get_verbosity_name(static_cast<loguru::Verbosity>(level));
    if (name)
    {
      return name;
    }
  }
#else
  {
    try
    {
      return getLogLevelName(level);
    }
    catch (std::out_of_range&)
    { /* fallthrough */
    }
  }
#endif

  // Create a string from the numeric value otherwise:
  using T = std::underlying_type<LogLevel>::type;
  return std::to_string(static_cast<T>(level));
}

VISKORES_CONT std::string TypeToString(const std::type_info& t)
{
#ifdef VISKORES_ENABLE_LOGGING
  return loguru::demangle(t.name()).c_str();
#else  // VISKORES_ENABLE_LOGGING
  return t.name();
#endif // VISKORES_ENABLE_LOGGING
}

VISKORES_CONT std::string TypeToString(const std::type_index& t)
{
#ifdef VISKORES_ENABLE_LOGGING
  return loguru::demangle(t.name()).c_str();
#else  // VISKORES_ENABLE_LOGGING
  return t.name();
#endif // VISKORES_ENABLE_LOGGING
}

#ifdef VISKORES_ENABLE_LOGGING
VISKORES_CONT
int getVerbosityByLevel(LogLevel level)
{
  return static_cast<loguru::Verbosity>(level);
}

namespace detail
{

struct LogScope::InternalStruct : loguru::LogScopeRAII
{
  template <typename... Ts>
  InternalStruct(Ts&&... args)
    : loguru::LogScopeRAII(std::forward<Ts>(args)...)
  {
  }
};

VISKORES_CONT
LogScope::LogScope(LogLevel level, const char* file, unsigned line, const char* format...)
{
  auto verbosity = getVerbosityByLevel(level);

  if (verbosity > loguru::current_verbosity_cutoff())
  {
    this->Internals = std::make_unique<InternalStruct>();
  }
  else
  {
    va_list args;
    va_start(args, format);
    this->Internals = std::make_unique<InternalStruct>(verbosity, file, line, format, args);
    va_end(args);
  }
}

LogScope::~LogScope() = default;

} // namespace detail

VISKORES_CONT
void LogCond(LogLevel level, bool cond, const char* file, unsigned line, const char* format...)
{
  if (cond)
  {
    auto verbosity = getVerbosityByLevel(level);

    if (verbosity <= loguru::current_verbosity_cutoff())
    {
      va_list args;
      va_start(args, format);
      loguru::vlog(verbosity, file, line, format, args);
      va_end(args);
    }
  }
}

VISKORES_CONT
LogCondStream::~LogCondStream() noexcept(false)
{
  LogCond(this->Level, this->Condition, this->File, this->Line, this->SStream.str().c_str());
}
#endif // VISKORES_ENABLE_LOGGING

}
} // end namespace viskores::cont
