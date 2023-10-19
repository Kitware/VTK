/*
Loguru logging library for C++, by Emil Ernerfeldt.
www.github.com/emilk/loguru
If you find Loguru useful, please let me know on twitter or in a mail!
Twitter: @ernerfeldt
Mail:    emil.ernerfeldt@gmail.com
Website: www.ilikebigbits.com

# License
	This software is in the public domain. Where that dedication is not
	recognized, you are granted a perpetual, irrevocable license to
	copy, modify and distribute it as you see fit.

# Inspiration
	Much of Loguru was inspired by GLOG, https://code.google.com/p/google-glog/.
	The choice of public domain is fully due Sean T. Barrett
	and his wonderful stb libraries at https://github.com/nothings/stb.

# Version history
	* Version 0.1.0 - 2015-03-22 - Works great on Mac.
	* Version 0.2.0 - 2015-09-17 - Removed the only dependency.
	* Version 0.3.0 - 2015-10-02 - Drop-in replacement for most of GLOG
	* Version 0.4.0 - 2015-10-07 - Single-file!
	* Version 0.5.0 - 2015-10-17 - Improved file logging
	* Version 0.6.0 - 2015-10-24 - Add stack traces
	* Version 0.7.0 - 2015-10-27 - Signals
	* Version 0.8.0 - 2015-10-30 - Color logging.
	* Version 0.9.0 - 2015-11-26 - ABORT_S and proper handling of FATAL
	* Version 1.0.0 - 2016-02-14 - ERROR_CONTEXT
	* Version 1.1.0 - 2016-02-19 - -v OFF, -v INFO etc
	* Version 1.1.1 - 2016-02-20 - textprintf vs strprintf
	* Version 1.1.2 - 2016-02-22 - Remove g_alsologtostderr
	* Version 1.1.3 - 2016-02-29 - ERROR_CONTEXT as linked list
	* Version 1.2.0 - 2016-03-19 - Add get_thread_name()
	* Version 1.2.1 - 2016-03-20 - Minor fixes
	* Version 1.2.2 - 2016-03-29 - Fix issues with set_fatal_handler throwing an exception
	* Version 1.2.3 - 2016-05-16 - Log current working directory in loguru::init().
	* Version 1.2.4 - 2016-05-18 - Custom replacement for -v in loguru::init() by bjoernpollex
	* Version 1.2.5 - 2016-05-18 - Add ability to print ERROR_CONTEXT of parent thread.
	* Version 1.2.6 - 2016-05-19 - Bug fix regarding VLOG verbosity argument lacking ().
	* Version 1.2.7 - 2016-05-23 - Fix PATH_MAX problem.
	* Version 1.2.8 - 2016-05-26 - Add shutdown() and remove_all_callbacks()
	* Version 1.2.9 - 2016-06-09 - Use a monotonic clock for uptime.
	* Version 1.3.0 - 2016-07-20 - Fix issues with callback flush/close not being called.
	* Version 1.3.1 - 2016-07-20 - Add LOGURU_UNSAFE_SIGNAL_HANDLER to toggle stacktrace on signals.
	* Version 1.3.2 - 2016-07-20 - Add loguru::arguments()
	* Version 1.4.0 - 2016-09-15 - Semantic versioning + add loguru::create_directories
	* Version 1.4.1 - 2016-09-29 - Customize formating with LOGURU_FILENAME_WIDTH
	* Version 1.5.0 - 2016-12-22 - LOGURU_USE_FMTLIB by kolis and LOGURU_WITH_FILEABS by scinart
	* Version 1.5.1 - 2017-08-08 - Terminal colors on Windows 10 thanks to looki
	* Version 1.6.0 - 2018-01-03 - Add LOGURU_RTTI and LOGURU_STACKTRACES settings
	* Version 1.7.0 - 2018-01-03 - Add ability to turn off the preamble with loguru::g_preamble
	* Version 1.7.1 - 2018-04-05 - Add function get_fatal_handler
	* Version 1.7.2 - 2018-04-22 - Fix a bug where large file names could cause stack corruption (thanks @ccamporesi)
	* Version 1.8.0 - 2018-04-23 - Shorten long file names to keep preamble fixed width
	* Version 1.9.0 - 2018-09-22 - Adjust terminal colors, add LOGURU_VERBOSE_SCOPE_ENDINGS, add LOGURU_SCOPE_TIME_PRECISION, add named log levels
	* Version 2.0.0 - 2018-09-22 - Split loguru.hpp into loguru.hpp and loguru.cpp
	* Version 2.1.0 - 2019-09-23 - Update fmtlib + add option to loguru::init to NOT set main thread name.
	* Version 2.2.0 - 2020-07-31 - Replace LOGURU_CATCH_SIGABRT with struct SignalOptions

# Compiling
	Just include <loguru.hpp> where you want to use Loguru.
	Then, in one .cpp file #include <loguru.cpp>
	Make sure you compile with -std=c++11 -lstdc++ -lpthread -ldl

# Usage
	For details, please see the official documentation at emilk.github.io/loguru

	#include <loguru.hpp>

	int main(int argc, char* argv[]) {
		loguru::init(argc, argv);

		// Put every log message in "everything.log":
		loguru::add_file("everything.log", loguru::Append, loguru::Verbosity_MAX);

		LOG_F(INFO, "The magic number is %d", 42);
	}

*/

// XXX(kitware): // include exports header
#include "vtkloguru_export.h"
// XXX(kitware): // mangle
#define loguru vtkloguru
// XXX(kitware): // vtk config defaults
#define LOGURU_WITH_STREAMS 1

#if defined(LOGURU_IMPLEMENTATION)
	#error "You are defining LOGURU_IMPLEMENTATION. This is for older versions of Loguru. You should now instead include loguru.cpp (or build it and link with it)"
#endif

// Disable all warnings from gcc/clang:
#if defined(__clang__)
	#pragma clang system_header
#elif defined(__GNUC__)
	#pragma GCC system_header
#endif

#ifndef LOGURU_HAS_DECLARED_FORMAT_HEADER
#define LOGURU_HAS_DECLARED_FORMAT_HEADER

// Semantic versioning. Loguru version can be printed with printf("%d.%d.%d", LOGURU_VERSION_MAJOR, LOGURU_VERSION_MINOR, LOGURU_VERSION_PATCH);
#define LOGURU_VERSION_MAJOR 2
#define LOGURU_VERSION_MINOR 1
#define LOGURU_VERSION_PATCH 0

#if defined(_MSC_VER)
#include <sal.h>	// Needed for _In_z_ etc annotations
#endif

// ----------------------------------------------------------------------------

#ifndef LOGURU_EXPORT
	// Define to your project's export declaration if needed for use in a shared library.
	#define LOGURU_EXPORT
#endif

#ifndef LOGURU_SCOPE_TEXT_SIZE
	// Maximum length of text that can be printed by a LOG_SCOPE.
	// This should be long enough to get most things, but short enough not to clutter the stack.
	#define LOGURU_SCOPE_TEXT_SIZE 196
#endif

#ifndef LOGURU_FILENAME_WIDTH
	// Width of the column containing the file name
	#define LOGURU_FILENAME_WIDTH 23
#endif

#ifndef LOGURU_THREADNAME_WIDTH
	// Width of the column containing the thread name
	#define LOGURU_THREADNAME_WIDTH 16
#endif

#ifndef LOGURU_SCOPE_TIME_PRECISION
	// Resolution of scope timers. 3=ms, 6=us, 9=ns
	#define LOGURU_SCOPE_TIME_PRECISION 3
#endif

#ifdef LOGURU_CATCH_SIGABRT
	#error "You are defining LOGURU_CATCH_SIGABRT. his is for older versions of Loguru. You should now instead set the options passed to loguru::init"
