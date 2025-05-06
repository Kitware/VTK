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
#ifndef viskores_cont_Logging_h
#define viskores_cont_Logging_h

#include <viskores/internal/Configure.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/Types.h>

#include <viskores/cont/viskores_cont_export.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <typeindex>
#include <typeinfo>

/// \file Logging.h
/// \brief Logging utilities.
///
/// This file includes the logging system for Viskores. There are a variety of
/// macros to print log messages using C++ stream or printf syntax. Nested
/// scopes may be created in the log output, and there are several helper
/// functions to help format common types of log data such as byte counts and
/// type names.
///
/// Logging is enabled via the CMake option Viskores_ENABLE_LOGGING by default.
/// The default log level is set to only log Warn and Error messages; Fatal
/// levels are printed to stderr by default. The logging system will need
/// to be initialized through a call to either viskores::cont::Initialize or
/// viskores::cont::InitLogging.
///
/// Additional logging features are enabled by calling viskores::cont::InitLogging
/// (or preferably, viskores::cont::Initialize) in an executable. This will:
/// - Set human-readable names for the log levels in the output.
/// - Allow the stderr logging level to be set at runtime by passing a
///   '--viskores-log-level [level]' argument to the executable.
/// - Name the main thread.
/// - Print a preamble with details of the program's startup (args, etc).
/// - Install signal handlers to automatically print stacktraces and error
///   contexts (linux only) on crashes.
///
/// The main logging entry points are the macros VISKORES_LOG_S and VISKORES_LOG_F,
/// which using C++ stream and printf syntax, repectively. Other variants exist,
/// including conditional logging and special-purpose logs for writing specific
/// events, such as DynamicObject cast results and TryExecute failures.
///
/// The logging backend supports the concept of "Scopes". By creating a new
/// scope with the macros VISKORES_LOG_SCOPE or VISKORES_LOG_SCOPE_FUNCTION, a new
/// "logging scope" is opened within the C++ scope the macro is called from. New
/// messages will be indented in the log until the scope ends, at which point
/// a message is logged with the elapsed time that the scope was active. Scopes
/// may be nested to arbitrary depths.
///
/// The logging implementation is thread-safe. When working in a multithreaded
/// environment, each thread may be assigned a human-readable name using
/// viskores::cont::SetThreadName. This will appear in the log output so that
/// per-thread messages can be easily tracked.
///
/// By default, only Warn, Error, and Fatal messages are printed to
/// stderr. This can be changed at runtime by passing the '--viskores-log-level' flag to an
/// executable that calls viskores::cont::InitLogging. Alternatively, the
/// application can explicitly call viskores::cont::SetStderrLogLevel to change the
/// verbosity. When specifying a verbosity, all log levels with enum values
/// less-than-or-equal-to the requested level are printed.
/// viskores::cont::LogLevel::Off (or "--viskores-log-level Off") may be used to silence the log
/// completely.
///
/// The helper functions viskores::cont::GetHumanReadableSize and
/// viskores::cont::GetSizeString assist in formating byte sizes to a more readable
/// format. Similarly, the viskores::cont::TypeToString template functions provide RTTI
/// based type-name information. When logging is enabled, these use the logging
/// backend to demangle symbol names on supported platforms.
///
/// The more verbose Viskores log levels are:
/// - Perf: Logs performance information, using the scopes feature to track
///   execution time of filters, worklets, and device algorithms with
///   microsecond resolution.
/// - MemCont / MemExec: These levels log memory allocations in the control and
///   execution environments, respectively.
/// - MemTransfer: This level logs memory transfers between the control and host
///   environments.
/// - KernelLaunches: This level logs details about each device side kernel launch
///   such as the CUDA PTX, Warps, and Grids used.
/// - Cast: Logs details of dynamic object resolution.
///
/// The log may be shared and extended by applications that use Viskores. There
/// are two log level ranges left available for applications: User and
/// UserVerbose. The User levels may be enabled without showing any of the
/// verbose Viskores levels, while UserVerbose levels will also enable all Viskores
/// levels.

/// \def VISKORES_LOG_S(level, ...)
/// \brief Writes a message using stream syntax to the indicated log \a level.
///
/// The ellipsis may be replaced with the log message as if constructing a C++
/// stream, e.g:
///
/// \code
/// VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
///            "Executed functor " << viskores::cont::TypeToString(functor)
///             << " on device " << deviceId.GetName());
/// \endcode

