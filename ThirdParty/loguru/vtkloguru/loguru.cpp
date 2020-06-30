#ifndef _WIN32
// Disable all warnings from gcc/clang:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"

#pragma GCC diagnostic ignored "-Wc++98-compat"
#pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-macros"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#else
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4018)
#endif // _MSC_VER
#endif

#include "loguru.hpp"

#ifndef LOGURU_HAS_BEEN_IMPLEMENTED
#define LOGURU_HAS_BEEN_IMPLEMENTED

#define LOGURU_PREAMBLE_WIDTH (53 + LOGURU_THREADNAME_WIDTH + LOGURU_FILENAME_WIDTH)

#undef min
#undef max

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
	#include <direct.h>

	#define localtime_r(a, b) localtime_s(b, a) // No localtime_r with MSVC, but arguments are swapped for localtime_s
#else
	#include <signal.h>
	#include <sys/stat.h> // mkdir
	#include <unistd.h>   // STDERR_FILENO
#endif

#ifdef __linux__
	#include <linux/limits.h> // PATH_MAX
#elif !defined(_WIN32)
	#include <limits.h> // PATH_MAX
#endif

#ifndef PATH_MAX
	#define PATH_MAX 1024
#endif

#ifdef __APPLE__
	#include "TargetConditionals.h"
#endif

// TODO: use defined(_POSIX_VERSION) for some of these things?

#if defined(_WIN32) || defined(__CYGWIN__)
	#define LOGURU_PTHREADS    0
	#define LOGURU_WINTHREADS  1
	#ifndef LOGURU_STACKTRACES
		#define LOGURU_STACKTRACES 0
	#endif
#elif defined(__rtems__) || defined(__ANDROID__)
	#define LOGURU_PTHREADS    1
	#define LOGURU_WINTHREADS  0
	#ifndef LOGURU_STACKTRACES
		#define LOGURU_STACKTRACES 0
	#endif
#else
	#define LOGURU_PTHREADS    1
	#define LOGURU_WINTHREADS  0
	#ifndef LOGURU_STACKTRACES
		#define LOGURU_STACKTRACES 1
	#endif
#endif

#if LOGURU_STACKTRACES
	#include <cxxabi.h>    // for __cxa_demangle
	#include <dlfcn.h>     // for dladdr
	#include <execinfo.h>  // for backtrace
#endif // LOGURU_STACKTRACES

#if LOGURU_PTHREADS
	#include <pthread.h>
	#if defined(__FreeBSD__)
		#include <pthread_np.h>
		#include <sys/thr.h>
	#elif defined(__OpenBSD__)
		#include <pthread_np.h>
	#endif

	#ifdef __linux__
		/* On Linux, the default thread name is the same as the name of the binary.
		   Additionally, all new threads inherit the name of the thread it got forked from.
		   For this reason, Loguru use the pthread Thread Local Storage
		   for storing thread names on Linux. */
		#define LOGURU_PTLS_NAMES 1
	#endif
#endif

#if LOGURU_WINTHREADS
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0502
	#endif
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

#ifndef LOGURU_PTLS_NAMES
   #define LOGURU_PTLS_NAMES 0
#endif


namespace loguru
{
	using namespace std::chrono;

#if LOGURU_WITH_FILEABS
	struct FileAbs
	{
		char path[PATH_MAX];
		char mode_str[4];
		Verbosity verbosity;
		struct stat st;
		FILE* fp;
		bool is_reopening = false; // to prevent recursive call in file_reopen.
		decltype(steady_clock::now()) last_check_time = steady_clock::now();
	};
#else
	typedef FILE* FileAbs;
#endif

	struct Callback
	{
		std::string     id;
		log_handler_t   callback;
		void*           user_data;
		Verbosity       verbosity; // Does not change!
		close_handler_t close;
		flush_handler_t flush;
		unsigned        indentation;
	};

	using CallbackVec = std::vector<Callback>;

	using StringPair     = std::pair<std::string, std::string>;
	using StringPairList = std::vector<StringPair>;

	const auto s_start_time = steady_clock::now();

	Verbosity g_stderr_verbosity  = Verbosity_0;
	bool      g_colorlogtostderr  = true;
	unsigned  g_flush_interval_ms = 0;
	bool      g_preamble          = true;

	Verbosity g_internal_verbosity = Verbosity_0;

	// Preamble details
	bool      g_preamble_date     = true;
	bool      g_preamble_time     = true;
	bool      g_preamble_uptime   = true;
	bool      g_preamble_thread   = true;
	bool      g_preamble_file     = true;
	bool      g_preamble_verbose  = true;
	bool      g_preamble_pipe     = true;

	static std::recursive_mutex  s_mutex;
	static Verbosity             s_max_out_verbosity = Verbosity_OFF;
	static std::string           s_argv0_filename;
	static std::string           s_arguments;
	static char                  s_current_dir[PATH_MAX];
	static CallbackVec           s_callbacks;
	static fatal_handler_t       s_fatal_handler   = nullptr;
	static verbosity_to_name_t   s_verbosity_to_name_callback = nullptr;
	static name_to_verbosity_t   s_name_to_verbosity_callback = nullptr;
	static StringPairList        s_user_stack_cleanups;
	static bool                  s_strip_file_path = true;
	static std::atomic<unsigned> s_stderr_indentation { 0 };

	// For periodic flushing:
	static std::thread* s_flush_thread   = nullptr;
	static bool         s_needs_flushing = false;

	static const bool s_terminal_has_color = [](){
		#ifdef _WIN32
			#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
			#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
			#endif

			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hOut != INVALID_HANDLE_VALUE) {
				DWORD dwMode = 0;
				GetConsoleMode(hOut, &dwMode);
				dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				return SetConsoleMode(hOut, dwMode) != 0;
			}
			return false;
		#else
			if (const char* term = getenv("TERM")) {
				return 0 == strcmp(term, "cygwin")
					|| 0 == strcmp(term, "linux")
					|| 0 == strcmp(term, "rxvt-unicode-256color")
					|| 0 == strcmp(term, "screen")
					|| 0 == strcmp(term, "screen-256color")
					|| 0 == strcmp(term, "screen.xterm-256color")
					|| 0 == strcmp(term, "tmux-256color")
					|| 0 == strcmp(term, "xterm")
					|| 0 == strcmp(term, "xterm-256color")
					|| 0 == strcmp(term, "xterm-termite")
					|| 0 == strcmp(term, "xterm-color");
			} else {
				return false;
			}
		#endif
	}();

	static void print_preamble_header(char* out_buff, size_t out_buff_size);

	#if LOGURU_PTLS_NAMES
		static pthread_once_t s_pthread_key_once = PTHREAD_ONCE_INIT;
		static pthread_key_t  s_pthread_key_name;

		void make_pthread_key_name()
		{
			(void)pthread_key_create(&s_pthread_key_name, free);
		}
	#endif

	// ------------------------------------------------------------------------------
	// Colors

	bool terminal_has_color() { return s_terminal_has_color; }

	// Colors

