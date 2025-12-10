// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file   vtkStringFormatter.h
 * @brief  Optimized C++ utilities for formatting values to strings and files.
 *
 * This header provides efficient, alternatives to common C/C++ string handling
 * functions such as `printf`, `strtol` etc.
 *
 * It includes utilities for converting strings to numbers and scanning values from
 * strings and files.
 *
 * Refer to the documentation for guidance on replacing standard C functions with
 * their modern, type-safe counterparts provided here.
 *
 * 1. C/C++ has the following functions to convert one/many numbers to a char* or string.
 *    1. itoa/_itoa, ltoa/_ltoa, lltoa/_i64toa, ultoa/_ultoa, ulltoa/_ulltoa/_ui64toa
 *    2. sprintf, sprintf_s, vsprintf, vsprintf_s,
 *    3. snprintf, snprintf_s, vsnprintf, vsnprintf_s,
 *    4. strftime
 *    5. std::to_chars, std::to_string
 *    6. std::put_time
 * These functions should be replaced by:
 *    1. vtk::to_chars or vtk::to_string, if one number needs to be converted
 *    2. vtk::format, vtk::format_to, or vtk::format_to_n, if one/many
 *       numbers need to be converted with a specific format
 *
 * 2. C/C++ has the following functions to print one/many numbers to stdout/file.
 *    1. printf, printf_s, vprintf, vprintf_s,
 *    2. fprintf, fprintf_s, vfprintf, vfprintf_s,
 *    3. std::cout, std::ofstream, vtksys::ofstream
 * These functions should be replaced by:
 *    1. vtk::print, vtk::println
 */

#ifndef vtkStringFormatter_h
#define vtkStringFormatter_h

#include "vtkCharConvCompatibility.h" // For std::chars_format, std::to_chars_result
#include "vtkCommonCoreModule.h"      // For export macro
#include "vtkLogger.h"                // For vtkLogF

#include "vtk_fmt.h" // For fmt
// clang-format off
#include VTK_FMT(fmt/args.h)    // For fmt's args
#include VTK_FMT(fmt/chrono.h)  // For fmt's chrono
#include VTK_FMT(fmt/compile.h) // For fmt's compile
#include VTK_FMT(fmt/format.h)  // For fmt's format
#include VTK_FMT(fmt/ranges.h)  // For fmt's ranges
// clang-format on

#include <string>      // For std::string
#include <string_view> // For std::string_view

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
///@{
/**
 * Given a number, convert it to a string within char* first and char* last, and return a
 * to_chars_result;
 *
 * @note The standard library provides these functions as of C++17, but they aren't as fast,
 * or fully implemented.
 */
template <typename T,
  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
VTK_ALWAYS_INLINE auto to_chars(char* first, char* last, const T& value, int base = 10)
  -> std::to_chars_result
{
  const std::size_t buffer_size = std::distance(first, last);
  if (buffer_size == 0)
  {
    return { first, std::errc::value_too_large };
  }
  const std::size_t buffer_size_1 = buffer_size - 1;
  fmt::format_to_n_result<char*> result;
  switch (base)
  {
    case 2:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:b}"), value);
      break;
    case 8:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:o}"), value);
      break;
    case 16:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:x}"), value);
      break;
    case 10:
    default:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:d}"), value);
  }
  if (result.size >= buffer_size_1)
  {
    return { first + buffer_size_1, std::errc::value_too_large };
  }
  *result.out = '\0';                 // Null-terminate
  return { result.out, std::errc{} }; // Success
}
template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
VTK_ALWAYS_INLINE auto to_chars(char* first, char* last, const T& value, std::chars_format format)
  -> std::to_chars_result
{
  const std::size_t buffer_size = std::distance(first, last);
  if (buffer_size == 0)
  {
    return { first, std::errc::value_too_large };
  }
  const std::size_t buffer_size_1 = buffer_size - 1;
  fmt::format_to_n_result<char*> result;
  switch (format)
  {
    case std::chars_format::scientific:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:e}"), value);
      break;
    case std::chars_format::fixed:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:f}"), value);
      break;
    case std::chars_format::hex:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:a}"), value);
      break;
    case std::chars_format::general:
    default:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:g}"), value);
  }
  if (result.size >= buffer_size_1)
  {
    return { first + buffer_size_1, std::errc::value_too_large };
  }
  *result.out = '\0';                 // Null-terminate
  return { result.out, std::errc{} }; // Success
}
template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
VTK_ALWAYS_INLINE auto to_chars(char* first, char* last, const T& value, std::chars_format format,
  int precision) -> std::to_chars_result
{
  const std::size_t buffer_size = std::distance(first, last);
  if (buffer_size == 0)
  {
    return { first, std::errc::value_too_large };
  }
  const std::size_t buffer_size_1 = buffer_size - 1;
  fmt::format_to_n_result<char*> result;
  switch (format)
  {
    case std::chars_format::scientific:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:.{}e}"), value, precision);
      break;
    case std::chars_format::fixed:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:.{}f}"), value, precision);
      break;
    case std::chars_format::hex:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:.{}a}"), value, precision);
      break;
    case std::chars_format::general:
    default:
      result = fmt::format_to_n(first, buffer_size_1, FMT_COMPILE("{:.{}g}"), value, precision);
  }
  if (result.size >= buffer_size_1)
  {
    return { first + buffer_size_1, std::errc::value_too_large };
  }
  *result.out = '\0';                 // Null-terminate
  return { result.out, std::errc{} }; // Success
}
///@}