#endif

#ifndef LOGURU_VERBOSE_SCOPE_ENDINGS
	// Show milliseconds and scope name at end of scope.
	#define LOGURU_VERBOSE_SCOPE_ENDINGS 1
#endif

#ifndef LOGURU_REDEFINE_ASSERT
	#define LOGURU_REDEFINE_ASSERT 0
#endif

#ifndef LOGURU_WITH_STREAMS
	#define LOGURU_WITH_STREAMS 0
#endif

#ifndef LOGURU_REPLACE_GLOG
	#define LOGURU_REPLACE_GLOG 0
#endif

#if LOGURU_REPLACE_GLOG
	#undef LOGURU_WITH_STREAMS
	#define LOGURU_WITH_STREAMS 1
#endif

#if defined(LOGURU_UNSAFE_SIGNAL_HANDLER)
	#error "You are defining LOGURU_UNSAFE_SIGNAL_HANDLER. This is for older versions of Loguru. You should now instead set the unsafe_signal_handler option when you call loguru::init."
#endif

#if LOGURU_IMPLEMENTATION
	#undef LOGURU_WITH_STREAMS
	#define LOGURU_WITH_STREAMS 1
#endif

#ifndef LOGURU_USE_FMTLIB
	#define LOGURU_USE_FMTLIB 0
#endif

#ifndef LOGURU_WITH_FILEABS
	#define LOGURU_WITH_FILEABS 0
#endif

#ifndef LOGURU_RTTI
#if defined(__clang__)
	#if __has_feature(cxx_rtti)
		#define LOGURU_RTTI 1
	#endif
#elif defined(__GNUG__)
	#if defined(__GXX_RTTI)
		#define LOGURU_RTTI 1
	#endif
#elif defined(_MSC_VER)
	#if defined(_CPPRTTI)
		#define LOGURU_RTTI 1
	#endif
#endif
#endif

// --------------------------------------------------------------------
// Utility macros

#define LOGURU_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define LOGURU_CONCATENATE(s1, s2) LOGURU_CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#   define LOGURU_ANONYMOUS_VARIABLE(str) LOGURU_CONCATENATE(str, __COUNTER__)
#else
#   define LOGURU_ANONYMOUS_VARIABLE(str) LOGURU_CONCATENATE(str, __LINE__)
#endif

#if defined(__clang__) || defined(__GNUC__)
	// Helper macro for declaring functions as having similar signature to printf.
	// This allows the compiler to catch format errors at compile-time.
	#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg) __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
	#define LOGURU_FORMAT_STRING_TYPE const char*
#elif defined(_MSC_VER)
	#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg)
	#define LOGURU_FORMAT_STRING_TYPE _In_z_ _Printf_format_string_ const char*
#else
	#define LOGURU_PRINTF_LIKE(fmtarg, firstvararg)
	#define LOGURU_FORMAT_STRING_TYPE const char*
#endif

// Used to mark log_and_abort for the benefit of the static analyzer and optimizer.
#if defined(_MSC_VER)
#define LOGURU_NORETURN __declspec(noreturn)
#else
#define LOGURU_NORETURN __attribute__((noreturn))
#endif

#if defined(_MSC_VER)
#define LOGURU_PREDICT_FALSE(x) (x)
#define LOGURU_PREDICT_TRUE(x)  (x)
#else
#define LOGURU_PREDICT_FALSE(x) (__builtin_expect(x,     0))
#define LOGURU_PREDICT_TRUE(x)  (__builtin_expect(!!(x), 1))
#endif

#if LOGURU_USE_FMTLIB
	#include <fmt/format.h>
	#define LOGURU_FMT(x) "{:" #x "}"
#else
	#define LOGURU_FMT(x) "%" #x
#endif

#ifdef _WIN32
	#define STRDUP(str) _strdup(str)
#else
	#define STRDUP(str) strdup(str)
#endif

// --------------------------------------------------------------------

namespace loguru
{
	// Simple RAII ownership of a char*.
	class LOGURU_EXPORT Text
	{
	public:
		explicit Text(char* owned_str) : _str(owned_str) {}
		~Text();
		Text(Text&& t)
		{
			_str = t._str;
			t._str = nullptr;
		}
		Text(Text& t) = delete;
		Text& operator=(Text& t) = delete;
		void operator=(Text&& t) = delete;

		const char* c_str() const { return _str; }
		bool empty() const { return _str == nullptr || *_str == '\0'; }

		char* release()
		{
			auto result = _str;
			_str = nullptr;
			return result;
		}

	private:
		char* _str;
	};

	// Like printf, but returns the formated text.
#if LOGURU_USE_FMTLIB
	LOGURU_EXPORT
	Text vtextprintf(const char* format, fmt::format_args args);