#ifdef _WIN32
#define VTSEQ(ID) ("\x1b[1;" #ID "m")
#else
#define VTSEQ(ID) ("\x1b[" #ID "m")
#endif

	const char* terminal_black()      { return s_terminal_has_color ? VTSEQ(30) : ""; }
	const char* terminal_red()        { return s_terminal_has_color ? VTSEQ(31) : ""; }
	const char* terminal_green()      { return s_terminal_has_color ? VTSEQ(32) : ""; }
	const char* terminal_yellow()     { return s_terminal_has_color ? VTSEQ(33) : ""; }
	const char* terminal_blue()       { return s_terminal_has_color ? VTSEQ(34) : ""; }
	const char* terminal_purple()     { return s_terminal_has_color ? VTSEQ(35) : ""; }
	const char* terminal_cyan()       { return s_terminal_has_color ? VTSEQ(36) : ""; }
	const char* terminal_light_gray() { return s_terminal_has_color ? VTSEQ(37) : ""; }
	const char* terminal_white()      { return s_terminal_has_color ? VTSEQ(37) : ""; }
	const char* terminal_light_red()  { return s_terminal_has_color ? VTSEQ(91) : ""; }
	const char* terminal_dim()        { return s_terminal_has_color ? VTSEQ(2)  : ""; }

	// Formating
	const char* terminal_bold()       { return s_terminal_has_color ? VTSEQ(1) : ""; }
	const char* terminal_underline()  { return s_terminal_has_color ? VTSEQ(4) : ""; }

	// You should end each line with this!
	const char* terminal_reset()      { return s_terminal_has_color ? VTSEQ(0) : ""; }

	// ------------------------------------------------------------------------------
#if LOGURU_WITH_FILEABS
	void file_reopen(void* user_data);
	inline FILE* to_file(void* user_data) { return reinterpret_cast<FileAbs*>(user_data)->fp; }
#else
	inline FILE* to_file(void* user_data) { return reinterpret_cast<FILE*>(user_data); }
#endif

	void file_log(void* user_data, const Message& message)
	{
#if LOGURU_WITH_FILEABS
		FileAbs* file_abs = reinterpret_cast<FileAbs*>(user_data);
		if (file_abs->is_reopening) {
			return;
		}
		// It is better checking file change every minute/hour/day,
		// instead of doing this every time we log.
		// Here check_interval is set to zero to enable checking every time;
		const auto check_interval = seconds(0);
		if (duration_cast<seconds>(steady_clock::now() - file_abs->last_check_time) > check_interval) {
			file_abs->last_check_time = steady_clock::now();
			file_reopen(user_data);
		}
		FILE* file = to_file(user_data);
		if (!file) {
			return;
		}
#else
		FILE* file = to_file(user_data);
#endif
		fprintf(file, "%s%s%s%s\n",
			message.preamble, message.indentation, message.prefix, message.message);
		if (g_flush_interval_ms == 0) {
			fflush(file);
		}
	}

	void file_close(void* user_data)
	{
		FILE* file = to_file(user_data);
		if (file) {
			fclose(file);
		}
#if LOGURU_WITH_FILEABS
		delete reinterpret_cast<FileAbs*>(user_data);
#endif
	}

	void file_flush(void* user_data)
	{
		FILE* file = to_file(user_data);
		fflush(file);
	}

#if LOGURU_WITH_FILEABS
	void file_reopen(void* user_data)
	{
		FileAbs * file_abs = reinterpret_cast<FileAbs*>(user_data);
		struct stat st;
		int ret;
		if (!file_abs->fp || (ret = stat(file_abs->path, &st)) == -1 || (st.st_ino != file_abs->st.st_ino)) {
			file_abs->is_reopening = true;
			if (file_abs->fp) {
				fclose(file_abs->fp);
			}
			if (!file_abs->fp) {
				VLOG_F(g_internal_verbosity, "Reopening file '" LOGURU_FMT(s) "' due to previous error", file_abs->path);
			}
			else if (ret < 0) {
				const auto why = errno_as_text();
				VLOG_F(g_internal_verbosity, "Reopening file '" LOGURU_FMT(s) "' due to '" LOGURU_FMT(s) "'", file_abs->path, why.c_str());
			} else {
				VLOG_F(g_internal_verbosity, "Reopening file '" LOGURU_FMT(s) "' due to file changed", file_abs->path);
			}
			// try reopen current file.
			if (!create_directories(file_abs->path)) {
				LOG_F(ERROR, "Failed to create directories to '" LOGURU_FMT(s) "'", file_abs->path);
			}
			file_abs->fp = fopen(file_abs->path, file_abs->mode_str);
			if (!file_abs->fp) {
				LOG_F(ERROR, "Failed to open '" LOGURU_FMT(s) "'", file_abs->path);
			} else {
				stat(file_abs->path, &file_abs->st);
			}
			file_abs->is_reopening = false;
		}
	}
#endif
	// ------------------------------------------------------------------------------

	// Helpers:

	Text::~Text() { free(_str); }

#if LOGURU_USE_FMTLIB
	Text vtextprintf(const char* format, fmt::format_args args)
	{
		return Text(STRDUP(fmt::vformat(format, args).c_str()));
	}
#else
	LOGURU_PRINTF_LIKE(1, 0)
	static Text vtextprintf(const char* format, va_list vlist)
	{
#ifdef _WIN32
		int bytes_needed = _vscprintf(format, vlist);
		CHECK_F(bytes_needed >= 0, "Bad string format: '%s'", format);
		char* buff = (char*)malloc(bytes_needed+1);
		vsnprintf(buff, bytes_needed+1, format, vlist);
		return Text(buff);
#else
		char* buff = nullptr;
		int result = vasprintf(&buff, format, vlist);
		CHECK_F(result >= 0, "Bad string format: '" LOGURU_FMT(s) "'", format);
		return Text(buff);
#endif
	}

	Text textprintf(const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto result = vtextprintf(format, vlist);
		va_end(vlist);
		return result;
	}