/// \def VISKORES_LOG_F(level, ...)
/// \brief Writes a message using printf syntax to the indicated log \a level.
///
/// The ellipsis may be replaced with the log message as if constructing a
/// printf call, e.g:
///
/// \code
/// VISKORES_LOG_F(viskores::cont::LogLevel::Perf,
///            "Executed functor %s on device %s",
///            viskores::cont::TypeToString(functor).c_str(),
///            deviceId.GetName().c_str());
/// \endcode

/// \def VISKORES_LOG_IF_S(level, cond, ...)
/// Same as VISKORES_LOG_S, but only logs if \a cond is true.

/// \def VISKORES_LOG_IF_F(level, cond, ...)
/// Same as VISKORES_LOG_F, but only logs if \a cond is true.

/// \def VISKORES_LOG_SCOPE(level, ...)
/// Creates a new scope at the requested \a level. The log scope ends when the
/// code scope ends. The ellipses form the scope name using printf syntax.
///
/// \code
/// {
///   VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
///                  "Executing filter %s",
///                  viskores::cont::TypeToString(myFilter).c_str());
///   myFilter.Execute();
/// }
/// \endcode

/// \def VISKORES_LOG_SCOPE_FUNCTION(level)
/// Equivalent to `VISKORES_LOG_SCOPE(level, __func__)`

/// \def VISKORES_LOG_ALWAYS_S(level, ...)
/// This ostream-style log message is always emitted, even when logging is
/// disabled at compile time.

/// \def VISKORES_LOG_CAST_SUCC(inObj, outObj)
/// \brief Convenience macro for logging the successful cast of dynamic object.
/// \param inObj The dynamic object.
/// \param outObj The resulting downcasted object.

/// \def VISKORES_LOG_CAST_FAIL(inObj, outType)
/// \brief Convenience macro for logging a failed cast of dynamic object.
/// \param inObj The dynamic object.
/// \param outType The candidate type (or typelist) that was unsuccessful.

/// \def VISKORES_LOG_TRYEXECUTE_FAIL(errorMessage, functorName, deviceId)
/// \brief Convenience macro for logging a TryExecute failure to the Error level.
/// If logging is disabled, a message is still printed to stderr.
/// \param errorMessage The error message detailing the failure.
/// \param functorName The name of the functor (see viskores::cont::TypeToString)
/// \param deviceId The device tag / id for the device on which the functor
/// failed.

/// \def VISKORES_LOG_TRYEXECUTE_DISABLE(errorMessage, functorName, deviceId)
/// \brief Similar to VISKORES_LOG_TRYEXECUTE_FAIL, but also informs the user
/// that the device has been disable for future TryExecute calls.
/// \param errorMessage The error message detailing the failure.
/// \param functorName The name of the functor (see viskores::cont::TypeToString)
/// \param deviceId The device tag / id for the device on which the functor
/// failed.

/// \def VISKORES_DEFINE_USER_LOG_LEVEL(name, offset)
/// \brief Convenience macro for creating a custom log level that is usable
/// in the other macros.  If logging is disabled this macro does nothing.
/// \param name The name to give the new log level
/// \param offset The offset from the viskores::cont::LogLevel::UserFirst value
/// from the LogLevel enum.  Additionally moduloed against the
/// viskores::cont::LogLevel::UserLast value
/// \note This macro is to be used for quickly setting log levels.  For a
/// more maintainable solution it is recommended to create a custom enum class
/// and then cast appropriately, as described here:
/// https://gitlab.kitware.com/vtk/viskores/-/issues/358#note_550157

#define VISKORES_CONCAT_IMPL(s1, s2) s1##s2
#define VISKORES_CONCAT(s1, s2) VISKORES_CONCAT_IMPL(s1, s2)

#ifdef __COUNTER__
#define VISKORES_ANONYMOUS_VARIABLE VISKORES_CONCAT(viskores_anonymous_, __COUNTER__)
#else
#define VISKORES_ANONYMOUS_VARIABLE VISKORES_CONCAT(viskores_anonymous_, __LINE__)
#endif

#if defined(VISKORES_ENABLE_LOGGING)

#define VISKORES_LOG_IF_S(level, cond, ...) \
  viskores::cont::LogCondStream(level, cond, __FILE__, __LINE__) << __VA_ARGS__

#define VISKORES_LOG_IF_F(level, cond, ...) \
  viskores::cont::LogCond(level, cond, __FILE__, __LINE__, __VA_ARGS__)