	template<typename... Args>
	LOGURU_EXPORT
	Text textprintf(LOGURU_FORMAT_STRING_TYPE format, const Args&... args) {
		return vtextprintf(format, fmt::make_format_args(args...));
	}
#else
	LOGURU_EXPORT
	Text textprintf(LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(1, 2);
#endif

	// Overloaded for variadic template matching.
	LOGURU_EXPORT
	Text textprintf();

	using Verbosity = int;

#undef FATAL
#undef ERROR
#undef WARNING
#undef INFO
#undef MAX

	enum NamedVerbosity : Verbosity
	{
		// Used to mark an invalid verbosity. Do not log to this level.
		Verbosity_INVALID = -10, // Never do LOG_F(INVALID)

		// You may use Verbosity_OFF on g_stderr_verbosity, but for nothing else!
		Verbosity_OFF     = -9, // Never do LOG_F(OFF)

		// Prefer to use ABORT_F or ABORT_S over LOG_F(FATAL) or LOG_S(FATAL).
		Verbosity_FATAL   = -3,
		Verbosity_ERROR   = -2,
		Verbosity_WARNING = -1,

		// Normal messages. By default written to stderr.
		Verbosity_INFO    =  0,

		// Same as Verbosity_INFO in every way.
		Verbosity_0       =  0,

		// Verbosity levels 1-9 are generally not written to stderr, but are written to file.
		Verbosity_1       = +1,
		Verbosity_2       = +2,
		Verbosity_3       = +3,
		Verbosity_4       = +4,
		Verbosity_5       = +5,
		Verbosity_6       = +6,
		Verbosity_7       = +7,
		Verbosity_8       = +8,
		Verbosity_9       = +9,

		// Don not use higher verbosity levels, as that will make grepping log files harder.
		Verbosity_MAX     = +9,
	};

	struct Message
	{
		// You would generally print a Message by just concating the buffers without spacing.
		// Optionally, ignore preamble and indentation.
		Verbosity   verbosity;   // Already part of preamble
		const char* filename;    // Already part of preamble
		unsigned    line;        // Already part of preamble
		const char* preamble;    // Date, time, uptime, thread, file:line, verbosity.
		const char* indentation; // Just a bunch of spacing.
		const char* prefix;      // Assertion failure info goes here (or "").
		const char* message;     // User message goes here.
	};

	/* Everything with a verbosity equal or greater than g_stderr_verbosity will be
	written to stderr. You can set this in code or via the -v argument.
	Set to loguru::Verbosity_OFF to write nothing to stderr.
	Default is 0, i.e. only log ERROR, WARNING and INFO are written to stderr.
	*/
	LOGURU_EXPORT extern Verbosity g_stderr_verbosity;
	LOGURU_EXPORT extern bool      g_colorlogtostderr; // True by default.
	LOGURU_EXPORT extern unsigned  g_flush_interval_ms; // 0 (unbuffered) by default.
	LOGURU_EXPORT extern bool      g_preamble; // Prefix each log line with date, time etc? True by default.

	/* Specify the verbosity used by loguru to log its info messages including the header
	logged when logged::init() is called or on exit. Default is 0 (INFO).
	*/
	LOGURU_EXPORT extern Verbosity g_internal_verbosity;

	// Turn off individual parts of the preamble
	LOGURU_EXPORT extern bool      g_preamble_date; // The date field
	LOGURU_EXPORT extern bool      g_preamble_time; // The time of the current day
	LOGURU_EXPORT extern bool      g_preamble_uptime; // The time since init call
	LOGURU_EXPORT extern bool      g_preamble_thread; // The logging thread
	LOGURU_EXPORT extern bool      g_preamble_file; // The file from which the log originates from
	LOGURU_EXPORT extern bool      g_preamble_verbose; // The verbosity field
	LOGURU_EXPORT extern bool      g_preamble_pipe; // The pipe symbol right before the message

	// May not throw!
	typedef void (*log_handler_t)(void* user_data, const Message& message);
	typedef void (*close_handler_t)(void* user_data);
	typedef void (*flush_handler_t)(void* user_data);

	// May throw if that's how you'd like to handle your errors.
	typedef void (*fatal_handler_t)(const Message& message);

	// Given a verbosity level, return the level's name or nullptr.
	typedef const char* (*verbosity_to_name_t)(Verbosity verbosity);

	// Given a verbosity level name, return the verbosity level or
	// Verbosity_INVALID if name is not recognized.
	typedef Verbosity (*name_to_verbosity_t)(const char* name);

	struct SignalOptions
	{
		/// Make Loguru try to do unsafe but useful things,
		/// like printing a stack trace, when catching signals.
		/// This may lead to bad things like deadlocks in certain situations.
		bool unsafe_signal_handler = true;

		/// Should Loguru catch SIGABRT ?
		bool sigabrt = true;

		/// Should Loguru catch SIGBUS ?
		bool sigbus = true;

		/// Should Loguru catch SIGFPE ?
		bool sigfpe = true;

		/// Should Loguru catch SIGILL ?
		bool sigill = true;

		/// Should Loguru catch SIGINT ?
		bool sigint = true;

		/// Should Loguru catch SIGSEGV ?
		bool sigsegv = true;

		/// Should Loguru catch SIGTERM ?
		bool sigterm = true;

		static SignalOptions none()
		{
			SignalOptions options;
			options.unsafe_signal_handler = false;
			options.sigabrt = false;
			options.sigbus = false;
			options.sigfpe = false;
			options.sigill = false;
			options.sigint = false;
			options.sigsegv = false;
			options.sigterm = false;
			return options;
		}
	};

	// Runtime options passed to loguru::init
	struct Options
	{
		// This allows you to use something else instead of "-v" via verbosity_flag.
		// Set to nullptr to if you don't want Loguru to parse verbosity from the args.'
		const char* verbosity_flag = "-v";

		// loguru::init will set the name of the calling thread to this.
		// If you don't want Loguru to set the name of the main thread,
		// set this to nullptr.
		// NOTE: on SOME platforms loguru::init will only overwrite the thread name
		// if a thread name has not already been set.
		// To always set a thread name, use loguru::set_thread_name instead.
		const char* main_thread_name = "main thread";

		SignalOptions signal_options;
	};

	/*  Should be called from the main thread.
		You don't *need* to call this, but if you do you get:
			* Signal handlers installed
			* Program arguments logged
			* Working dir logged
			* Optional -v verbosity flag parsed
			* Main thread name set to "main thread"
			* Explanation of the preamble (date, threanmae etc) logged

		loguru::init() will look for arguments meant for loguru and remove them.
		Arguments meant for loguru are:
			-v n   Set loguru::g_stderr_verbosity level. Examples:
				-v 3        Show verbosity level 3 and lower.
				-v 0        Only show INFO, WARNING, ERROR, FATAL (default).
				-v INFO     Only show INFO, WARNING, ERROR, FATAL (default).
				-v WARNING  Only show WARNING, ERROR, FATAL.
				-v ERROR    Only show ERROR, FATAL.
				-v FATAL    Only show FATAL.
				-v OFF      Turn off logging to stderr.

		Tip: You can set g_stderr_verbosity before calling loguru::init.
		That way you can set the default but have the user override it with the -v flag.
		Note that -v does not affect file logging (see loguru::add_file).

		You can you something other than the -v flag by setting the verbosity_flag option.
	*/
	LOGURU_EXPORT
	void init(int& argc, char* argv[], const Options& options = {});

	// Will call remove_all_callbacks(). After calling this, logging will still go to stderr.
	// You generally don't need to call this.
	LOGURU_EXPORT
	void shutdown();

	// What ~ will be replaced with, e.g. "/home/your_user_name/"
	LOGURU_EXPORT
	const char* home_dir();

	/* Returns the name of the app as given in argv[0] but without leading path.
	   That is, if argv[0] is "../foo/app" this will return "app".
	*/
	LOGURU_EXPORT
	const char* argv0_filename();

	// Returns all arguments given to loguru::init(), but escaped with a single space as separator.
	LOGURU_EXPORT
	const char* arguments();

	// Returns the path to the current working dir when loguru::init() was called.
	LOGURU_EXPORT
	const char* current_dir();

	// Returns the part of the path after the last / or \ (if any).
	LOGURU_EXPORT
	const char* filename(const char* path);

	// e.g. "foo/bar/baz.ext" will create the directories "foo/" and "foo/bar/"
	LOGURU_EXPORT
	bool create_directories(const char* file_path_const);

	// Writes date and time with millisecond precision, e.g. "20151017_161503.123"
	LOGURU_EXPORT
	void write_date_time(char* buff, unsigned buff_size);

	// Helper: thread-safe version strerror
	LOGURU_EXPORT
	Text errno_as_text();

	/* Given a prefix of e.g. "~/loguru/" this might return
	   "/home/your_username/loguru/app_name/20151017_161503.123.log"

	   where "app_name" is a sanitized version of argv[0].
	*/
	LOGURU_EXPORT
	void suggest_log_path(const char* prefix, char* buff, unsigned buff_size);

	enum FileMode { Truncate, Append };

	/*  Will log to a file at the given path.
		Any logging message with a verbosity lower or equal to
		the given verbosity will be included.
		The function will create all directories in 'path' if needed.
		If path starts with a ~, it will be replaced with loguru::home_dir()
		To stop the file logging, just call loguru::remove_callback(path) with the same path.
	*/
	LOGURU_EXPORT
	bool add_file(const char* path, FileMode mode, Verbosity verbosity);

	/*  Will be called right before abort().
		You can for instance use this to print custom error messages, or throw an exception.
		Feel free to call LOG:ing function from this, but not FATAL ones! */
	LOGURU_EXPORT
	void set_fatal_handler(fatal_handler_t handler);

	// Get the current fatal handler, if any. Default value is nullptr.
	LOGURU_EXPORT
	fatal_handler_t get_fatal_handler();

	/*  Will be called on each log messages with a verbosity less or equal to the given one.
		Useful for displaying messages on-screen in a game, for example.
		The given on_close is also expected to flush (if desired).
	*/
	LOGURU_EXPORT
	void add_callback(
		const char*     id,
		log_handler_t   callback,
		void*           user_data,
		Verbosity       verbosity,
		close_handler_t on_close = nullptr,
		flush_handler_t on_flush = nullptr);

	/*  Set a callback that returns custom verbosity level names. If callback
		is nullptr or returns nullptr, default log names will be used.
	*/
	LOGURU_EXPORT
	void set_verbosity_to_name_callback(verbosity_to_name_t callback);

	/*  Set a callback that returns the verbosity level matching a name. The
		callback should return Verbosity_INVALID if the name is not
		recognized.
	*/
	LOGURU_EXPORT
	void set_name_to_verbosity_callback(name_to_verbosity_t callback);

	/*  Get a custom name for a specific verbosity, if one exists, or nullptr. */
	LOGURU_EXPORT
	const char* get_verbosity_name(Verbosity verbosity);

	/*  Get the verbosity enum value from a custom 4-character level name, if one exists.
		If the name does not match a custom level name, Verbosity_INVALID is returned.
	*/
	LOGURU_EXPORT
	Verbosity get_verbosity_from_name(const char* name);

	// Returns true iff the callback was found (and removed).
	LOGURU_EXPORT
	bool remove_callback(const char* id);

	// Shut down all file logging and any other callback hooks installed.
	LOGURU_EXPORT
	void remove_all_callbacks();

	// Returns the maximum of g_stderr_verbosity and all file/custom outputs.
	LOGURU_EXPORT
	Verbosity current_verbosity_cutoff();

#if LOGURU_USE_FMTLIB
	// Internal functions
	void vlog(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, fmt::format_args args);
	void raw_vlog(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, fmt::format_args args);

	// Actual logging function. Use the LOG macro instead of calling this directly.
	template <typename... Args>
	LOGURU_EXPORT
	void log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, const Args &... args) {
	    vlog(verbosity, file, line, format, fmt::make_format_args(args...));
	}