#endif

	// Overloaded for variadic template matching.
	Text textprintf()
	{
		return Text(static_cast<char*>(calloc(1, 1)));
	}

	static const char* indentation(unsigned depth)
	{
		static const char buff[] =
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   "
		".   .   .   .   .   .   .   .   .   .   " ".   .   .   .   .   .   .   .   .   .   ";
		static const size_t INDENTATION_WIDTH = 4;
		static const size_t NUM_INDENTATIONS = (sizeof(buff) - 1) / INDENTATION_WIDTH;
		depth = std::min<unsigned>(depth, NUM_INDENTATIONS);
		return buff + INDENTATION_WIDTH * (NUM_INDENTATIONS - depth);
	}

	static void parse_args(int& argc, char* argv[], const char* verbosity_flag)
	{
		int arg_dest = 1;
		int out_argc = argc;

		for (int arg_it = 1; arg_it < argc; ++arg_it) {
			auto cmd = argv[arg_it];
			auto arg_len = strlen(verbosity_flag);
			if (strncmp(cmd, verbosity_flag, arg_len) == 0 && !std::isalpha(cmd[arg_len], std::locale(""))) {
				out_argc -= 1;
				auto value_str = cmd + arg_len;
				if (value_str[0] == '\0') {
					// Value in separate argument
					arg_it += 1;
					CHECK_LT_F(arg_it, argc, "Missing verbosiy level after " LOGURU_FMT(s) "", verbosity_flag);
					value_str = argv[arg_it];
					out_argc -= 1;
				}
				if (*value_str == '=') { value_str += 1; }

				auto req_verbosity = get_verbosity_from_name(value_str);
				if (req_verbosity != Verbosity_INVALID) {
					g_stderr_verbosity = req_verbosity;
				} else {
					char* end = 0;
					g_stderr_verbosity = static_cast<int>(strtol(value_str, &end, 10));
					CHECK_F(end && *end == '\0',
						"Invalid verbosity. Expected integer, INFO, WARNING, ERROR or OFF, got '" LOGURU_FMT(s) "'", value_str);
				}
			} else {
				argv[arg_dest++] = argv[arg_it];
			}
		}

		argc = out_argc;
		argv[argc] = nullptr;
	}

	static long long now_ns()
	{
		return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	// Returns the part of the path after the last / or \ (if any).
	const char* filename(const char* path)
	{
		for (auto ptr = path; *ptr; ++ptr) {
			if (*ptr == '/' || *ptr == '\\') {
				path = ptr + 1;
			}
		}
		return path;
	}

	// ------------------------------------------------------------------------------

	static void on_atexit()
	{
		VLOG_F(g_internal_verbosity, "atexit");
		flush();
	}

	static void install_signal_handlers(bool unsafe_signal_handler);

	static void write_hex_digit(std::string& out, unsigned num)
	{
		DCHECK_LT_F(num, 16u);
		if (num < 10u) { out.push_back(char('0' + num)); }
		else { out.push_back(char('A' + num - 10)); }
	}

	static void write_hex_byte(std::string& out, uint8_t n)
	{
		write_hex_digit(out, n >> 4u);
		write_hex_digit(out, n & 0x0f);
	}

	static void escape(std::string& out, const std::string& str)
	{
		for (char c : str) {
			/**/ if (c == '\a') { out += "\\a";  }
			else if (c == '\b') { out += "\\b";  }
			else if (c == '\f') { out += "\\f";  }
			else if (c == '\n') { out += "\\n";  }
			else if (c == '\r') { out += "\\r";  }
			else if (c == '\t') { out += "\\t";  }
			else if (c == '\v') { out += "\\v";  }
			else if (c == '\\') { out += "\\\\"; }
			else if (c == '\'') { out += "\\\'"; }
			else if (c == '\"') { out += "\\\""; }
			else if (c == ' ')  { out += "\\ ";  }
			else if (0 <= c && c < 0x20) { // ASCI control character:
			// else if (c < 0x20 || c != (c & 127)) { // ASCII control character or UTF-8:
				out += "\\x";
				write_hex_byte(out, static_cast<uint8_t>(c));
			} else { out += c; }
		}
	}

	Text errno_as_text()
	{
		char buff[256];
	#if defined(__GLIBC__) && defined(_GNU_SOURCE)
		// GNU Version
		return Text(STRDUP(strerror_r(errno, buff, sizeof(buff))));
	#elif defined(__APPLE__) || _POSIX_C_SOURCE >= 200112L
		// XSI Version
		strerror_r(errno, buff, sizeof(buff));
		return Text(strdup(buff));
	#elif defined(_WIN32)
		strerror_s(buff, sizeof(buff), errno);
		return Text(STRDUP(buff));
	#else
		// Not thread-safe.
		return Text(STRDUP(strerror(errno)));
	#endif
	}

	void init(int& argc, char* argv[], const Options& options)
	{
		CHECK_GT_F(argc,       0,       "Expected proper argc/argv");
		CHECK_EQ_F(argv[argc], nullptr, "Expected proper argc/argv");

		s_argv0_filename = filename(argv[0]);

		#ifdef _WIN32
			#define getcwd _getcwd
		#endif

		if (!getcwd(s_current_dir, sizeof(s_current_dir))) {
			const auto error_text = errno_as_text();
			LOG_F(WARNING, "Failed to get current working directory: " LOGURU_FMT(s) "", error_text.c_str());
		}

		s_arguments = "";
		for (int i = 0; i < argc; ++i) {
			escape(s_arguments, argv[i]);
			if (i + 1 < argc) {
				s_arguments += " ";
			}
		}

		if (options.verbosity_flag) {
			parse_args(argc, argv, options.verbosity_flag);
		}

		if (const auto main_thread_name = options.main_thread_name) {
			#if LOGURU_PTLS_NAMES || LOGURU_WINTHREADS
				set_thread_name(main_thread_name);
			#elif LOGURU_PTHREADS
				char old_thread_name[16] = {0};
				auto this_thread = pthread_self();
				#if defined(__APPLE__) || defined(__linux__)
					pthread_getname_np(this_thread, old_thread_name, sizeof(old_thread_name));
				#endif
				if (old_thread_name[0] == 0) {
					#ifdef __APPLE__
						pthread_setname_np(main_thread_name);
					#elif defined(__FreeBSD__) || defined(__OpenBSD__)
						pthread_set_name_np(this_thread, main_thread_name);
					#elif defined(__linux__)
						pthread_setname_np(this_thread, main_thread_name);
					#endif
				}
			#endif // LOGURU_PTHREADS
		}

		if (g_stderr_verbosity >= Verbosity_INFO) {
			if (g_preamble) {
				char preamble_explain[LOGURU_PREAMBLE_WIDTH];
				print_preamble_header(preamble_explain, sizeof(preamble_explain));
				if (g_colorlogtostderr && s_terminal_has_color) {
					fprintf(stderr, "%s%s%s\n", terminal_reset(), terminal_dim(), preamble_explain);
				} else {
					fprintf(stderr, "%s\n", preamble_explain);
				}
			}
			fflush(stderr);
		}
		VLOG_F(g_internal_verbosity, "arguments: " LOGURU_FMT(s) "", s_arguments.c_str());
		if (strlen(s_current_dir) != 0)
		{
			VLOG_F(g_internal_verbosity, "Current dir: " LOGURU_FMT(s) "", s_current_dir);
		}
		VLOG_F(g_internal_verbosity, "stderr verbosity: " LOGURU_FMT(d) "", g_stderr_verbosity);
		VLOG_F(g_internal_verbosity, "-----------------------------------");

		install_signal_handlers(options.unsafe_signal_handler);

		atexit(on_atexit);
	}

	void shutdown()
	{
		VLOG_F(g_internal_verbosity, "loguru::shutdown()");
		remove_all_callbacks();
		set_fatal_handler(nullptr);
		set_verbosity_to_name_callback(nullptr);
		set_name_to_verbosity_callback(nullptr);
	}

	void write_date_time(char* buff, size_t buff_size)
	{
		auto now = system_clock::now();
		long long ms_since_epoch = duration_cast<milliseconds>(now.time_since_epoch()).count();
		time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
		tm time_info;
		localtime_r(&sec_since_epoch, &time_info);
		snprintf(buff, buff_size, "%04d%02d%02d_%02d%02d%02d.%03lld",
			1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
			time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000);
	}

	const char* argv0_filename()
	{
		return s_argv0_filename.c_str();
	}

	const char* arguments()
	{
		return s_arguments.c_str();
	}

	const char* current_dir()
	{
		return s_current_dir;
	}

	const char* home_dir()
	{
		#ifdef _WIN32
			auto user_profile = getenv("USERPROFILE");
			CHECK_F(user_profile != nullptr, "Missing USERPROFILE");
			return user_profile;
		#else // _WIN32
			auto home = getenv("HOME");
			CHECK_F(home != nullptr, "Missing HOME");
			return home;
		#endif // _WIN32
	}

	void suggest_log_path(const char* prefix, char* buff, unsigned buff_size)
	{
		if (prefix[0] == '~') {
			snprintf(buff, buff_size - 1, "%s%s", home_dir(), prefix + 1);
		} else {
			snprintf(buff, buff_size - 1, "%s", prefix);
		}

		// Check for terminating /
		size_t n = strlen(buff);
		if (n != 0) {
			if (buff[n - 1] != '/') {
				CHECK_F(n + 2 < buff_size, "Filename buffer too small");
				buff[n] = '/';
				buff[n + 1] = '\0';
			}
		}

		strncat(buff, s_argv0_filename.c_str(), buff_size - strlen(buff) - 1);
		strncat(buff, "/",                      buff_size - strlen(buff) - 1);
		write_date_time(buff + strlen(buff),    buff_size - strlen(buff));
		strncat(buff, ".log",                   buff_size - strlen(buff) - 1);
	}

	bool create_directories(const char* file_path_const)
	{
		CHECK_F(file_path_const && *file_path_const);
		char* file_path = STRDUP(file_path_const);
		for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
			*p = '\0';

	#ifdef _WIN32
			if (_mkdir(file_path) == -1) {
	#else
			if (mkdir(file_path, 0755) == -1) {
	#endif
				if (errno != EEXIST) {
					LOG_F(ERROR, "Failed to create directory '" LOGURU_FMT(s) "'", file_path);
					LOG_IF_F(ERROR, errno == EACCES,       "EACCES");
					LOG_IF_F(ERROR, errno == ENAMETOOLONG, "ENAMETOOLONG");
					LOG_IF_F(ERROR, errno == ENOENT,       "ENOENT");
					LOG_IF_F(ERROR, errno == ENOTDIR,      "ENOTDIR");
					LOG_IF_F(ERROR, errno == ELOOP,        "ELOOP");

					*p = '/';
					free(file_path);
					return false;
				}
			}
			*p = '/';
		}
		free(file_path);
		return true;
	}
	bool add_file(const char* path_in, FileMode mode, Verbosity verbosity)
	{
		char path[PATH_MAX];
		if (path_in[0] == '~') {
			snprintf(path, sizeof(path) - 1, "%s%s", home_dir(), path_in + 1);
		} else {
			snprintf(path, sizeof(path) - 1, "%s", path_in);
		}

		if (!create_directories(path)) {
			LOG_F(ERROR, "Failed to create directories to '" LOGURU_FMT(s) "'", path);
		}

		const char* mode_str = (mode == FileMode::Truncate ? "w" : "a");
		auto file = fopen(path, mode_str);
		if (!file) {
			LOG_F(ERROR, "Failed to open '" LOGURU_FMT(s) "'", path);
			return false;
		}
#if LOGURU_WITH_FILEABS
		FileAbs* file_abs = new FileAbs(); // this is deleted in file_close;
		snprintf(file_abs->path, sizeof(file_abs->path) - 1, "%s", path);
		snprintf(file_abs->mode_str, sizeof(file_abs->mode_str) - 1, "%s", mode_str);
		stat(file_abs->path, &file_abs->st);
		file_abs->fp = file;
		file_abs->verbosity = verbosity;
		add_callback(path_in, file_log, file_abs, verbosity, file_close, file_flush);
#else
		add_callback(path_in, file_log, file, verbosity, file_close, file_flush);
#endif

		if (mode == FileMode::Append) {
			fprintf(file, "\n\n\n\n\n");
		}
		if (!s_arguments.empty()) {
			fprintf(file, "arguments: %s\n", s_arguments.c_str());
		}
		if (strlen(s_current_dir) != 0) {
			fprintf(file, "Current dir: %s\n", s_current_dir);
		}
		fprintf(file, "File verbosity level: %d\n", verbosity);
		if (g_preamble) {
			char preamble_explain[LOGURU_PREAMBLE_WIDTH];
			print_preamble_header(preamble_explain, sizeof(preamble_explain));
			fprintf(file, "%s\n", preamble_explain);
		}
		fflush(file);

		VLOG_F(g_internal_verbosity, "Logging to '" LOGURU_FMT(s) "', mode: '" LOGURU_FMT(s) "', verbosity: " LOGURU_FMT(d) "", path, mode_str, verbosity);
		return true;
	}

	// Will be called right before abort().
	void set_fatal_handler(fatal_handler_t handler)
	{
		s_fatal_handler = handler;
	}

	fatal_handler_t get_fatal_handler()
	{
		return s_fatal_handler;
	}

	void set_verbosity_to_name_callback(verbosity_to_name_t callback)
	{
		s_verbosity_to_name_callback = callback;
	}

	void set_name_to_verbosity_callback(name_to_verbosity_t callback)
	{
		s_name_to_verbosity_callback = callback;
	}

	void add_stack_cleanup(const char* find_this, const char* replace_with_this)
	{
		if (strlen(find_this) <= strlen(replace_with_this)) {
			LOG_F(WARNING, "add_stack_cleanup: the replacement should be shorter than the pattern!");
			return;
		}

		s_user_stack_cleanups.push_back(StringPair(find_this, replace_with_this));
	}

	static void on_callback_change()
	{
		s_max_out_verbosity = Verbosity_OFF;
		for (const auto& callback : s_callbacks) {
			s_max_out_verbosity = std::max(s_max_out_verbosity, callback.verbosity);
		}
	}

	void add_callback(
		const char*     id,
		log_handler_t   callback,
		void*           user_data,
		Verbosity       verbosity,
		close_handler_t on_close,
		flush_handler_t on_flush)
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		s_callbacks.push_back(Callback{id, callback, user_data, verbosity, on_close, on_flush, 0});
		on_callback_change();
	}

	// Returns a custom verbosity name if one is available, or nullptr.
	// See also set_verbosity_to_name_callback.
	const char* get_verbosity_name(Verbosity verbosity)
	{
		auto name = s_verbosity_to_name_callback
				? (*s_verbosity_to_name_callback)(verbosity)
				: nullptr;

		// Use standard replacements if callback fails:
		if (!name)
		{
			if (verbosity <= Verbosity_FATAL) {
				name = "FATL";
			} else if (verbosity == Verbosity_ERROR) {
				name = "ERR";
			} else if (verbosity == Verbosity_WARNING) {
				name = "WARN";
			} else if (verbosity == Verbosity_INFO) {
				name = "INFO";
			}
		}

		return name;
	}

	// Returns Verbosity_INVALID if the name is not found.
	// See also set_name_to_verbosity_callback.
	Verbosity get_verbosity_from_name(const char* name)
	{
		auto verbosity = s_name_to_verbosity_callback
				? (*s_name_to_verbosity_callback)(name)
				: Verbosity_INVALID;

		// Use standard replacements if callback fails:
		if (verbosity == Verbosity_INVALID) {
			if (strcmp(name, "OFF") == 0) {
				verbosity = Verbosity_OFF;
			} else if (strcmp(name, "INFO") == 0) {
				verbosity = Verbosity_INFO;
			} else if (strcmp(name, "WARNING") == 0) {
				verbosity = Verbosity_WARNING;
			} else if (strcmp(name, "ERROR") == 0) {
				verbosity = Verbosity_ERROR;
			} else if (strcmp(name, "FATAL") == 0) {
				verbosity = Verbosity_FATAL;
			}
		}

		return verbosity;
	}

	bool remove_callback(const char* id)
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		auto it = std::find_if(begin(s_callbacks), end(s_callbacks), [&](const Callback& c) { return c.id == id; });
		if (it != s_callbacks.end()) {
			if (it->close) { it->close(it->user_data); }
			s_callbacks.erase(it);
			on_callback_change();
			return true;
		} else {
			LOG_F(ERROR, "Failed to locate callback with id '" LOGURU_FMT(s) "'", id);
			return false;
		}
	}

	void remove_all_callbacks()
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		for (auto& callback : s_callbacks) {
			if (callback.close) {
				callback.close(callback.user_data);
			}
		}
		s_callbacks.clear();
		on_callback_change();
	}

	// Returns the maximum of g_stderr_verbosity and all file/custom outputs.
	Verbosity current_verbosity_cutoff()
	{
		return g_stderr_verbosity > s_max_out_verbosity ?
			   g_stderr_verbosity : s_max_out_verbosity;
	}