#define VISKORES_LOG_S(level, ...) VISKORES_LOG_IF_S(level, true, __VA_ARGS__)
#define VISKORES_LOG_F(level, ...) VISKORES_LOG_IF_F(level, true, __VA_ARGS__)

#define VISKORES_LOG_SCOPE(level, ...)                         \
  viskores::cont::detail::LogScope VISKORES_ANONYMOUS_VARIABLE \
  {                                                            \
    level, __FILE__, __LINE__, __VA_ARGS__                     \
  }

#define VISKORES_LOG_SCOPE_FUNCTION(level) VISKORES_LOG_SCOPE(level, __func__)
#define VISKORES_LOG_ALWAYS_S(level, ...) VISKORES_LOG_S(level, __VA_ARGS__)


// Convenience macros:

// Cast success:
#define VISKORES_LOG_CAST_SUCC(inObj, outObj)                  \
  VISKORES_LOG_F(viskores::cont::LogLevel::Cast,               \
                 "Cast succeeded: %s (%p) --> %s (%p)",        \
                 viskores::cont::TypeToString(inObj).c_str(),  \
                 &inObj,                                       \
                 viskores::cont::TypeToString(outObj).c_str(), \
                 &outObj)

// Cast failure:
#define VISKORES_LOG_CAST_FAIL(inObj, outType)                \
  VISKORES_LOG_F(viskores::cont::LogLevel::Cast,              \
                 "Cast failed: %s (%p) --> %s",               \
                 viskores::cont::TypeToString(inObj).c_str(), \
                 &inObj,                                      \
                 viskores::cont::TypeToString<outType>().c_str())

// TryExecute failure
#define VISKORES_LOG_TRYEXECUTE_FAIL(errorMessage, functorName, deviceId)              \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error,                                      \
                 "TryExecute encountered an error: " << errorMessage);                 \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error, "Failing functor: " << functorName); \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error, "Failing device: " << deviceId.GetName())

// Same, but disabling device:
#define VISKORES_LOG_TRYEXECUTE_DISABLE(errorMessage, functorName, deviceId)                 \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error,                                            \
                 "TryExecute encountered an error: " << errorMessage);                       \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error, "Failing functor: " << functorName);       \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error, "Failing device: " << deviceId.GetName()); \
  VISKORES_LOG_S(viskores::cont::LogLevel::Error, "The failing device has been disabled.")

// Custom log level
#define VISKORES_DEFINE_USER_LOG_LEVEL(name, offset)                                      \
  static constexpr viskores::cont::LogLevel name = static_cast<viskores::cont::LogLevel>( \
    static_cast<typename std::underlying_type<viskores::cont::LogLevel>::type>(           \
      viskores::cont::LogLevel::UserFirst) +                                              \
    offset %                                                                              \
      static_cast<typename std::underlying_type<viskores::cont::LogLevel>::type>(         \
        viskores::cont::LogLevel::UserLast))

#else // VISKORES_ENABLE_LOGGING

#define VISKORES_LOG_S(level, ...)
#define VISKORES_LOG_F(level, ...)
#define VISKORES_LOG_IF_S(level, cond, ...)
#define VISKORES_LOG_IF_F(level, cond, ...)
#define VISKORES_LOG_SCOPE(level, ...)
#define VISKORES_LOG_SCOPE_FUNCTION(level)
#define VISKORES_LOG_ERROR_CONTEXT(desc, data)
#define VISKORES_LOG_CAST_SUCC(inObj, outObj)
#define VISKORES_LOG_CAST_FAIL(inObj, outType)
#define VISKORES_DEFINE_USER_LOG_LEVEL(name, offset)

// Always emitted. When logging is disabled, std::cerr is used.

#define VISKORES_LOG_ALWAYS_S(level, ...)               \
  (static_cast<int>(level) < 0 ? std::cerr : std::cout) \
    << viskores::cont::GetLogLevelName(level) << ": " << __VA_ARGS__ << "\n"

// TryExecute failures are still important enough to log, but we just write to
// std::cerr when logging is disabled.
#define VISKORES_LOG_TRYEXECUTE_FAIL(errorMessage, functorName, deviceId)         \
  std::cerr << "Error: TryExecute encountered an error: " << errorMessage << "\n" \
            << "\t- Failing functor: " << functorName << "\n"                     \
            << "\t- Failing device: " << deviceId.GetName() << "\n\n"
#define VISKORES_LOG_TRYEXECUTE_DISABLE(errorMessage, functorName, deviceId)      \
  std::cerr << "Error: TryExecute encountered an error: " << errorMessage << "\n" \
            << "\t- Failing functor: " << functorName << "\n"                     \
            << "\t- Failing device: " << deviceId.GetName() << "\n"               \
            << "The failing device has been disabled.\n\n"

