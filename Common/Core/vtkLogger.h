// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkLogger
 * @brief logging framework for use in VTK and in applications based on VTK
 *
 * vtkLogger acts as the entry point to VTK's logging framework. The
 * implementation uses the loguru (https://github.com/emilk/loguru). vtkLogger
 * provides some static API to initialize and configure logging together with a
 * collection of macros that can be used to add items to the generated log.
 *
 * The logging framework is based on verbosity levels. Level 0-9 are supported
 * in addition to named levels such as ERROR, WARNING, and INFO. When a log for
 * a particular verbosity level is being generated, all log additions issued
 * with verbosity level less than or equal to the requested verbosity level will
 * get logged.
 *
 * When using any of the logging macros, it must be noted that unless a log output
 * is requesting that verbosity provided (or higher), the call is a no-op and
 * the message stream or printf-style arguments will not be evaluated.
 *
 * @section Setup Setup
 *
 * To initialize logging, in your application's `main()` you may call
 * `vtkLogger::Init(argv, argc)`. This is totally optional but useful to
 * time-stamp the  start of the  log. Furthermore, it can optionally detect
 * verbosity level on the command line as `-v` (or any another string pass as
 * the optional argument to `Init`) that will be used as the verbosity level for
 * logging on to `stderr`. By default, it is set to `0` (or `INFO`) unless
 * changed by calling `vtkLogger::SetStderrVerbosity`.
 *
 * In additional to logging to `stderr`, one can accumulate logs to one or more files using
 * `vtkLogger::LogToFile`. Each log file can be given its own verbosity level.
 *
 * For multithreaded applications, you may want to name each of the threads so
 * that the generated log can use human readable names for the threads. For
 * that, use `vtkLogger::SetThreadName`. Calling `vtkLogger::Init` will set the name
 * for the main thread.
 *
 * You can choose to turn on signal handlers for intercepting signals. By default,
 * all signal handlers are disabled. The following is a list of signal handlers
 * and the corresponding static variable that can be used to enable/disable each
 * signal handler.
 *
 * - SIGABRT - `vtkLogger::EnableSigabrtHandler`
 * - SIGBUS - `vtkLogger::EnableSigbusHandler`
 * - SIGFPE - `vtkLogger::EnableSigfpeHandler`
 * - SIGILL - `vtkLogger::EnableSigillHandler`
 * - SIGINT - `vtkLogger::EnableSigintHandler`
 * - SIGSEGV - `vtkLogger::EnableSigsegvHandler`
 * - SIGTERM - `vtkLogger::EnableSigtermHandler`
 *
 * To enable any of these signal handlers, set their value to `true` prior to calling
 * `vtkLogger::Init(argc, argv)` or `vtkLogger::Init()`.
 *
 * When signal handlers are enabled,
 * to prevent the logging framework from intercepting signals from your application,
 * you can set the static variable `vtkLogger::EnableUnsafeSignalHandler` to `false`
 * prior to calling `vtkLogger::Init(argc, argv)` or `vtkLogger::Init()`.
 *
 * @section Logging Logging
 *
 * vtkLogger provides several macros (again, based on `loguru`) that can be
 * used to add the log. Both printf-style and stream-style is supported. All
 * printf-style macros are suffixed with `F` to distinguish them from the stream
 * macros. Another pattern in naming macros is the presence of `V`
 * e.g. `vtkVLog` vs `vtkLog`. A macro with the `V` prefix takes a fully
 * qualified verbosity enum e.g. `vtkLogger::VERBOSITY_INFO` or
 * `vtkLogger::VERBOSITY_0`, while the non-`V` variant takes the verbosity
 * name e.g. `INFO` or `0`.
 *
 * Following code snippet provides an overview of the available macros and their
 * usage.
 *
 * @code{.cpp}
 *
 *  // Optional, leaving this as the default value `true` will let the logging
 *  // framework log signals such as segmentation faults.
 *
 *  vtkLogger::EnableUnsafeSignalHandler = false;
 *
 *  // Optional, but useful to time-stamp the start of the log.
 *  // Will also detect verbosity level on the command line as -v.
 *
 *  vtkLogger::Init(argc, argv);
 *
 *  // Put every log message in "everything.log":
 *  vtkLogger::LogToFile("everything.log", vtkLogger::APPEND, vtkLogger::VERBOSITY_MAX);
 *
 *  // Only log INFO, WARNING, ERROR to "latest_readable.log":
 *  vtkLogger::LogToFile("latest_readable.log", vtkLogger::TRUNCATE, vtkLogger::VERBOSITY_INFO);
 *
 *  // Only show most relevant things on stderr:
 *  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_1);
 *
 *  // add a line to log using the verbosity name.
 *  vtkLogF(INFO, "I'm hungry for some %.3f!", 3.14159);
 *  vtkLogF(0, "same deal");
 *
 *  // add a line to log using the verbosity enum.
 *  vtkVLogF(vtkLogger::VERBOSITY_INFO, "I'm hungry for some %.3f!", 3.14159);
 *  vtkVLogF(vtkLogger::VERBOSITY_0, "same deal");
 *
 *  // to add an identifier for a vtkObjectBase or subclass
 *  vtkLogF(INFO, "The object is %s", vtkLogIdentifier(vtkobject));
 *
 *  // add a line conditionally to log if the condition succeeds:
 *  vtkLogIfF(INFO, ptr == nullptr, "ptr is nullptr (some number: %.3f)", *  3.14159);
 *
 *  vtkLogScopeF(INFO, "Will indent all log messages within this scope.");
 *  // in a function, you may use vtkLogScopeFunction(INFO)
 *
 *  // scope can be explicitly started and closed by vtkLogStartScope (or
 *  // vtkLogStartScopef) and vtkLogEndScope
 *  vtkLogStartScope(INFO, "id-used-as-message");
 *  vtkLogStartScopeF(INFO, "id", "message-%d", 1);
 *  vtkLogEndScope("id");
 *  vtkLogEndScope("id-used-as-message");
 *
 *  // alternatively, you can use streams instead of printf-style
 *  vtkLog(INFO, "I'm hungry for some " << 3.14159 << "!");
 *  vtkLogIF(INFO, ptr == nullptr, "ptr is " << "nullptr");
 *
 * @endcode
 *
 * @section LoggingAndLegacyMacros Logging and VTK error macros
 *
 * VTK has long supported multiple macros to report errors, warnings and debug
 * messages through `vtkErrorMacro`, `vtkWarningMacro`, `vtkDebugMacro`, etc.
 * In addition to performing the traditional message reporting via
 * `vtkOutputWindow`, these macros also log to the logging sub-system with
 * appropriate verbosity levels.
 *
 * To avoid the vtkLogger and vtkOutputWindow both posting the message to the
 * standard output streams, vtkOutputWindow now supports an ability to specify
 * terminal display mode, via `vtkOutputWindow::SetDisplayMode`. If display mode
 * is `vtkOutputWindow::DEFAULT` then the output window will not
 * post messages originating from the standard error/warning/debug macros to the
 * standard output if VTK is built with logging support. If VTK is not built
 * with logging support, then vtkOutputWindow will post the messages to the
 * standard output streams, unless disabled explicitly.
 *
 * @section Callbacks Custom callbacks/handlers for log messages
 *
 * vtkLogger supports ability to register callbacks to call on each logged
 * message. This is useful to show the messages in application specific
 * viewports, e.g. a special message widget.
 *
 * To register a callback use `vtkLogger::AddCallback` and to remove a callback
 * use `vtkLogger::RemoveCallback` with the id provided when registering the
 * callback.
 *
 */