#if LOGURU_WINTHREADS
	char* get_thread_name_win32()
	{
		__declspec( thread ) static char thread_name[LOGURU_THREADNAME_WIDTH + 1] = {0};
		return &thread_name[0];
	}
#endif // LOGURU_WINTHREADS

	void set_thread_name(const char* name)
	{
		#if LOGURU_PTLS_NAMES
			(void)pthread_once(&s_pthread_key_once, make_pthread_key_name);
			(void)pthread_setspecific(s_pthread_key_name, STRDUP(name));

		#elif LOGURU_PTHREADS
			#ifdef __APPLE__
				pthread_setname_np(name);
			#elif defined(__FreeBSD__) || defined(__OpenBSD__)
				pthread_set_name_np(pthread_self(), name);
			#elif defined(__linux__)
				pthread_setname_np(pthread_self(), name);
			#endif
		#elif LOGURU_WINTHREADS
			strncpy_s(get_thread_name_win32(), LOGURU_THREADNAME_WIDTH + 1, name, _TRUNCATE);
		#else // LOGURU_PTHREADS
			(void)name;
		#endif // LOGURU_PTHREADS
	}

#if LOGURU_PTLS_NAMES
	const char* get_thread_name_ptls()
	{
		(void)pthread_once(&s_pthread_key_once, make_pthread_key_name);
		return static_cast<const char*>(pthread_getspecific(s_pthread_key_name));
	}