#endif // VISKORES_ENABLE_LOGGING

namespace viskores
{
namespace cont
{

/// Log levels for use with the logging macros.
enum class LogLevel
{
  /// A placeholder used to silence all logging. Do not actually log to
  /// this level.
  Off = -9, //loguru::Verbosity_OFF,

  /// Fatal errors that should abort execution.
  Fatal = -3, // loguru::Verbosity_FATAL,

  /// Important but non-fatal errors, such as device fail-over.
  Error = -2, // loguru::Verbosity_ERROR,

  /// Less important user errors, such as out-of-bounds parameters.
  Warn = -1, // loguru::Verbosity_WARNING,

  /// Information messages (detected hardware, etc) and temporary debugging
  /// output.
  Info = 0, //loguru::Verbosity_INFO,

  /// The first in a range of logging levels reserved for code that uses Viskores.
  /// Internal Viskores code will not log on these levels but will report these logs.
  UserFirst = 1,
  /// The last in a range of logging levels reserved for code that uses Viskores.
  UserLast = 255,

  /// Information about which devices are enabled/disabled.
  DevicesEnabled,

  /// General timing data and algorithm flow information, such as filter
  /// execution, worklet dispatches, and device algorithm calls.
  Perf,

  /// Host-side resource allocations/frees (e.g. ArrayHandle control buffers).
  MemCont,

  /// Device-side resource allocations/frees (e.g ArrayHandle device buffers).
  MemExec,

  /// Transferring of data between a host and device.
  MemTransfer,

  /// Details on device-side kernel launches.
  KernelLaunches,

  /// Reports when a dynamic object is (or is not) resolved via a CastAndCall or other
  /// casting method.
  Cast,