	// Log without any preamble or indentation.
	template <typename... Args>
	LOGURU_EXPORT
	void raw_log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, const Args &... args) {
	    raw_vlog(verbosity, file, line, format, fmt::make_format_args(args...));
	}
#else // LOGURU_USE_FMTLIB?
	// Actual logging function. Use the LOG macro instead of calling this directly.
	LOGURU_EXPORT
	void log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(4, 5);

	// Log without any preamble or indentation.
	LOGURU_EXPORT
	void raw_log(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(4, 5);
#endif // !LOGURU_USE_FMTLIB

	// Helper class for LOG_SCOPE_F
	class LOGURU_EXPORT LogScopeRAII
	{
	public:
		LogScopeRAII() : _file(nullptr) {} // No logging
		LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(5, 6);
		~LogScopeRAII();

#if defined(_MSC_VER) && _MSC_VER > 1800
		// older MSVC default move ctors close the scope on move. See
		// issue #43
		LogScopeRAII(LogScopeRAII&& other)
			: _verbosity(other._verbosity)
			, _file(other._file)
			, _line(other._line)
			, _indent_stderr(other._indent_stderr)
			, _start_time_ns(other._start_time_ns)
		{
			// Make sure the tmp object's destruction doesn't close the scope:
			other._file = nullptr;

			for (unsigned int i = 0; i < LOGURU_SCOPE_TEXT_SIZE; ++i) {
				_name[i] = other._name[i];
			}
		}
#else
		LogScopeRAII(LogScopeRAII&&) = default;
#endif

	private:
		LogScopeRAII(const LogScopeRAII&) = delete;
		LogScopeRAII& operator=(const LogScopeRAII&) = delete;
		void operator=(LogScopeRAII&&) = delete;

		Verbosity   _verbosity;
		const char* _file; // Set to null if we are disabled due to verbosity
		unsigned    _line;
		bool        _indent_stderr; // Did we?
		long long   _start_time_ns;
		char        _name[LOGURU_SCOPE_TEXT_SIZE];
	};

	// Marked as 'noreturn' for the benefit of the static analyzer and optimizer.
	// stack_trace_skip is the number of extrace stack frames to skip above log_and_abort.
#if LOGURU_USE_FMTLIB
	LOGURU_EXPORT
	LOGURU_NORETURN void vlog_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, fmt::format_args);
	template <typename... Args>
	LOGURU_EXPORT
	LOGURU_NORETURN void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, const Args&... args) {
	    vlog_and_abort(stack_trace_skip, expr, file, line, format, fmt::make_format_args(args...));
	}
#else
	LOGURU_EXPORT
	LOGURU_NORETURN void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(5, 6);