#endif // LOGURU_PTLS_NAMES

	void get_thread_name(char* buffer, unsigned long long length, bool right_align_hext_id)
	{
#ifdef _WIN32
		(void)right_align_hext_id;
#endif
		CHECK_NE_F(length, 0u, "Zero length buffer in get_thread_name");
		CHECK_NOTNULL_F(buffer, "nullptr in get_thread_name");
#if LOGURU_PTHREADS
		auto thread = pthread_self();
		#if LOGURU_PTLS_NAMES
			if (const char* name = get_thread_name_ptls()) {
				snprintf(buffer, length, "%s", name);
			} else {
				buffer[0] = 0;
			}
		#elif defined(__APPLE__) || defined(__linux__)
			pthread_getname_np(thread, buffer, length);
		#else
			buffer[0] = 0;
		#endif

		if (buffer[0] == 0) {
			#ifdef __APPLE__
				uint64_t thread_id;
				pthread_threadid_np(thread, &thread_id);
			#elif defined(__FreeBSD__)
				long thread_id;
				(void)thr_self(&thread_id);
			#elif defined(__OpenBSD__)
				unsigned thread_id = -1;
			#else
				uint64_t thread_id = thread;
			#endif
			if (right_align_hext_id) {
				snprintf(buffer, length, "%*X", static_cast<int>(length - 1), static_cast<unsigned>(thread_id));
			} else {
				snprintf(buffer, length, "%X", static_cast<unsigned>(thread_id));
			}
		}
#elif LOGURU_WINTHREADS
		if (const char* name = get_thread_name_win32()) {
			snprintf(buffer, (size_t)length, "%s", name);
		} else {
			buffer[0] = 0;
		}
#else // !LOGURU_WINTHREADS && !LOGURU_WINTHREADS
		buffer[0] = 0;
#endif

	}

	// ------------------------------------------------------------------------
	// Stack traces

#if LOGURU_STACKTRACES
	Text demangle(const char* name)
	{
		int status = -1;
		char* demangled = abi::__cxa_demangle(name, 0, 0, &status);
		Text result{status == 0 ? demangled : STRDUP(name)};
		return result;
	}

	#if LOGURU_RTTI
		template <class T>
		std::string type_name()
		{
			auto demangled = demangle(typeid(T).name());
			return demangled.c_str();
		}
	#endif // LOGURU_RTTI

	static const StringPairList REPLACE_LIST = {
		#if LOGURU_RTTI
			{ type_name<std::string>(),    "std::string"    },
			{ type_name<std::wstring>(),   "std::wstring"   },
			{ type_name<std::u16string>(), "std::u16string" },
			{ type_name<std::u32string>(), "std::u32string" },
		#endif // LOGURU_RTTI
		{ "std::__1::",                "std::"          },
		{ "__thiscall ",               ""               },
		{ "__cdecl ",                  ""               },
	};

	void do_replacements(const StringPairList& replacements, std::string& str)
	{
		for (auto&& p : replacements) {
			if (p.first.size() <= p.second.size()) {
				// On gcc, "type_name<std::string>()" is "std::string"
				continue;
			}

			size_t it;
			while ((it=str.find(p.first)) != std::string::npos) {
				str.replace(it, p.first.size(), p.second);
			}
		}
	}

	std::string prettify_stacktrace(const std::string& input)
	{
		std::string output = input;

		do_replacements(s_user_stack_cleanups, output);
		do_replacements(REPLACE_LIST, output);

		try {
			std::regex std_allocator_re(R"(,\s*std::allocator<[^<>]+>)");
			output = std::regex_replace(output, std_allocator_re, std::string(""));

			std::regex template_spaces_re(R"(<\s*([^<> ]+)\s*>)");
			output = std::regex_replace(output, template_spaces_re, std::string("<$1>"));
		} catch (std::regex_error&) {
			// Probably old GCC.
		}

		return output;
	}

	std::string stacktrace_as_stdstring(int skip)
	{
		// From https://gist.github.com/fmela/591333
		void* callstack[128];
		const auto max_frames = sizeof(callstack) / sizeof(callstack[0]);
		int num_frames = backtrace(callstack, max_frames);
		char** symbols = backtrace_symbols(callstack, num_frames);

		std::string result;
		// Print stack traces so the most relevant ones are written last
		// Rationale: http://yellerapp.com/posts/2015-01-22-upside-down-stacktraces.html
		for (int i = num_frames - 1; i >= skip; --i) {
			char buf[1024];
			Dl_info info;
			if (dladdr(callstack[i], &info) && info.dli_sname) {
				char* demangled = NULL;
				int status = -1;
				if (info.dli_sname[0] == '_') {
					demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
				}
				snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
						 i - skip, int(2 + sizeof(void*) * 2), callstack[i],
						 status == 0 ? demangled :
						 info.dli_sname == 0 ? symbols[i] : info.dli_sname,
						 static_cast<char*>(callstack[i]) - static_cast<char*>(info.dli_saddr));
				free(demangled);
			} else {
				snprintf(buf, sizeof(buf), "%-3d %*p %s\n",
						 i - skip, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
			}
			result += buf;
		}
		free(symbols);

		if (num_frames == max_frames) {
			result = "[truncated]\n" + result;
		}

		if (!result.empty() && result[result.size() - 1] == '\n') {
			result.resize(result.size() - 1);
		}

		return prettify_stacktrace(result);
	}

#else // LOGURU_STACKTRACES
	Text demangle(const char* name)
	{
		return Text(STRDUP(name));
	}

	std::string stacktrace_as_stdstring(int)
	{
		// No stacktraces available on this platform"
		return "";
	}