// Create a macro that evaluates the result of to_chars, checks for errors, and executes a command
#define VTK_TO_CHARS_RESULT_IF_ERROR_COMMAND(to_chars_result, value, command)                      \
  switch (to_chars_result.ec)                                                                      \
  {                                                                                                \
    case std::errc::invalid_argument:                                                              \
    {                                                                                              \
      vtkLogF(ERROR, "The given argument was invalid, failed to get the converted " #value ".");   \
      command;                                                                                     \
    }                                                                                              \
    case std::errc::value_too_large:                                                               \
    {                                                                                              \
      vtkLogF(ERROR, "The given buffer was too small, failed to get the converted " #value ".");   \
      command;                                                                                     \
    }                                                                                              \
    default:                                                                                       \
    {                                                                                              \
    }                                                                                              \
  }
// Create a macro that evaluates the result of to_chars, checks for errors, and breaks
#define VTK_TO_CHARS_RESULT_IF_ERROR_BREAK(to_chars_result, value)                                 \
  VTK_TO_CHARS_RESULT_IF_ERROR_COMMAND(to_chars_result, value, break)
// Create a macro that evaluates the result of to_chars, checks for errors, and returns a value
#define VTK_TO_CHARS_RESULT_IF_ERROR_RETURN(to_chars_result, value, returnValue)                   \
  VTK_TO_CHARS_RESULT_IF_ERROR_COMMAND(to_chars_result, value, return returnValue)

/**
 * Given a number, convert it to a string.
 *
 * @note The standard library provides this function as of C++11, but it's not as fast.
 */
using fmt::to_string;

/**
 * Given a format and a set of numbers/variables return the size of the formatting string
 *
 * @note The standard library provides this function as of C++20, but we support up to C++17.
 */
using fmt::formatted_size;

/**
 * Given a format and a set of numbers/variables convert it to a string.
 *
 * @note The standard library provides this function as of C++20, but we support up to C++17.
 */
using fmt::format;

/**
 * The result type of a format_to operation.
 */
using fmt::format_to_result;

/**
 * Given a format and a set of numbers/variables convert it to a string within out.
 *
 * @remark `format_to` does not append a terminating null character.
 * @note The standard library provides this function as of C++20, but we support up to C++17.
 */
using fmt::format_to;

/**
 * The result type of a format_to_n operation.
 */
using fmt::format_to_n_result;

/**
 * Given a format and a set of numbers/variables convert it to a string within out using up to n
 * characters. It returns a format_to_n_result.
 *
 * @remark `format_to_n` does not append a terminating null character.
 * @note The standard library provides this function as of C++20, but we support up to C++17.
 */
using fmt::format_to_n;

/**
 * Converts given time since epoch as ``std::time_t`` value into calendar time,
 * expressed in local time. Unlike ``std::localtime``, this function is
 * thread-safe on most platforms.
 *
 * @note The standard library provides this function, but it's not thread-safe.
 */
using fmt::localtime;

/**
 * Converts given time since epoch as ``std::time_t`` value into calendar time,
 * expressed in Coordinated Universal Time (UTC). Unlike ``std::gmtime``, this
 * function is thread-safe on most platforms.
 *
 * @note The standard library provides this function, but it's not thread-safe.
 */
using fmt::gmtime;

/**
 * Given a format and a set of numbers/variables print it to a file/stdout.
 *
 * @note The standard library provides this function as of C++23, but we support up to C++17.
 */
using fmt::print;

/**
 * Given a format and a set of numbers/variables print it to file/stdout with a newline.
 *
 * @note The standard library provides this function as of C++23, but we support up to C++17.
 */
using fmt::println;

/**
 * Check if the given string is a printf style format.
 *
 * @note The standard library does not provide this function.
 */
VTKCOMMONCORE_EXPORT bool is_printf_format(const std::string& format);

/**
 * Convert a printf style format to a std::format style format.
 *
 * @note The standard library does not provide this function.
 */
VTKCOMMONCORE_EXPORT std::string printf_to_std_format(const std::string& printf_format);

/**
 * Convert printf and std::format style format strings. If a printf style
 * string is passed in, convert it and return a std::format string.
 */
VTKCOMMONCORE_EXPORT std::string to_std_format(const std::string& format);

VTK_ABI_NAMESPACE_END
}

#endif // vtkStringFormatter_h
// VTK-HeaderTest-Exclude: vtkStringFormatter.h