#endif
	LOGURU_EXPORT
	LOGURU_NORETURN void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line);

	// Flush output to stderr and files.
	// If g_flush_interval_ms is set to non-zero, this will be called automatically this often.
	// If not set, you do not need to call this at all.
	LOGURU_EXPORT
	void flush();

	template<class T> inline Text format_value(const T&)                    { return textprintf("N/A");     }
	template<>        inline Text format_value(const char& v)               { return textprintf(LOGURU_FMT(c),   v); }
	template<>        inline Text format_value(const int& v)                { return textprintf(LOGURU_FMT(d),   v); }
	template<>        inline Text format_value(const unsigned int& v)       { return textprintf(LOGURU_FMT(u),   v); }
	template<>        inline Text format_value(const long& v)               { return textprintf(LOGURU_FMT(lu),  v); }
	template<>        inline Text format_value(const unsigned long& v)      { return textprintf(LOGURU_FMT(ld),  v); }
	template<>        inline Text format_value(const long long& v)          { return textprintf(LOGURU_FMT(llu), v); }
	template<>        inline Text format_value(const unsigned long long& v) { return textprintf(LOGURU_FMT(lld), v); }
	template<>        inline Text format_value(const float& v)              { return textprintf(LOGURU_FMT(f),   v); }
	template<>        inline Text format_value(const double& v)             { return textprintf(LOGURU_FMT(f),   v); }

	/* Thread names can be set for the benefit of readable logs.
	   If you do not set the thread name, a hex id will be shown instead.
	   These thread names may or may not be the same as the system thread names,
	   depending on the system.
	   Try to limit the thread name to 15 characters or less. */
	LOGURU_EXPORT
	void set_thread_name(const char* name);

	/* Returns the thread name for this thread.
	   On most *nix systems this will return the system thread name (settable from both within and without Loguru).
	   On other systems it will return whatever you set in `set_thread_name()`;
	   If no thread name is set, this will return a hexadecimal thread id.
	   `length` should be the number of bytes available in the buffer.
	   17 is a good number for length.
	   `right_align_hex_id` means any hexadecimal thread id will be written to the end of buffer.
	*/
	LOGURU_EXPORT
	void get_thread_name(char* buffer, unsigned long long length, bool right_align_hex_id);

	/* Generates a readable stacktrace as a string.
	   'skip' specifies how many stack frames to skip.
	   For instance, the default skip (1) means:
	   don't include the call to loguru::stacktrace in the stack trace. */
	LOGURU_EXPORT
	Text stacktrace(int skip = 1);

	/*  Add a string to be replaced with something else in the stack output.

		For instance, instead of having a stack trace look like this:
			0x41f541 some_function(std::basic_ofstream<char, std::char_traits<char> >&)
		You can clean it up with:
			auto verbose_type_name = loguru::demangle(typeid(std::ofstream).name());
			loguru::add_stack_cleanup(verbose_type_name.c_str(); "std::ofstream");
		So the next time you will instead see:
			0x41f541 some_function(std::ofstream&)

		`replace_with_this` must be shorter than `find_this`.
	*/
	LOGURU_EXPORT
	void add_stack_cleanup(const char* find_this, const char* replace_with_this);

	// Example: demangle(typeid(std::ofstream).name()) -> "std::basic_ofstream<char, std::char_traits<char> >"
	LOGURU_EXPORT
	Text demangle(const char* name);

	// ------------------------------------------------------------------------
	/*
	Not all terminals support colors, but if they do, and g_colorlogtostderr
	is set, Loguru will write them to stderr to make errors in red, etc.

	You also have the option to manually use them, via the function below.

	Note, however, that if you do, the color codes could end up in your logfile!

	This means if you intend to use them functions you should either:
		* Use them on the stderr/stdout directly (bypass Loguru).
		* Don't add file outputs to Loguru.
		* Expect some \e[1m things in your logfile.

	Usage:
		printf("%sRed%sGreen%sBold green%sClear again\n",
			   loguru::terminal_red(), loguru::terminal_green(),
			   loguru::terminal_bold(), loguru::terminal_reset());

	If the terminal at hand does not support colors the above output
	will just not have funky \e[1m things showing.
	*/

	// Do the output terminal support colors?
	LOGURU_EXPORT
	bool terminal_has_color();

	// Colors
	LOGURU_EXPORT const char* terminal_black();
	LOGURU_EXPORT const char* terminal_red();
	LOGURU_EXPORT const char* terminal_green();
	LOGURU_EXPORT const char* terminal_yellow();
	LOGURU_EXPORT const char* terminal_blue();
	LOGURU_EXPORT const char* terminal_purple();
	LOGURU_EXPORT const char* terminal_cyan();
	LOGURU_EXPORT const char* terminal_light_gray();
	LOGURU_EXPORT const char* terminal_light_red();
	LOGURU_EXPORT const char* terminal_white();

	// Formating
	LOGURU_EXPORT const char* terminal_bold();
	LOGURU_EXPORT const char* terminal_underline();

	// You should end each line with this!
	LOGURU_EXPORT const char* terminal_reset();

	// --------------------------------------------------------------------
	// Error context related:

	struct StringStream;

	// Use this in your EcEntryBase::print_value overload.
	LOGURU_EXPORT
	void stream_print(StringStream& out_string_stream, const char* text);

	class LOGURU_EXPORT EcEntryBase
	{
	public:
		EcEntryBase(const char* file, unsigned line, const char* descr);
		~EcEntryBase();
		EcEntryBase(const EcEntryBase&) = delete;
		EcEntryBase(EcEntryBase&&) = delete;
		EcEntryBase& operator=(const EcEntryBase&) = delete;
		EcEntryBase& operator=(EcEntryBase&&) = delete;

		virtual void print_value(StringStream& out_string_stream) const = 0;

		EcEntryBase* previous() const { return _previous; }

	// private:
		const char*  _file;
		unsigned     _line;
		const char*  _descr;
		EcEntryBase* _previous;
	};

	template<typename T>
	class EcEntryData : public EcEntryBase
	{
	public:
		using Printer = Text(*)(T data);

		EcEntryData(const char* file, unsigned line, const char* descr, T data, Printer&& printer)
			: EcEntryBase(file, line, descr), _data(data), _printer(printer) {}

		virtual void print_value(StringStream& out_string_stream) const override
		{
			const auto str = _printer(_data);
			stream_print(out_string_stream, str.c_str());
		}

	private:
		T       _data;
		Printer _printer;
	};

	// template<typename Printer>
	// class EcEntryLambda : public EcEntryBase
	// {
	// public:
	// 	EcEntryLambda(const char* file, unsigned line, const char* descr, Printer&& printer)
	// 		: EcEntryBase(file, line, descr), _printer(std::move(printer)) {}

	// 	virtual void print_value(StringStream& out_string_stream) const override
	// 	{
	// 		const auto str = _printer();
	// 		stream_print(out_string_stream, str.c_str());
	// 	}

	// private:
	// 	Printer _printer;
	// };

	// template<typename Printer>
	// EcEntryLambda<Printer> make_ec_entry_lambda(const char* file, unsigned line, const char* descr, Printer&& printer)
	// {
	// 	return {file, line, descr, std::move(printer)};
	// }

	template <class T>
	struct decay_char_array { using type = T; };

	template <unsigned long long  N>
	struct decay_char_array<const char(&)[N]> { using type = const char*; };

	template <class T>
	struct make_const_ptr { using type = T; };

	template <class T>
	struct make_const_ptr<T*> { using type = const T*; };

	template <class T>
	struct make_ec_type { using type = typename make_const_ptr<typename decay_char_array<T>::type>::type; };

	/* 	A stack trace gives you the names of the function at the point of a crash.
		With ERROR_CONTEXT, you can also get the values of select local variables.
		Usage:

		void process_customers(const std::string& filename)
		{
			ERROR_CONTEXT("Processing file", filename.c_str());
			for (int customer_index : ...)
			{
				ERROR_CONTEXT("Customer index", customer_index);
				...
			}
		}

		The context is in effect during the scope of the ERROR_CONTEXT.
		Use loguru::get_error_context() to get the contents of the active error contexts.

		Example result:

		------------------------------------------------
		[ErrorContext]                main.cpp:416   Processing file:    "customers.json"
		[ErrorContext]                main.cpp:417   Customer index:     42
		------------------------------------------------

		Error contexts are printed automatically on crashes, and only on crashes.
		This makes them much faster than logging the value of a variable.
	*/
	#define ERROR_CONTEXT(descr, data)                                             \
		const loguru::EcEntryData<loguru::make_ec_type<decltype(data)>::type>      \
			LOGURU_ANONYMOUS_VARIABLE(error_context_scope_)(                       \
				__FILE__, __LINE__, descr, data,                                   \
				static_cast<loguru::EcEntryData<loguru::make_ec_type<decltype(data)>::type>::Printer>(loguru::ec_to_text) ) // For better error messages

/*
	#define ERROR_CONTEXT(descr, data)                                 \
		const auto LOGURU_ANONYMOUS_VARIABLE(error_context_scope_)(    \
			loguru::make_ec_entry_lambda(__FILE__, __LINE__, descr,    \
				[=](){ return loguru::ec_to_text(data); }))
*/

	using EcHandle = const EcEntryBase*;

	/*
		Get a light-weight handle to the error context stack on this thread.
		The handle is valid as long as the current thread has no changes to its error context stack.
		You can pass the handle to loguru::get_error_context on another thread.
		This can be very useful for when you have a parent thread spawning several working threads,
		and you want the error context of the parent thread to get printed (too) when there is an
		error on the child thread. You can accomplish this thusly:

		void foo(const char* parameter)
		{
			ERROR_CONTEXT("parameter", parameter)
			const auto parent_ec_handle = loguru::get_thread_ec_handle();

			std::thread([=]{
				loguru::set_thread_name("child thread");
				ERROR_CONTEXT("parent context", parent_ec_handle);
				dangerous_code();
			}.join();
		}

	*/
	LOGURU_EXPORT
	EcHandle get_thread_ec_handle();

	// Get a string describing the current stack of error context. Empty string if there is none.
	LOGURU_EXPORT
	Text get_error_context();

	// Get a string describing the error context of the given thread handle.
	LOGURU_EXPORT
	Text get_error_context_for(EcHandle ec_handle);

	// ------------------------------------------------------------------------

	LOGURU_EXPORT Text ec_to_text(const char* data);
	LOGURU_EXPORT Text ec_to_text(char data);
	LOGURU_EXPORT Text ec_to_text(int data);
	LOGURU_EXPORT Text ec_to_text(unsigned int data);
	LOGURU_EXPORT Text ec_to_text(long data);
	LOGURU_EXPORT Text ec_to_text(unsigned long data);
	LOGURU_EXPORT Text ec_to_text(long long data);
	LOGURU_EXPORT Text ec_to_text(unsigned long long data);
	LOGURU_EXPORT Text ec_to_text(float data);
	LOGURU_EXPORT Text ec_to_text(double data);
	LOGURU_EXPORT Text ec_to_text(long double data);
	LOGURU_EXPORT Text ec_to_text(EcHandle);

	/*
	You can add ERROR_CONTEXT support for your own types by overloading ec_to_text. Here's how:

	some.hpp:
		namespace loguru {
			Text ec_to_text(MySmallType data)
			Text ec_to_text(const MyBigType* data)
		} // namespace loguru

	some.cpp:
		namespace loguru {
			Text ec_to_text(MySmallType small_value)
			{
				// Called only when needed, i.e. on a crash.
				std::string str = small_value.as_string(); // Format 'small_value' here somehow.
				return Text{STRDUP(str.c_str())};
			}

			Text ec_to_text(const MyBigType* big_value)
			{
				// Called only when needed, i.e. on a crash.
				std::string str = big_value->as_string(); // Format 'big_value' here somehow.
				return Text{STRDUP(str.c_str())};
			}
		} // namespace loguru

	Any file that include some.hpp:
		void foo(MySmallType small, const MyBigType& big)
		{
			ERROR_CONTEXT("Small", small); // Copy ´small` by value.
			ERROR_CONTEXT("Big",   &big);  // `big` should not change during this scope!
			....
		}
	*/
} // namespace loguru