  /// The first in a range of logging levels reserved for code that uses Viskores.
  /// Internal Viskores code will not log on these levels but will report these logs.
  /// These are used similarly to those in the UserFirst range but are at a lower
  /// precedence that also includes more verbose reporting from Viskores.
  UserVerboseFirst = 1024,
  /// The last in a range of logging levels reserved for code that uses Viskores.
  UserVerboseLast = 2047
};


/**
 * This shouldn't be called directly -- prefer calling viskores::cont::Initialize,
 * which takes care of logging as well as other initializations.
 *
 * Initializes logging. Sets up custom log level and thread names. Parses any
 * "--viskores-log-level [LogLevel]" arguments to set the stderr log level. This argument may
 * be either numeric, or the 4-character string printed in the output. Note that
 * loguru will consume the "--viskores-log-level [LogLevel]" argument and shrink the arg list.
 *
 * If the parameterless overload is used, the `--viskores-log-level` parsing is not used, but
 * other functionality should still work.
 *
 * @note This function is not threadsafe and should only be called from a single
 * thread (ideally the main thread).
 * @{
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
void InitLogging(int& argc,
                 char* argv[],
                 const std::string& loggingFlag = "--viskores-log-level",
                 const std::string& loggingEnv = "VISKORES_LOG_LEVEL");
VISKORES_CONT_EXPORT
VISKORES_CONT
void InitLogging();
/**@}*/

/**
 * Set the range of log levels that will be printed to stderr. All levels
 * with an enum value less-than-or-equal-to \a level will be printed.
 * @{
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
void SetStderrLogLevel(const char* verbosity);

VISKORES_CONT_EXPORT
VISKORES_CONT
void SetStderrLogLevel(viskores::cont::LogLevel level);
/**@}*/

/**
 * Get the active highest log level that will be printed to stderr.
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::LogLevel GetStderrLogLevel();

/**
 * Register a custom name to identify a log level. The name will be truncated
 * to 4 characters internally.
 *
 * Must not be called after InitLogging. Such calls will fail and log an error.
 *
 * There is no need to call this for the default viskores::cont::LogLevels. They
 * are populated in InitLogging and will be overwritten.
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
void SetLogLevelName(viskores::cont::LogLevel level, const std::string& name);

/**
 * Get a human readable name for the log level. If a name has not been
 * registered via InitLogging or SetLogLevelName, the returned string just
 * contains the integer representation of the level.
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
std::string GetLogLevelName(viskores::cont::LogLevel level);

/**
 * Specifies a humman-readable name to identify the current thread in the log output.
 * @{
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
void SetLogThreadName(const std::string& name);
VISKORES_CONT_EXPORT
VISKORES_CONT
std::string GetLogThreadName();
/**@}*/

// Per-thread error context, not currently used, undocumented....
VISKORES_CONT_EXPORT
VISKORES_CONT
std::string GetLogErrorContext();

/**
 * Returns a stacktrace on supported platforms.
 * Argument is the number of frames to skip (GetStackTrace and below are already
 * skipped).
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
std::string GetStackTrace(viskores::Int32 skip = 0);

//@{
/// Convert a size in bytes to a human readable string (such as "64 bytes",
/// "1.44 MiB", "128 GiB", etc). @a prec controls the fixed point precision
/// of the stringified number.
VISKORES_CONT_EXPORT
VISKORES_CONT
std::string GetHumanReadableSize(viskores::UInt64 bytes, int prec = 2);

template <typename T>
VISKORES_CONT inline std::string GetHumanReadableSize(T&& bytes, int prec = 2)
{
  return GetHumanReadableSize(static_cast<viskores::UInt64>(std::forward<T>(bytes)), prec);
}
//@}

//@{
/// Returns "%1 (%2 bytes)" where %1 is the result from GetHumanReadableSize
/// and %2 is the exact number of bytes.
VISKORES_CONT_EXPORT
VISKORES_CONT
std::string GetSizeString(viskores::UInt64 bytes, int prec = 2);

template <typename T>
VISKORES_CONT inline std::string GetSizeString(T&& bytes, int prec = 2)
{
  return GetSizeString(static_cast<viskores::UInt64>(std::forward<T>(bytes)), prec);
}
//@}

/**
 * Use RTTI information to retrieve the name of the type T. If logging is
 * enabled and the platform supports it, the type name will also be demangled.
 * @{
 */
VISKORES_CONT_EXPORT VISKORES_CONT std::string TypeToString(const std::type_info& t);
VISKORES_CONT_EXPORT VISKORES_CONT std::string TypeToString(const std::type_index& t);
template <typename T>
inline VISKORES_CONT std::string TypeToString()
{
  return TypeToString(typeid(T));
}
template <typename T>
inline VISKORES_CONT std::string TypeToString(const T&)
{
  return TypeToString(typeid(T));
}
/**@}*/

#ifdef VISKORES_ENABLE_LOGGING

/**
 * \brief Conditionally logs a message with a printf-like format.
 *
 * \param level  Desired LogLevel value for the log message.
 * \param cond   When false this function is no-op.
 * \param file   The source file where the log entry was genearted.
 * \param line   The line in the source file where the log entry was generated.
 * \param format Printf like format string.
 */
VISKORES_CONT_EXPORT
VISKORES_CONT
void LogCond(LogLevel level, bool cond, const char* file, unsigned line, const char* format...);

namespace detail
{

/**
 * \brief Logs a scoped message with a printf-like format.
 *
 * The indentation level will be determined based on its LogLevel and it will
 * print out its wall time upon exiting its scope. The scope starts from when
 * the object is created to when it is destroyed.
 */
class VISKORES_CONT_EXPORT LogScope
{
  struct InternalStruct;
  std::unique_ptr<InternalStruct> Internals;

public:
  /*
   * \param level  Desired LogLevel value for the log message.
   * \param cond   When false this function is no-op.
   * \param format Printf like format string.
   */
  VISKORES_CONT
  LogScope(LogLevel level, const char* file, unsigned line, const char* format...);

  VISKORES_CONT ~LogScope();
};


} // namespace detail

/**
 * \brief Conditionally logs a message with a stream-like interface.
 *
 * Messages are flushed to output by the destructor.
 */
struct VISKORES_CONT_EXPORT LogCondStream
{
  VISKORES_CONT
  LogCondStream(LogLevel level, bool cond, const char* file, int line)
    : Level(level)
    , Condition(cond)
    , File(file)
    , Line(line)
  {
  }

  VISKORES_CONT
  ~LogCondStream() noexcept(false);

  template <typename T>
  VISKORES_CONT LogCondStream& operator<<(const T& in)
  {
    SStream << in;
    return *this;
  }

  VISKORES_CONT
  LogCondStream& operator<<(std::ostream& (*f)(std::ostream&))
  {
    f(SStream);
    return *this;
  }

private:
  LogLevel Level;
  bool Condition;
  const char* File;
  int Line;
  std::ostringstream SStream;
};
#endif // VISKORES_ENABLE_LOGGING

}
} // end namespace viskores::cont

#endif // viskores_cont_Logging_h