#endif // LOGURU_STACKTRACES

	Text stacktrace(int skip)
	{
		auto str = stacktrace_as_stdstring(skip + 1);
		return Text(STRDUP(str.c_str()));
	}

	// ------------------------------------------------------------------------

	static void print_preamble_header(char* out_buff, size_t out_buff_size)
	{
		if (out_buff_size == 0) { return; }
		out_buff[0] = '\0';
		long pos = 0;
		if (g_preamble_date && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "date       ");
		}
		if (g_preamble_time && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "time         ");
		}
		if (g_preamble_uptime && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "( uptime  ) ");
		}
		if (g_preamble_thread && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "[%-*s]", LOGURU_THREADNAME_WIDTH, " thread name/id");
		}
		if (g_preamble_file && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "%*s:line  ", LOGURU_FILENAME_WIDTH, "file");
		}
		if (g_preamble_verbose && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "   v");
		}
		if (g_preamble_pipe && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "| ");
		}
	}

	static void print_preamble(char* out_buff, size_t out_buff_size, Verbosity verbosity, const char* file, unsigned line)
	{
		if (out_buff_size == 0) { return; }
		out_buff[0] = '\0';
		if (!g_preamble) { return; }
		long long ms_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
		tm time_info;
		localtime_r(&sec_since_epoch, &time_info);

		auto uptime_ms = duration_cast<milliseconds>(steady_clock::now() - s_start_time).count();
		auto uptime_sec = uptime_ms / 1000.0;

		char thread_name[LOGURU_THREADNAME_WIDTH + 1] = {0};
		get_thread_name(thread_name, LOGURU_THREADNAME_WIDTH + 1, true);

		if (s_strip_file_path) {
			file = filename(file);
		}

		char level_buff[6];
		const char* custom_level_name = get_verbosity_name(verbosity);
		if (custom_level_name) {
			snprintf(level_buff, sizeof(level_buff) - 1, "%s", custom_level_name);
		} else {
			snprintf(level_buff, sizeof(level_buff) - 1, "% 4d", verbosity);
		}

		long pos = 0;

		if (g_preamble_date && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "%04d-%02d-%02d ",
				             1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday);
		}
		if (g_preamble_time && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "%02d:%02d:%02d.%03lld ",
			               time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000);
		}
		if (g_preamble_uptime && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "(%8.3fs) ",
			               uptime_sec);
		}
		if (g_preamble_thread && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "[%-*s]",
			               LOGURU_THREADNAME_WIDTH, thread_name);
		}
		if (g_preamble_file && pos < out_buff_size) {
			char shortened_filename[LOGURU_FILENAME_WIDTH + 1];
			snprintf(shortened_filename, LOGURU_FILENAME_WIDTH + 1, "%s", file);
			pos += snprintf(out_buff + pos, out_buff_size - pos, "%*s:%-5u ",
			               LOGURU_FILENAME_WIDTH, shortened_filename, line);
		}
		if (g_preamble_verbose && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "%4s",
			               level_buff);
		}
		if (g_preamble_pipe && pos < out_buff_size) {
			pos += snprintf(out_buff + pos, out_buff_size - pos, "| ");
		}
	}

	// stack_trace_skip is just if verbosity == FATAL.
	static void log_message(int stack_trace_skip, Message& message, bool with_indentation, bool abort_if_fatal)
	{
		const auto verbosity = message.verbosity;
		std::lock_guard<std::recursive_mutex> lock(s_mutex);

		if (message.verbosity == Verbosity_FATAL) {
			auto st = loguru::stacktrace(stack_trace_skip + 2);
			if (!st.empty()) {
				RAW_LOG_F(ERROR, "Stack trace:\n" LOGURU_FMT(s) "", st.c_str());
			}

			auto ec = loguru::get_error_context();
			if (!ec.empty()) {
				RAW_LOG_F(ERROR, "" LOGURU_FMT(s) "", ec.c_str());
			}
		}

		if (with_indentation) {
			message.indentation = indentation(s_stderr_indentation);
		}

		if (verbosity <= g_stderr_verbosity) {
			if (g_colorlogtostderr && s_terminal_has_color) {
				if (verbosity > Verbosity_WARNING) {
					fprintf(stderr, "%s%s%s%s%s%s%s%s\n",
						terminal_reset(),
						terminal_dim(),
						message.preamble,
						message.indentation,
						verbosity == Verbosity_INFO ? terminal_reset() : "", // un-dim for info
						message.prefix,
						message.message,
						terminal_reset());
				} else {
					fprintf(stderr, "%s%s%s%s%s%s%s\n",
						terminal_reset(),
						verbosity == Verbosity_WARNING ? terminal_yellow() : terminal_red(),
						message.preamble,
						message.indentation,
						message.prefix,
						message.message,
						terminal_reset());
				}
			} else {
				fprintf(stderr, "%s%s%s%s\n",
					message.preamble, message.indentation, message.prefix, message.message);
			}

			if (g_flush_interval_ms == 0) {
				fflush(stderr);
			} else {
				s_needs_flushing = true;
			}
		}

		for (auto& p : s_callbacks) {
			if (verbosity <= p.verbosity) {
				if (with_indentation) {
					message.indentation = indentation(p.indentation);
				}
				p.callback(p.user_data, message);
				if (g_flush_interval_ms == 0) {
					if (p.flush) { p.flush(p.user_data); }
				} else {
					s_needs_flushing = true;
				}
			}
		}

		if (g_flush_interval_ms > 0 && !s_flush_thread) {
			s_flush_thread = new std::thread([](){
				for (;;) {
					if (s_needs_flushing) {
						flush();
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(g_flush_interval_ms));
				}
			});
		}

		if (message.verbosity == Verbosity_FATAL) {
			flush();

			if (s_fatal_handler) {
				s_fatal_handler(message);
				flush();
			}

			if (abort_if_fatal) {
#if LOGURU_CATCH_SIGABRT && !defined(_WIN32)
				// Make sure we don't catch our own abort:
				signal(SIGABRT, SIG_DFL);
#endif
				abort();
			}
		}
	}

	// stack_trace_skip is just if verbosity == FATAL.
	void log_to_everywhere(int stack_trace_skip, Verbosity verbosity,
	                       const char* file, unsigned line,
	                       const char* prefix, const char* buff)
	{
		char preamble_buff[LOGURU_PREAMBLE_WIDTH];
		print_preamble(preamble_buff, sizeof(preamble_buff), verbosity, file, line);
		auto message = Message{verbosity, file, line, preamble_buff, "", prefix, buff};
		log_message(stack_trace_skip + 1, message, true, true);
	}

#if LOGURU_USE_FMTLIB
	void vlog(Verbosity verbosity, const char* file, unsigned line, const char* format, fmt::format_args args)
	{
		auto formatted = fmt::vformat(format, args);
		log_to_everywhere(1, verbosity, file, line, "", formatted.c_str());
	}

	void raw_vlog(Verbosity verbosity, const char* file, unsigned line, const char* format, fmt::format_args args)
	{
		auto formatted = fmt::vformat(format, args);
		auto message = Message{verbosity, file, line, "", "", "", formatted.c_str()};
		log_message(1, message, false, true);
	}
#else
	void log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = vtextprintf(format, vlist);
		log_to_everywhere(1, verbosity, file, line, "", buff.c_str());
		va_end(vlist);
	}

	void raw_log(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = vtextprintf(format, vlist);
		auto message = Message{verbosity, file, line, "", "", "", buff.c_str()};
		log_message(1, message, false, true);
		va_end(vlist);
	}