#ifndef vtkLogger_h
#define vtkLogger_h

#include "vtkObjectBase.h"
#include "vtkSetGet.h" // needed for macros

#include <string> // needed for std::string

#if defined(_MSC_VER)
#include <sal.h> // Needed for _In_z_ etc annotations
#endif

// this is copied from `loguru.hpp`
#if defined(__clang__) || defined(__GNUC__)
// Helper macro for declaring functions as having similar signature to printf.
// This allows the compiler to catch format errors at compile-time.
#define VTK_PRINTF_LIKE(fmtarg, firstvararg)                                                       \
  __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#define VTK_FORMAT_STRING_TYPE const char*
#elif defined(_MSC_VER)
#define VTK_PRINTF_LIKE(fmtarg, firstvararg)
#define VTK_FORMAT_STRING_TYPE _In_z_ _Printf_format_string_ const char*
#else
#define VTK_PRINTF_LIKE(fmtarg, firstvararg)
#define VTK_FORMAT_STRING_TYPE const char*
#endif

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkLogger : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkLogger, vtkObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Verbosity
  {
    // Used to mark an invalid verbosity. Do not log to this level.
    VERBOSITY_INVALID = -10, // Never do LOG_F(INVALID)

    // You may use VERBOSITY_OFF on g_stderr_verbosity, but for nothing else!
    VERBOSITY_OFF = -9, // Never do LOG_F(OFF)

    VERBOSITY_ERROR = -2,
    VERBOSITY_WARNING = -1,

    // Normal messages. By default written to stderr.
    VERBOSITY_INFO = 0,

    // Same as VERBOSITY_INFO in every way.
    VERBOSITY_0 = 0,

    // Verbosity levels 1-9 are generally not written to stderr, but are written to file.
    VERBOSITY_1 = +1,
    VERBOSITY_2 = +2,
    VERBOSITY_3 = +3,
    VERBOSITY_4 = +4,
    VERBOSITY_5 = +5,
    VERBOSITY_6 = +6,
    VERBOSITY_7 = +7,
    VERBOSITY_8 = +8,
    VERBOSITY_9 = +9,

    // trace level, same as VERBOSITY_9
    VERBOSITY_TRACE = +9,

    // Don not use higher verbosity levels, as that will make grepping log files harder.
    VERBOSITY_MAX = +9,
  };

  /**
   * Initializes logging. This should be called from the main thread, if at all.
   * Your application doesn't *need* to call this, but if you do:
   *  * signal handlers are installed
   *  * program arguments are logged
   *  * working directory is logged
   *  * optional -v verbosity flag is parsed
   *  * main thread name is set to "main thread"
   *  * explanation of the preamble (date, threadname, etc.) is logged.
   *
   * This method will look for arguments meant for logging subsystem and remove
   * them. Arguments meant for logging subsystem are:
   *
   * -v n Set stderr logging verbosity. Examples
   *    -v 3        Show verbosity level 3 and lower.
   *    -v 0        Only show INFO, WARNING, ERROR, FATAL (default).
   *    -v INFO     Only show INFO, WARNING, ERROR, FATAL (default).
   *    -v WARNING  Only show WARNING, ERROR, FATAL.
   *    -v ERROR    Only show ERROR, FATAL.
   *    -v FATAL    Only show FATAL.
   *    -v OFF      Turn off logging to stderr.
   *
   * You can set the default logging verbosity programmatically by calling
   * `vtkLogger::SetStderrVerbosity` before calling `vtkLogger::Init`. That
   * way, you can specify a default that the user can override using command
   * line arguments. Note that this does not affect file logging.
   *
   * You can also use something else instead of '-v' flag by the via
   * `verbosity_flag` argument. You can also set to nullptr to skip parsing
   * verbosity level from the command line arguments.
   *
   * For applications that do not want loguru to handle any signals, i.e.,
   * print a stack trace when a signal is intercepted, the
   * `vtkLogger::EnableUnsafeSignalHandler` static member variable
   * should be set to `false`.
   * @{
   */
  static void Init(int& argc, char* argv[], const char* verbosity_flag = "-v");
  static void Init();
  /** @} */

  /**
   * Set the verbosity level for the output logged to stderr. Everything with a
   * verbosity equal or less than the level specified will be written to
   * stderr. Set to `VERBOSITY_OFF` to write nothing to stderr.
   * Default is 0.
   */
  static void SetStderrVerbosity(Verbosity level);

  /**
   * Set internal messages verbosity level. The library used by VTK, `loguru`
   * generates log messages during initialization and at exit. These are logged
   * as log level VERBOSITY_1, by default. One can change that using this
   * method. Typically, you want to call this before `vtkLogger::Init`.
   */
  static void SetInternalVerbosityLevel(Verbosity level);

  /**
   * Support log file modes: `TRUNCATE` truncates the file clearing any existing
   * contents while `APPEND` appends to the existing log file contents, if any.
   */
  enum FileMode
  {
    TRUNCATE,
    APPEND
  };

  /**
   * Enable logging to a file at the given path.
   * Any logging message with verbosity lower or equal to the given verbosity
   * will be included. This method will create all directories in the 'path' if
   * needed. To stop the file logging, call `EndLogToFile` with the same path.
   */
  static void LogToFile(const char* path, FileMode filemode, Verbosity verbosity);

  /**
   * Stop logging to a file at the given path.
   */
  static void EndLogToFile(const char* path);

  ///@{
  /**
   * Get/Set the name to identify the current thread in the log output.
   */
  static void SetThreadName(const std::string& name);
  static std::string GetThreadName();
  ///@}

  /**
   * Returns a printable string for a vtkObjectBase instance.
   */
  static std::string GetIdentifier(vtkObjectBase* obj);

  /**
   * The message structure that is passed to custom callbacks registered using
   * `vtkLogger::AddCallback`.
   */
  struct Message
  {
    // You would generally print a Message by just concatenating the buffers without spacing.
    // Optionally, ignore preamble and indentation.
    Verbosity verbosity;     // Already part of preamble
    const char* filename;    // Already part of preamble
    unsigned line;           // Already part of preamble
    const char* preamble;    // Date, time, uptime, thread, file:line, verbosity.
    const char* indentation; // Just a bunch of spacing.
    const char* prefix;      // Assertion failure info goes here (or "").
    const char* message;     // User message goes here.
  };

  ///@{
  /**
   * Callback handle types.
   */
  using LogHandlerCallbackT = void (*)(void* user_data, const Message& message);
  using CloseHandlerCallbackT = void (*)(void* user_data);
  using FlushHandlerCallbackT = void (*)(void* user_data);
  ///@}

  /**
   * Add a callback to call on each log message with a  verbosity less or equal
   * to the given one.  Useful for displaying messages in an application output
   * window, for example. The given `on_close` is also expected to flush (if
   * desired).
   *
   * Note that if logging is disabled at compile time, then these callback will
   * never be called.
   */
  static void AddCallback(const char* id, LogHandlerCallbackT callback, void* user_data,
    Verbosity verbosity, CloseHandlerCallbackT on_close = nullptr,
    FlushHandlerCallbackT on_flush = nullptr);

  /**
   * Remove a callback using the id specified.
   * Returns true if and only if the callback was found (and removed).
   */
  static bool RemoveCallback(const char* id);

  /**
   * Returns true if VTK is built with logging support enabled.
   */
  static bool IsEnabled();

  /**
   * Returns the maximum verbosity of all log outputs. A log item for a
   * verbosity higher than this will not be generated in any of the currently
   * active outputs.
   */
  static Verbosity GetCurrentVerbosityCutoff();

  /**
   * Convenience function to convert an integer to matching verbosity level. If
   * val is less than or equal to vtkLogger::VERBOSITY_INVALID, then
   * vtkLogger::VERBOSITY_INVALID is returned. If value is greater than
   * vtkLogger::VERBOSITY_MAX, then vtkLogger::VERBOSITY_MAX is returned.
   */
  static Verbosity ConvertToVerbosity(int value);

  /**
   * Convenience function to convert a string to matching verbosity level.
   * vtkLogger::VERBOSITY_INVALID will be return for invalid strings.
   * Accepted string values are OFF, ERROR, WARNING, INFO, TRACE, MAX, INVALID or ASCII
   * representation for an integer in the range [-9,9].
   */
  static Verbosity ConvertToVerbosity(const char* text);

  ///@{
  /**
   * @internal
   *
   * Not intended for public use, please use the logging macros instead.
   */
  static void Log(
    Verbosity verbosity, VTK_FILEPATH const char* fname, unsigned int lineno, const char* txt);
  static void StartScope(
    Verbosity verbosity, const char* id, VTK_FILEPATH const char* fname, unsigned int lineno);
  static void EndScope(const char* id);