// --------------------------------------------------------------------
// Logging macros

// LOG_F(2, "Only logged if verbosity is 2 or higher: %d", some_number);
#define VLOG_F(verbosity, ...)                                                                     \
	((verbosity) > loguru::current_verbosity_cutoff()) ? (void)0                                   \
									  : loguru::log(verbosity, __FILE__, __LINE__, __VA_ARGS__)

// LOG_F(INFO, "Foo: %d", some_number);
#define LOG_F(verbosity_name, ...) VLOG_F(loguru::Verbosity_ ## verbosity_name, __VA_ARGS__)

#define VLOG_IF_F(verbosity, cond, ...)                                                            \
	((verbosity) > loguru::current_verbosity_cutoff() || (cond) == false)                          \
		? (void)0                                                                                  \
		: loguru::log(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_IF_F(verbosity_name, cond, ...)                                                        \
	VLOG_IF_F(loguru::Verbosity_ ## verbosity_name, cond, __VA_ARGS__)

#define VLOG_SCOPE_F(verbosity, ...)                                                               \
	loguru::LogScopeRAII LOGURU_ANONYMOUS_VARIABLE(error_context_RAII_) =                          \
	((verbosity) > loguru::current_verbosity_cutoff()) ? loguru::LogScopeRAII() :                  \
	loguru::LogScopeRAII(verbosity, __FILE__, __LINE__, __VA_ARGS__)

// Raw logging - no preamble, no indentation. Slightly faster than full logging.
#define RAW_VLOG_F(verbosity, ...)                                                                 \
	((verbosity) > loguru::current_verbosity_cutoff()) ? (void)0                                   \
									  : loguru::raw_log(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define RAW_LOG_F(verbosity_name, ...) RAW_VLOG_F(loguru::Verbosity_ ## verbosity_name, __VA_ARGS__)

// Use to book-end a scope. Affects logging on all threads.
#define LOG_SCOPE_F(verbosity_name, ...)                                                           \
	VLOG_SCOPE_F(loguru::Verbosity_ ## verbosity_name, __VA_ARGS__)

#define LOG_SCOPE_FUNCTION(verbosity_name) LOG_SCOPE_F(verbosity_name, __func__)

// -----------------------------------------------
// ABORT_F macro. Usage:  ABORT_F("Cause of error: %s", error_str);

// Message is optional
#define ABORT_F(...) loguru::log_and_abort(0, "ABORT: ", __FILE__, __LINE__, __VA_ARGS__)

// --------------------------------------------------------------------
// CHECK_F macros:

#define CHECK_WITH_INFO_F(test, info, ...)                                                         \
	LOGURU_PREDICT_TRUE((test) == true) ? (void)0 : loguru::log_and_abort(0, "CHECK FAILED:  " info "  ", __FILE__,      \
													   __LINE__, ##__VA_ARGS__)

/* Checked at runtime too. Will print error, then call fatal_handler (if any), then 'abort'.
   Note that the test must be boolean.
   CHECK_F(ptr); will not compile, but CHECK_F(ptr != nullptr); will. */
#define CHECK_F(test, ...) CHECK_WITH_INFO_F(test, #test, ##__VA_ARGS__)

#define CHECK_NOTNULL_F(x, ...) CHECK_WITH_INFO_F((x) != nullptr, #x " != nullptr", ##__VA_ARGS__)

#define CHECK_OP_F(expr_left, expr_right, op, ...)                                                 \
	do                                                                                             \
	{                                                                                              \
		auto val_left = expr_left;                                                                 \
		auto val_right = expr_right;                                                               \
		if (! LOGURU_PREDICT_TRUE(val_left op val_right))                                          \
		{                                                                                          \
			auto str_left = loguru::format_value(val_left);                                        \
			auto str_right = loguru::format_value(val_right);                                      \
			auto fail_info = loguru::textprintf("CHECK FAILED:  " LOGURU_FMT(s) " " LOGURU_FMT(s) " " LOGURU_FMT(s) "  (" LOGURU_FMT(s) " " LOGURU_FMT(s) " " LOGURU_FMT(s) ")  ",           \
				#expr_left, #op, #expr_right, str_left.c_str(), #op, str_right.c_str());           \
			auto user_msg = loguru::textprintf(__VA_ARGS__);                                       \
			loguru::log_and_abort(0, fail_info.c_str(), __FILE__, __LINE__,                        \
			                      LOGURU_FMT(s), user_msg.c_str());                                         \
		}                                                                                          \
	} while (false)

#ifndef LOGURU_DEBUG_LOGGING
	#ifndef NDEBUG
		#define LOGURU_DEBUG_LOGGING 1
	#else
		#define LOGURU_DEBUG_LOGGING 0
	#endif
#endif

#if LOGURU_DEBUG_LOGGING
	// Debug logging enabled:
	#define DLOG_F(verbosity_name, ...)     LOG_F(verbosity_name, __VA_ARGS__)
	#define DVLOG_F(verbosity, ...)         VLOG_F(verbosity, __VA_ARGS__)
	#define DLOG_IF_F(verbosity_name, ...)  LOG_IF_F(verbosity_name, __VA_ARGS__)
	#define DVLOG_IF_F(verbosity, ...)      VLOG_IF_F(verbosity, __VA_ARGS__)
	#define DRAW_LOG_F(verbosity_name, ...) RAW_LOG_F(verbosity_name, __VA_ARGS__)
	#define DRAW_VLOG_F(verbosity, ...)     RAW_VLOG_F(verbosity, __VA_ARGS__)
#else
	// Debug logging disabled:
	#define DLOG_F(verbosity_name, ...)
	#define DVLOG_F(verbosity, ...)
	#define DLOG_IF_F(verbosity_name, ...)
	#define DVLOG_IF_F(verbosity, ...)
	#define DRAW_LOG_F(verbosity_name, ...)
	#define DRAW_VLOG_F(verbosity, ...)
#endif

#define CHECK_EQ_F(a, b, ...) CHECK_OP_F(a, b, ==, ##__VA_ARGS__)
#define CHECK_NE_F(a, b, ...) CHECK_OP_F(a, b, !=, ##__VA_ARGS__)
#define CHECK_LT_F(a, b, ...) CHECK_OP_F(a, b, < , ##__VA_ARGS__)
#define CHECK_GT_F(a, b, ...) CHECK_OP_F(a, b, > , ##__VA_ARGS__)
#define CHECK_LE_F(a, b, ...) CHECK_OP_F(a, b, <=, ##__VA_ARGS__)
#define CHECK_GE_F(a, b, ...) CHECK_OP_F(a, b, >=, ##__VA_ARGS__)

#ifndef LOGURU_DEBUG_CHECKS
	#ifndef NDEBUG
		#define LOGURU_DEBUG_CHECKS 1
	#else
		#define LOGURU_DEBUG_CHECKS 0
	#endif
#endif

#if LOGURU_DEBUG_CHECKS
	// Debug checks enabled:
	#define DCHECK_F(test, ...)             CHECK_F(test, ##__VA_ARGS__)
	#define DCHECK_NOTNULL_F(x, ...)        CHECK_NOTNULL_F(x, ##__VA_ARGS__)
	#define DCHECK_EQ_F(a, b, ...)          CHECK_EQ_F(a, b, ##__VA_ARGS__)
	#define DCHECK_NE_F(a, b, ...)          CHECK_NE_F(a, b, ##__VA_ARGS__)
	#define DCHECK_LT_F(a, b, ...)          CHECK_LT_F(a, b, ##__VA_ARGS__)
	#define DCHECK_LE_F(a, b, ...)          CHECK_LE_F(a, b, ##__VA_ARGS__)
	#define DCHECK_GT_F(a, b, ...)          CHECK_GT_F(a, b, ##__VA_ARGS__)
	#define DCHECK_GE_F(a, b, ...)          CHECK_GE_F(a, b, ##__VA_ARGS__)
#else
	// Debug checks disabled:
	#define DCHECK_F(test, ...)
	#define DCHECK_NOTNULL_F(x, ...)
	#define DCHECK_EQ_F(a, b, ...)
	#define DCHECK_NE_F(a, b, ...)
	#define DCHECK_LT_F(a, b, ...)
	#define DCHECK_LE_F(a, b, ...)
	#define DCHECK_GT_F(a, b, ...)
	#define DCHECK_GE_F(a, b, ...)
#endif // NDEBUG


#if LOGURU_REDEFINE_ASSERT
	#undef assert
	#ifndef NDEBUG
		// Debug:
		#define assert(test) CHECK_WITH_INFO_F(!!(test), #test) // HACK
	#else
		#define assert(test)
	#endif
#endif // LOGURU_REDEFINE_ASSERT

#endif // LOGURU_HAS_DECLARED_FORMAT_HEADER

// ----------------------------------------------------------------------------
// .dP"Y8 888888 88""Yb 888888    db    8b    d8 .dP"Y8
// `Ybo."   88   88__dP 88__     dPYb   88b  d88 `Ybo."
// o.`Y8b   88   88"Yb  88""    dP__Yb  88YbdP88 o.`Y8b
// 8bodP'   88   88  Yb 888888 dP""""Yb 88 YY 88 8bodP'

#if LOGURU_WITH_STREAMS
#ifndef LOGURU_HAS_DECLARED_STREAMS_HEADER
#define LOGURU_HAS_DECLARED_STREAMS_HEADER

/* This file extends loguru to enable std::stream-style logging, a la Glog.
   It's an optional feature behind the LOGURU_WITH_STREAMS settings
   because including it everywhere will slow down compilation times.
*/

#include <cstdarg>
#include <sstream> // Adds about 38 kLoC on clang.
#include <string>

namespace loguru
{
	// Like sprintf, but returns the formated text.
	LOGURU_EXPORT
	std::string strprintf(LOGURU_FORMAT_STRING_TYPE format, ...) LOGURU_PRINTF_LIKE(1, 2);

	// Like vsprintf, but returns the formated text.
	LOGURU_EXPORT
	std::string vstrprintf(LOGURU_FORMAT_STRING_TYPE format, va_list) LOGURU_PRINTF_LIKE(1, 0);

	class LOGURU_EXPORT StreamLogger
	{
	public:
		StreamLogger(Verbosity verbosity, const char* file, unsigned line) : _verbosity(verbosity), _file(file), _line(line) {}
		~StreamLogger() noexcept(false);

		template<typename T>
		StreamLogger& operator<<(const T& t)
		{
			_ss << t;
			return *this;
		}

		// std::endl and other iomanip:s.
		StreamLogger& operator<<(std::ostream&(*f)(std::ostream&))
		{
			f(_ss);
			return *this;
		}

	private:
		Verbosity   _verbosity;
		const char* _file;
		unsigned    _line;
		std::ostringstream _ss;
	};

	class LOGURU_EXPORT AbortLogger
	{
	public:
		AbortLogger(const char* expr, const char* file, unsigned line) : _expr(expr), _file(file), _line(line) { }
		LOGURU_NORETURN ~AbortLogger() noexcept(false);

		template<typename T>
		AbortLogger& operator<<(const T& t)
		{
			_ss << t;
			return *this;
		}

		// std::endl and other iomanip:s.
		AbortLogger& operator<<(std::ostream&(*f)(std::ostream&))
		{
			f(_ss);
			return *this;
		}

	private:
		const char*        _expr;
		const char*        _file;
		unsigned           _line;
		std::ostringstream _ss;
	};

	class LOGURU_EXPORT Voidify
	{
	public:
		Voidify() {}
		// This has to be an operator with a precedence lower than << but higher than ?:
		void operator&(const StreamLogger&) { }
		void operator&(const AbortLogger&)  { }
	};

	/*  Helper functions for CHECK_OP_S macro.
		GLOG trick: The (int, int) specialization works around the issue that the compiler
		will not instantiate the template version of the function on values of unnamed enum type. */
	#define DEFINE_CHECK_OP_IMPL(name, op)                                                             \
		template <typename T1, typename T2>                                                            \
		inline std::string* name(const char* expr, const T1& v1, const char* op_str, const T2& v2)     \
		{                                                                                              \
			if (LOGURU_PREDICT_TRUE(v1 op v2)) { return NULL; }                                        \
			std::ostringstream ss;                                                                     \
			ss << "CHECK FAILED:  " << expr << "  (" << v1 << " " << op_str << " " << v2 << ")  ";     \
			return new std::string(ss.str());                                                          \
		}                                                                                              \
		inline std::string* name(const char* expr, int v1, const char* op_str, int v2)                 \
		{                                                                                              \
			return name<int, int>(expr, v1, op_str, v2);                                               \
		}

	DEFINE_CHECK_OP_IMPL(check_EQ_impl, ==)
	DEFINE_CHECK_OP_IMPL(check_NE_impl, !=)
	DEFINE_CHECK_OP_IMPL(check_LE_impl, <=)
	DEFINE_CHECK_OP_IMPL(check_LT_impl, < )
	DEFINE_CHECK_OP_IMPL(check_GE_impl, >=)
	DEFINE_CHECK_OP_IMPL(check_GT_impl, > )
	#undef DEFINE_CHECK_OP_IMPL

	/*  GLOG trick: Function is overloaded for integral types to allow static const integrals
		declared in classes and not defined to be used as arguments to CHECK* macros. */
	template <class T>
	inline const T&           referenceable_value(const T&           t) { return t; }
	inline char               referenceable_value(char               t) { return t; }
	inline unsigned char      referenceable_value(unsigned char      t) { return t; }
	inline signed char        referenceable_value(signed char        t) { return t; }
	inline short              referenceable_value(short              t) { return t; }
	inline unsigned short     referenceable_value(unsigned short     t) { return t; }
	inline int                referenceable_value(int                t) { return t; }
	inline unsigned int       referenceable_value(unsigned int       t) { return t; }
	inline long               referenceable_value(long               t) { return t; }
	inline unsigned long      referenceable_value(unsigned long      t) { return t; }
	inline long long          referenceable_value(long long          t) { return t; }
	inline unsigned long long referenceable_value(unsigned long long t) { return t; }
} // namespace loguru

// -----------------------------------------------
// Logging macros:

// usage:  LOG_STREAM(INFO) << "Foo " << std::setprecision(10) << some_value;
#define VLOG_IF_S(verbosity, cond)                                                                 \
	((verbosity) > loguru::current_verbosity_cutoff() || (cond) == false)                          \
		? (void)0                                                                                  \
		: loguru::Voidify() & loguru::StreamLogger(verbosity, __FILE__, __LINE__)
#define LOG_IF_S(verbosity_name, cond) VLOG_IF_S(loguru::Verbosity_ ## verbosity_name, cond)
#define VLOG_S(verbosity)              VLOG_IF_S(verbosity, true)
#define LOG_S(verbosity_name)          VLOG_S(loguru::Verbosity_ ## verbosity_name)

// -----------------------------------------------
// ABORT_S macro. Usage:  ABORT_S() << "Causo of error: " << details;

#define ABORT_S() loguru::Voidify() & loguru::AbortLogger("ABORT: ", __FILE__, __LINE__)

// -----------------------------------------------
// CHECK_S macros:

#define CHECK_WITH_INFO_S(cond, info)                                                              \
	LOGURU_PREDICT_TRUE((cond) == true)                                                            \
		? (void)0                                                                                  \
		: loguru::Voidify() & loguru::AbortLogger("CHECK FAILED:  " info "  ", __FILE__, __LINE__)

#define CHECK_S(cond) CHECK_WITH_INFO_S(cond, #cond)
#define CHECK_NOTNULL_S(x) CHECK_WITH_INFO_S((x) != nullptr, #x " != nullptr")

#define CHECK_OP_S(function_name, expr1, op, expr2)                                                \
	while (auto error_string = loguru::function_name(#expr1 " " #op " " #expr2,                    \
													 loguru::referenceable_value(expr1), #op,      \
													 loguru::referenceable_value(expr2)))          \
		loguru::AbortLogger(error_string->c_str(), __FILE__, __LINE__)

#define CHECK_EQ_S(expr1, expr2) CHECK_OP_S(check_EQ_impl, expr1, ==, expr2)
#define CHECK_NE_S(expr1, expr2) CHECK_OP_S(check_NE_impl, expr1, !=, expr2)
#define CHECK_LE_S(expr1, expr2) CHECK_OP_S(check_LE_impl, expr1, <=, expr2)
#define CHECK_LT_S(expr1, expr2) CHECK_OP_S(check_LT_impl, expr1, < , expr2)
#define CHECK_GE_S(expr1, expr2) CHECK_OP_S(check_GE_impl, expr1, >=, expr2)
#define CHECK_GT_S(expr1, expr2) CHECK_OP_S(check_GT_impl, expr1, > , expr2)

#if LOGURU_DEBUG_LOGGING
	// Debug logging enabled:
	#define DVLOG_IF_S(verbosity, cond)     VLOG_IF_S(verbosity, cond)
	#define DLOG_IF_S(verbosity_name, cond) LOG_IF_S(verbosity_name, cond)
	#define DVLOG_S(verbosity)              VLOG_S(verbosity)
	#define DLOG_S(verbosity_name)          LOG_S(verbosity_name)
#else
	// Debug logging disabled:
	#define DVLOG_IF_S(verbosity, cond)                                                     \
		(true || (verbosity) > loguru::current_verbosity_cutoff() || (cond) == false)       \
			? (void)0                                                                       \
			: loguru::Voidify() & loguru::StreamLogger(verbosity, __FILE__, __LINE__)

	#define DLOG_IF_S(verbosity_name, cond) DVLOG_IF_S(loguru::Verbosity_ ## verbosity_name, cond)
	#define DVLOG_S(verbosity)              DVLOG_IF_S(verbosity, true)
	#define DLOG_S(verbosity_name)          DVLOG_S(loguru::Verbosity_ ## verbosity_name)
#endif

#if LOGURU_DEBUG_CHECKS
	// Debug checks enabled:
	#define DCHECK_S(cond)                  CHECK_S(cond)
	#define DCHECK_NOTNULL_S(x)             CHECK_NOTNULL_S(x)
	#define DCHECK_EQ_S(a, b)               CHECK_EQ_S(a, b)
	#define DCHECK_NE_S(a, b)               CHECK_NE_S(a, b)
	#define DCHECK_LT_S(a, b)               CHECK_LT_S(a, b)
	#define DCHECK_LE_S(a, b)               CHECK_LE_S(a, b)
	#define DCHECK_GT_S(a, b)               CHECK_GT_S(a, b)
	#define DCHECK_GE_S(a, b)               CHECK_GE_S(a, b)
#else
// Debug checks disabled:
	#define DCHECK_S(cond)                  CHECK_S(true || (cond))
	#define DCHECK_NOTNULL_S(x)             CHECK_S(true || (x) != nullptr)
	#define DCHECK_EQ_S(a, b)               CHECK_S(true || (a) == (b))
	#define DCHECK_NE_S(a, b)               CHECK_S(true || (a) != (b))
	#define DCHECK_LT_S(a, b)               CHECK_S(true || (a) <  (b))
	#define DCHECK_LE_S(a, b)               CHECK_S(true || (a) <= (b))
	#define DCHECK_GT_S(a, b)               CHECK_S(true || (a) >  (b))
	#define DCHECK_GE_S(a, b)               CHECK_S(true || (a) >= (b))
#endif

#if LOGURU_REPLACE_GLOG
	#undef LOG
	#undef VLOG
	#undef LOG_IF
	#undef VLOG_IF
	#undef CHECK
	#undef CHECK_NOTNULL
	#undef CHECK_EQ
	#undef CHECK_NE
	#undef CHECK_LT
	#undef CHECK_LE
	#undef CHECK_GT
	#undef CHECK_GE
	#undef DLOG
	#undef DVLOG
	#undef DLOG_IF
	#undef DVLOG_IF
	#undef DCHECK
	#undef DCHECK_NOTNULL
	#undef DCHECK_EQ
	#undef DCHECK_NE
	#undef DCHECK_LT
	#undef DCHECK_LE
	#undef DCHECK_GT
	#undef DCHECK_GE
	#undef VLOG_IS_ON

	#define LOG            LOG_S
	#define VLOG           VLOG_S
	#define LOG_IF         LOG_IF_S
	#define VLOG_IF        VLOG_IF_S
	#define CHECK(cond)    CHECK_S(!!(cond))
	#define CHECK_NOTNULL  CHECK_NOTNULL_S
	#define CHECK_EQ       CHECK_EQ_S
	#define CHECK_NE       CHECK_NE_S
	#define CHECK_LT       CHECK_LT_S
	#define CHECK_LE       CHECK_LE_S
	#define CHECK_GT       CHECK_GT_S
	#define CHECK_GE       CHECK_GE_S
	#define DLOG           DLOG_S
	#define DVLOG          DVLOG_S
	#define DLOG_IF        DLOG_IF_S
	#define DVLOG_IF       DVLOG_IF_S
	#define DCHECK         DCHECK_S
	#define DCHECK_NOTNULL DCHECK_NOTNULL_S
	#define DCHECK_EQ      DCHECK_EQ_S
	#define DCHECK_NE      DCHECK_NE_S
	#define DCHECK_LT      DCHECK_LT_S
	#define DCHECK_LE      DCHECK_LE_S
	#define DCHECK_GT      DCHECK_GT_S
	#define DCHECK_GE      DCHECK_GE_S
	#define VLOG_IS_ON(verbosity) ((verbosity) <= loguru::current_verbosity_cutoff())

#endif // LOGURU_REPLACE_GLOG

#endif // LOGURU_WITH_STREAMS

#endif // LOGURU_HAS_DECLARED_STREAMS_HEADER