#endif

	void flush()
	{
		std::lock_guard<std::recursive_mutex> lock(s_mutex);
		fflush(stderr);
		for (const auto& callback : s_callbacks)
		{
			if (callback.flush) {
				callback.flush(callback.user_data);
			}
		}
		s_needs_flushing = false;
	}

	LogScopeRAII::LogScopeRAII(Verbosity verbosity, const char* file, unsigned line, const char* format, ...)
		: _verbosity(verbosity), _file(file), _line(line)
	{
		if (verbosity <= current_verbosity_cutoff()) {
			std::lock_guard<std::recursive_mutex> lock(s_mutex);
			_indent_stderr = (verbosity <= g_stderr_verbosity);
			_start_time_ns = now_ns();
			va_list vlist;
			va_start(vlist, format);
			vsnprintf(_name, sizeof(_name), format, vlist);
			log_to_everywhere(1, _verbosity, file, line, "{ ", _name);
			va_end(vlist);

			if (_indent_stderr) {
				++s_stderr_indentation;
			}

			for (auto& p : s_callbacks) {
				if (verbosity <= p.verbosity) {
					++p.indentation;
				}
			}
		} else {
			_file = nullptr;
		}
	}

	LogScopeRAII::~LogScopeRAII()
	{
		if (_file) {
			std::lock_guard<std::recursive_mutex> lock(s_mutex);
			if (_indent_stderr && s_stderr_indentation > 0) {
				--s_stderr_indentation;
			}
			for (auto& p : s_callbacks) {
				// Note: Callback indentation cannot change!
				if (_verbosity <= p.verbosity) {
					// in unlikely case this callback is new
					if (p.indentation > 0) {
						--p.indentation;
					}
				}
			}
#if LOGURU_VERBOSE_SCOPE_ENDINGS
			auto duration_sec = (now_ns() - _start_time_ns) / 1e9;
#if LOGURU_USE_FMTLIB
			auto buff = textprintf("{:.{}f} s: {:s}", duration_sec, LOGURU_SCOPE_TIME_PRECISION, _name);
#else
			auto buff = textprintf("%.*f s: %s", LOGURU_SCOPE_TIME_PRECISION, duration_sec, _name);
#endif
			log_to_everywhere(1, _verbosity, _file, _line, "} ", buff.c_str());
#else
			log_to_everywhere(1, _verbosity, _file, _line, "}", "");
#endif
		}
	}

#if LOGURU_USE_FMTLIB
	void vlog_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, const char* format, fmt::format_args args)
	{
		auto formatted = fmt::vformat(format, args);
		log_to_everywhere(stack_trace_skip + 1, Verbosity_FATAL, file, line, expr, formatted.c_str());
		abort(); // log_to_everywhere already does this, but this makes the analyzer happy.
	}
#else
	void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line, const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = vtextprintf(format, vlist);
		log_to_everywhere(stack_trace_skip + 1, Verbosity_FATAL, file, line, expr, buff.c_str());
		va_end(vlist);
		abort(); // log_to_everywhere already does this, but this makes the analyzer happy.
	}
#endif

	void log_and_abort(int stack_trace_skip, const char* expr, const char* file, unsigned line)
	{
		log_and_abort(stack_trace_skip + 1, expr, file, line, " ");
	}

	// ----------------------------------------------------------------------------
	// Streams:

#if LOGURU_USE_FMTLIB
	template<typename... Args>
	std::string vstrprintf(const char* format, const Args&... args)
	{
		auto text = textprintf(format, args...);
		std::string result = text.c_str();
		return result;
	}

	template<typename... Args>
	std::string strprintf(const char* format, const Args&... args)
	{
		return vstrprintf(format, args...);
	}
#else
	std::string vstrprintf(const char* format, va_list vlist)
	{
		auto text = vtextprintf(format, vlist);
		std::string result = text.c_str();
		return result;
	}

	std::string strprintf(const char* format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto result = vstrprintf(format, vlist);
		va_end(vlist);
		return result;
	}
#endif

	#if LOGURU_WITH_STREAMS

	StreamLogger::~StreamLogger() noexcept(false)
	{
		auto message = _ss.str();
		log(_verbosity, _file, _line, LOGURU_FMT(s), message.c_str());
	}

	AbortLogger::~AbortLogger() noexcept(false)
	{
		auto message = _ss.str();
		loguru::log_and_abort(1, _expr, _file, _line, LOGURU_FMT(s), message.c_str());
	}

	#endif // LOGURU_WITH_STREAMS

	// ----------------------------------------------------------------------------
	// 888888 88""Yb 88""Yb  dP"Yb  88""Yb      dP""b8  dP"Yb  88b 88 888888 888888 Yb  dP 888888
	// 88__   88__dP 88__dP dP   Yb 88__dP     dP   `" dP   Yb 88Yb88   88   88__    YbdP    88
	// 88""   88"Yb  88"Yb  Yb   dP 88"Yb      Yb      Yb   dP 88 Y88   88   88""    dPYb    88
	// 888888 88  Yb 88  Yb  YbodP  88  Yb      YboodP  YbodP  88  Y8   88   888888 dP  Yb   88
	// ----------------------------------------------------------------------------

	struct StringStream
	{
		std::string str;
	};

	// Use this in your EcPrinter implementations.
	void stream_print(StringStream& out_string_stream, const char* text)
	{
		out_string_stream.str += text;
	}

	// ----------------------------------------------------------------------------

	using ECPtr = EcEntryBase*;

#if defined(_WIN32) || (defined(__APPLE__) && !TARGET_OS_IPHONE)
	#ifdef __APPLE__
		#define LOGURU_THREAD_LOCAL __thread
	#else
		#define LOGURU_THREAD_LOCAL thread_local
	#endif
	static LOGURU_THREAD_LOCAL ECPtr thread_ec_ptr = nullptr;

	ECPtr& get_thread_ec_head_ref()
	{
		return thread_ec_ptr;
	}
#else // !thread_local
	static pthread_once_t s_ec_pthread_once = PTHREAD_ONCE_INIT;
	static pthread_key_t  s_ec_pthread_key;

	void free_ec_head_ref(void* io_error_context)
	{
		delete reinterpret_cast<ECPtr*>(io_error_context);
	}

	void ec_make_pthread_key()
	{
		(void)pthread_key_create(&s_ec_pthread_key, free_ec_head_ref);
	}

	ECPtr& get_thread_ec_head_ref()
	{
		(void)pthread_once(&s_ec_pthread_once, ec_make_pthread_key);
		auto ec = reinterpret_cast<ECPtr*>(pthread_getspecific(s_ec_pthread_key));
		if (ec == nullptr) {
			ec = new ECPtr(nullptr);
			(void)pthread_setspecific(s_ec_pthread_key, ec);
		}
		return *ec;
	}