#if !defined(__WRAP__)
  static void LogF(Verbosity verbosity, VTK_FILEPATH const char* fname, unsigned int lineno,
    VTK_FORMAT_STRING_TYPE format, ...) VTK_PRINTF_LIKE(4, 5);
  static void StartScopeF(Verbosity verbosity, const char* id, VTK_FILEPATH const char* fname,
    unsigned int lineno, VTK_FORMAT_STRING_TYPE format, ...) VTK_PRINTF_LIKE(5, 6);

  class VTKCOMMONCORE_EXPORT LogScopeRAII
  {
  public:
    LogScopeRAII();
    LogScopeRAII(vtkLogger::Verbosity verbosity, const char* fname, unsigned int lineno,
      VTK_FORMAT_STRING_TYPE format, ...) VTK_PRINTF_LIKE(5, 6);
    ~LogScopeRAII();
#if defined(_MSC_VER) && _MSC_VER > 1800
    // see loguru.hpp for the reason why this is needed on MSVC
    LogScopeRAII(LogScopeRAII&& other)
      : Internals(other.Internals)
    {
      other.Internals = nullptr;
    }
#else
    LogScopeRAII(LogScopeRAII&&) = default;
#endif

  private:
    LogScopeRAII(const LogScopeRAII&) = delete;
    void operator=(const LogScopeRAII&) = delete;
    class LSInternals;
    LSInternals* Internals;
  };
#endif
  ///@}

  /**
   * Flag to enable/disable the logging frameworks printing of a stack trace
   * when catching signals, which could lead to crashes and deadlocks in
   * certain circumstances.
   */
  static bool EnableUnsafeSignalHandler;
  static bool EnableSigabrtHandler;
  static bool EnableSigbusHandler;
  static bool EnableSigfpeHandler;
  static bool EnableSigillHandler;
  static bool EnableSigintHandler;
  static bool EnableSigsegvHandler;
  static bool EnableSigtermHandler;

protected:
  vtkLogger();
  ~vtkLogger() override;

private:
  vtkLogger(const vtkLogger&) = delete;
  void operator=(const vtkLogger&) = delete;
  static vtkLogger::Verbosity InternalVerbosityLevel;
};

///@{
/**
 * Add to log given the verbosity level.
 * The text will be logged when the log verbosity is set to the specified level
 * or higher.
 *
 *     // using printf-style
 *     vtkLogF(INFO, "Hello %s", "world!");
 *     vtkVLogF(vtkLogger::VERBOSITY_INFO, "Hello %s", "world!");
 *
 *     // using streams
 *     vtkLog(INFO, "Hello " << "world!");
 *     vtkVLog(vtkLogger::VERBOSITY_INFO, << "Hello world!");
 *
 */