#endif // !thread_local

	// ----------------------------------------------------------------------------

	EcHandle get_thread_ec_handle()
	{
		return get_thread_ec_head_ref();
	}

	Text get_error_context()
	{
		return get_error_context_for(get_thread_ec_head_ref());
	}

	Text get_error_context_for(const EcEntryBase* ec_head)
	{
		std::vector<const EcEntryBase*> stack;
		while (ec_head) {
			stack.push_back(ec_head);
			ec_head = ec_head->_previous;
		}
		std::reverse(stack.begin(), stack.end());

		StringStream result;
		if (!stack.empty()) {
			result.str += "------------------------------------------------\n";
			for (auto entry : stack) {
				const auto description = std::string(entry->_descr) + ":";
#if LOGURU_USE_FMTLIB
				auto prefix = textprintf("[ErrorContext] {.{}s}:{:-5u} {:-20s} ",
					filename(entry->_file), LOGURU_FILENAME_WIDTH, entry->_line, description.c_str());
#else
				auto prefix = textprintf("[ErrorContext] %*s:%-5u %-20s ",
					LOGURU_FILENAME_WIDTH, filename(entry->_file), entry->_line, description.c_str());
#endif
				result.str += prefix.c_str();
				entry->print_value(result);
				result.str += "\n";
			}
			result.str += "------------------------------------------------";
		}
		return Text(STRDUP(result.str.c_str()));
	}

	EcEntryBase::EcEntryBase(const char* file, unsigned line, const char* descr)
		: _file(file), _line(line), _descr(descr)
	{
		EcEntryBase*& ec_head = get_thread_ec_head_ref();
		_previous = ec_head;
		ec_head = this;
	}

	EcEntryBase::~EcEntryBase()
	{
		get_thread_ec_head_ref() = _previous;
	}

	// ------------------------------------------------------------------------

	Text ec_to_text(const char* value)
	{
		// Add quotes around the string to make it obvious where it begin and ends.
		// This is great for detecting erroneous leading or trailing spaces in e.g. an identifier.
		auto str = "\"" + std::string(value) + "\"";
		return Text{STRDUP(str.c_str())};
	}

	Text ec_to_text(char c)
	{
		// Add quotes around the character to make it obvious where it begin and ends.
		std::string str = "'";

		auto write_hex_digit = [&](unsigned num)
		{
			if (num < 10u) { str += char('0' + num); }
			else           { str += char('a' + num - 10); }
		};

		auto write_hex_16 = [&](uint16_t n)
		{
			write_hex_digit((n >> 12u) & 0x0f);
			write_hex_digit((n >>  8u) & 0x0f);
			write_hex_digit((n >>  4u) & 0x0f);
			write_hex_digit((n >>  0u) & 0x0f);
		};

		if      (c == '\\') { str += "\\\\"; }
		else if (c == '\"') { str += "\\\""; }
		else if (c == '\'') { str += "\\\'"; }
		else if (c == '\0') { str += "\\0";  }
		else if (c == '\b') { str += "\\b";  }
		else if (c == '\f') { str += "\\f";  }
		else if (c == '\n') { str += "\\n";  }
		else if (c == '\r') { str += "\\r";  }
		else if (c == '\t') { str += "\\t";  }
		else if (0 <= c && c < 0x20) {
			str += "\\u";
			write_hex_16(static_cast<uint16_t>(c));
		} else { str += c; }

		str += "'";

		return Text{STRDUP(str.c_str())};
	}

	#define DEFINE_EC(Type)                        \
		Text ec_to_text(Type value)                \
		{                                          \
			auto str = std::to_string(value);      \
			return Text{STRDUP(str.c_str())};      \
		}

	DEFINE_EC(int)
	DEFINE_EC(unsigned int)
	DEFINE_EC(long)
	DEFINE_EC(unsigned long)
	DEFINE_EC(long long)
	DEFINE_EC(unsigned long long)
	DEFINE_EC(float)
	DEFINE_EC(double)
	DEFINE_EC(long double)

	#undef DEFINE_EC

	Text ec_to_text(EcHandle ec_handle)
	{
		Text parent_ec = get_error_context_for(ec_handle);
		char* with_newline = reinterpret_cast<char*>(malloc(strlen(parent_ec.c_str()) + 2));
		with_newline[0] = '\n';
		strcpy(with_newline + 1, parent_ec.c_str());
		return Text(with_newline);
	}

	// ----------------------------------------------------------------------------

} // namespace loguru

// ----------------------------------------------------------------------------
// .dP"Y8 88  dP""b8 88b 88    db    88     .dP"Y8
// `Ybo." 88 dP   `" 88Yb88   dPYb   88     `Ybo."
// o.`Y8b 88 Yb  "88 88 Y88  dP__Yb  88  .o o.`Y8b
// 8bodP' 88  YboodP 88  Y8 dP""""Yb 88ood8 8bodP'
// ----------------------------------------------------------------------------

#ifdef _WIN32
namespace loguru {
	void install_signal_handlers(bool unsafe_signal_handler)
	{
		(void)unsafe_signal_handler;
		// TODO: implement signal handlers on windows
	}
} // namespace loguru

#else // _WIN32

namespace loguru
{
	struct Signal
	{
		int         number;
		const char* name;
	};
	const Signal ALL_SIGNALS[] = {
#if LOGURU_CATCH_SIGABRT
		{ SIGABRT, "SIGABRT" },
#endif
		{ SIGBUS,  "SIGBUS"  },
		{ SIGFPE,  "SIGFPE"  },
		{ SIGILL,  "SIGILL"  },
		{ SIGINT,  "SIGINT"  },
		{ SIGSEGV, "SIGSEGV" },
		{ SIGTERM, "SIGTERM" },
	};

	void write_to_stderr(const char* data, size_t size)
	{
		auto result = write(STDERR_FILENO, data, size);
		(void)result; // Ignore errors.
	}

	void write_to_stderr(const char* data)
	{
		write_to_stderr(data, strlen(data));
	}

	void call_default_signal_handler(int signal_number)
	{
		struct sigaction sig_action;
		memset(&sig_action, 0, sizeof(sig_action));
		sigemptyset(&sig_action.sa_mask);
		sig_action.sa_handler = SIG_DFL;
		sigaction(signal_number, &sig_action, NULL);
		kill(getpid(), signal_number);
	}

	static bool s_unsafe_signal_handler = false;

	void signal_handler(int signal_number, siginfo_t*, void*)
	{
		const char* signal_name = "UNKNOWN SIGNAL";

		for (const auto& s : ALL_SIGNALS) {
			if (s.number == signal_number) {
				signal_name = s.name;
				break;
			}
		}

		// --------------------------------------------------------------------
		/* There are few things that are safe to do in a signal handler,
		   but writing to stderr is one of them.
		   So we first print out what happened to stderr so we're sure that gets out,
		   then we do the unsafe things, like logging the stack trace.
		*/

		if (g_colorlogtostderr && s_terminal_has_color) {
			write_to_stderr(terminal_reset());
			write_to_stderr(terminal_bold());
			write_to_stderr(terminal_light_red());
		}
		write_to_stderr("\n");
		write_to_stderr("Loguru caught a signal: ");
		write_to_stderr(signal_name);
		write_to_stderr("\n");
		if (g_colorlogtostderr && s_terminal_has_color) {
			write_to_stderr(terminal_reset());
		}

		// --------------------------------------------------------------------

		if (s_unsafe_signal_handler) {
			// --------------------------------------------------------------------
			/* Now we do unsafe things. This can for example lead to deadlocks if
			   the signal was triggered from the system's memory management functions
			   and the code below tries to do allocations.
			*/

			flush();
			char preamble_buff[LOGURU_PREAMBLE_WIDTH];
			print_preamble(preamble_buff, sizeof(preamble_buff), Verbosity_FATAL, "", 0);
			auto message = Message{Verbosity_FATAL, "", 0, preamble_buff, "", "Signal: ", signal_name};
			try {
				log_message(1, message, false, false);
			} catch (...) {
				// This can happed due to s_fatal_handler.
				write_to_stderr("Exception caught and ignored by Loguru signal handler.\n");
			}
			flush();

			// --------------------------------------------------------------------
		}

		call_default_signal_handler(signal_number);
	}

	void install_signal_handlers(bool unsafe_signal_handler)
	{
		s_unsafe_signal_handler = unsafe_signal_handler;

		struct sigaction sig_action;
		memset(&sig_action, 0, sizeof(sig_action));
		sigemptyset(&sig_action.sa_mask);
		sig_action.sa_flags |= SA_SIGINFO;
		sig_action.sa_sigaction = &signal_handler;
		for (const auto& s : ALL_SIGNALS) {
			CHECK_F(sigaction(s.number, &sig_action, NULL) != -1,
				"Failed to install handler for " LOGURU_FMT(s) "", s.name);
		}
	}
} // namespace loguru

#endif // _WIN32

#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
#endif // _WIN32

#endif // LOGURU_IMPLEMENTATION