#define vtkVLogF(level, ...)                                                                       \
  ((level) > vtkLogger::GetCurrentVerbosityCutoff())                                               \
    ? (void)0                                                                                      \
    : vtkLogger::LogF(level, __FILE__, __LINE__, __VA_ARGS__)
#define vtkLogF(verbosity_name, ...) vtkVLogF(vtkLogger::VERBOSITY_##verbosity_name, __VA_ARGS__)
#define vtkVLog(level, x)                                                                          \
  do                                                                                               \
  {                                                                                                \
    if ((level) <= vtkLogger::GetCurrentVerbosityCutoff())                                         \
    {                                                                                              \
      vtkOStrStreamWrapper::EndlType const endl;                                                   \
      vtkOStrStreamWrapper::UseEndl(endl);                                                         \
      vtkOStrStreamWrapper vtkmsg;                                                                 \
      vtkmsg << "" x;                                                                              \
      vtkLogger::Log(level, __FILE__, __LINE__, vtkmsg.str());                                     \
      vtkmsg.rdbuf()->freeze(0);                                                                   \
    }                                                                                              \
  } while (false)
#define vtkLog(verbosity_name, x) vtkVLog(vtkLogger::VERBOSITY_##verbosity_name, x)
///@}

///@{
/**
 * Add to log only when the `cond` passes.
 *
 *     // using printf-style
 *     vtkLogIfF(ERROR, ptr == nullptr, "`ptr` cannot be null!");
 *     vtkVLogIfF(vtkLogger::VERBOSITY_ERROR, ptr == nullptr, "`ptr` cannot be null!");
 *
 *     // using streams
 *     vtkLogIf(ERROR, ptr == nullptr, "`ptr` cannot be null!");
 *     vtkVLogIf(vtkLogger::VERBOSITY_ERROR, ptr == nullptr, << "`ptr` cannot be null!");
 *
 */
#define vtkVLogIfF(level, cond, ...)                                                               \
  ((level) > vtkLogger::GetCurrentVerbosityCutoff() || (cond) == false)                            \
    ? (void)0                                                                                      \
    : vtkLogger::LogF(level, __FILE__, __LINE__, __VA_ARGS__)

#define vtkLogIfF(verbosity_name, cond, ...)                                                       \
  vtkVLogIfF(vtkLogger::VERBOSITY_##verbosity_name, cond, __VA_ARGS__)

#define vtkVLogIf(level, cond, x)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if ((level) <= vtkLogger::GetCurrentVerbosityCutoff() && (cond))                               \
    {                                                                                              \
      vtkOStrStreamWrapper::EndlType endl;                                                         \
      vtkOStrStreamWrapper::UseEndl(endl);                                                         \
      vtkOStrStreamWrapper vtkmsg;                                                                 \
      vtkmsg << "" x;                                                                              \
      vtkLogger::Log(level, __FILE__, __LINE__, vtkmsg.str());                                     \
      vtkmsg.rdbuf()->freeze(0);                                                                   \
    }                                                                                              \
  } while (false)
#define vtkLogIf(verbosity_name, cond, x) vtkVLogIf(vtkLogger::VERBOSITY_##verbosity_name, cond, x)
///@}

#define VTKLOG_CONCAT_IMPL(s1, s2) s1##s2
#define VTKLOG_CONCAT(s1, s2) VTKLOG_CONCAT_IMPL(s1, s2)
#define VTKLOG_ANONYMOUS_VARIABLE(x) VTKLOG_CONCAT(x, __LINE__)

#define vtkVLogScopeF(level, ...)                                                                  \
  auto VTKLOG_ANONYMOUS_VARIABLE(msg_context) = ((level) > vtkLogger::GetCurrentVerbosityCutoff()) \
    ? vtkLogger::LogScopeRAII()                                                                    \
    : vtkLogger::LogScopeRAII(level, __FILE__, __LINE__, __VA_ARGS__)

#define vtkLogScopeF(verbosity_name, ...)                                                          \
  vtkVLogScopeF(vtkLogger::VERBOSITY_##verbosity_name, __VA_ARGS__)

#define vtkLogScopeFunction(verbosity_name) vtkLogScopeF(verbosity_name, "%s", __func__)
#define vtkVLogScopeFunction(level) vtkVLogScopeF(level, "%s", __func__)

///@{
/**
 * Explicitly mark start and end of log scope. This is useful in cases where the
 * start and end of the scope does not happen within the same C++ scope.
 */
#define vtkLogStartScope(verbosity_name, id)                                                       \
  vtkLogger::StartScope(vtkLogger::VERBOSITY_##verbosity_name, id, __FILE__, __LINE__)
#define vtkLogEndScope(id) vtkLogger::EndScope(id)

#define vtkLogStartScopeF(verbosity_name, id, ...)                                                 \
  vtkLogger::StartScopeF(vtkLogger::VERBOSITY_##verbosity_name, id, __FILE__, __LINE__, __VA_ARGS__)

#define vtkVLogStartScope(level, id) vtkLogger::StartScope(level, id, __FILE__, __LINE__)
#define vtkVLogStartScopeF(level, id, ...)                                                         \
  vtkLogger::StartScopeF(level, id, __FILE__, __LINE__, __VA_ARGS__)
///@}

/**
 * Convenience macro to generate an identifier string for any vtkObjectBase subclass.
 * @note do not store the returned value as it returns a char* pointer to a
 * temporary std::string that will be released as soon as it goes out of scope.
 */
#define vtkLogIdentifier(vtkobject) vtkLogger::GetIdentifier(vtkobject).c_str()

VTK_ABI_NAMESPACE_END
#endif
