// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStringFormatter.h"

#include <any>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

// clang-format off
#include VTK_FMT(fmt/printf.h)
// clang-format on

// Type-safe format test case with tuple arguments
template <typename... Args>
struct ArgTestCase
{
  std::string printf_format;       // printf-style format string
  std::tuple<Args...> args;        // Arguments for testing
  bool expected_valid;             // Whether the format is expected to be valid
  std::string expected_std_format; // Expected std::format string (for conversion)
  std::string test_description;    // Description of the test case
};
// Validation function that uses both sprintf and std::format
class FormatValidator
{
private:
  // Validate format string by comparing sprintf and std::format outputs
  template <typename... Args>
  static std::tuple<bool, std::string, std::string> validate_printf_format(
    const std::string& printf_format, const std::string& std_format, Args... args)
  {
    std::string snprintf_output, std_format_output;
    try
    {
      // Run sprintf
      snprintf_output = fmt::sprintf(printf_format, args...);
    }
    catch (const std::exception& e)
    {
      vtkLogF(ERROR, "Failed to convert using sprintf. Reason: %s. Printf format used %s", e.what(),
        printf_format.c_str());
      return { false, snprintf_output, std_format_output };
    }
    try
    {
      // Run std::format
      std_format_output = fmt::format(std_format, args...);
    }
    catch (const std::exception& e)
    {
      vtkLogF(ERROR, "Failed to convert using std::format. Reason: %s. std::format used %s",
        e.what(), std_format.c_str());
      return { false, snprintf_output, std_format_output };
    }
    return { snprintf_output == std_format_output, snprintf_output, std_format_output };
  }

public:
  // Wrapper for tuple-based validation of printf format strings
  template <typename... Args>
  static std::tuple<bool, std::string, std::string> validate_printf_format_tuple(
    const std::string& printf_format, const std::string& std_format,
    const std::tuple<Args...>& args)
  {
    return std::apply([&](Args... unpacked_args)
      { return validate_printf_format(printf_format, std_format, unpacked_args...); },
      args);
  }
};

// Function to run comprehensive format string tests
static int run_comprehensive_format_tests()
{
  std::vector<ArgTestCase<char>> char_test_cases = {
    // Basic
    { "%c", std::make_tuple('A'), true, "{0:c}", "Basic character" },

    // Flags
    { "%-c", std::make_tuple('B'), true, "{0:<c}", "Char with left justification" },
    { "%+c", std::make_tuple('C'), false, "%+c", "Invalid Char with plus sign" },
    { "% c", std::make_tuple('D'), false, "% c", "Invalid Char with space sign" },
    { "%#c", std::make_tuple('E'), false, "%#c", "Invalid Char with alternate form" },

    // Width
    { "%8c", std::make_tuple('F'), true, "{0:>8c}", "Char with width" },

    // Zero-padding
    { "%08c", std::make_tuple('H'), false, "%08c", "Invalid Char with zero-padding" },
    { "%0*c", std::make_tuple('I'), false, "%0*c", "Invalid Char with dynamic zero-padding" },

    // Note: %c doesn’t support precision or length modifiers in standard printf

    // Combinations
    { "%-8c", std::make_tuple('J'), true, "{0:<8c}", "Char with left justify and width" },

    // Text and multiple specifiers
    { "Letter: %c", std::make_tuple('K'), true, "Letter: {0:c}", "Char with text" },

    // Invalid
    { "%.2c", std::make_tuple('M'), false, "%.2c", "Invalid precision with char" },
  };

  std::vector<ArgTestCase<int, char>> char_test_cases_two_variables = {
    { "%c%c", std::make_tuple('L', 'a'), true, "{0:c}{1:c}", "Consecutive chars" },
    { "%*c", std::make_tuple(8, 'G'), true, "{1:>{0}c}", "Char with dynamic width" },
  };

  std::vector<ArgTestCase<const char*>> string_test_cases = {
    // Basic
    { "%s", std::make_tuple("hello"), true, "{0:s}", "Basic string" },

    // Flags
    { "%-s", std::make_tuple("hello"), true, "{0:<s}", "String with left justification" },
    { "%+s", std::make_tuple("hello"), false, "%+s", "Invalid String with plus sign" },
    { "% s", std::make_tuple("hello"), false, "% s", "Invalid String with space sign" },
    { "%#s", std::make_tuple("hello"), false, "%#s", "Invalid String with alternate form" },

    // Width
    { "%10s", std::make_tuple("hello"), true, "{0:>10s}", "String with width" },

    // Precision
    { "%.3s", std::make_tuple("hello"), true, "{0:.3s}", "String with precision (truncate)" },
    { "%.s", std::make_tuple("hello"), true, "{0:.0s}", "String with precision (truncate)" },

    // Width and precision
    { "%10.3s", std::make_tuple("hello"), true, "{0:>10.3s}",
      "String with width and precision (truncate)" },
    { "%8.s", std::make_tuple("hello"), true, "{0:>8.0s}",
      "String with width and precision (truncate)" },

    // Zero-padding (not typically used with %s, but valid syntax)
    { "%010s", std::make_tuple("hello"), false, "%010s", "Invalid String with zero-padding" },
    { "%0*s", std::make_tuple("hello"), false, "%0*s", "Invalid String with dynamic zero-padding" },

    // Combinations
    { "%-10.3s", std::make_tuple("hello"), true, "{0:<10.3s}",
      "String with left justify, width, precision" },

    // Text and multiple specifiers
    { "Name: %s!", std::make_tuple("Alice"), true, "Name: {0:s}!", "String with text" },

    // Invalid
    { "%s%", std::make_tuple("hello"), false, "%s%", "Invalid trailing %" },
  };

  std::vector<ArgTestCase<int, const char*>> string_test_cases_two_variables = {
    { "%*s", std::make_tuple(8, "world"), true, "{1:>{0}s}", "String with dynamic width" },
    { "%.*s", std::make_tuple(8, "hello"), true, "{1:.{0}s}", "String with dynamic precision" },
  };

  std::vector<ArgTestCase<int, int, const char*>> string_test_cases_three_variables = {
    { "%*.*s", std::make_tuple(8, 10, "hello"), true, "{2:>{0}.{1}s}",
      "String with dynamic width and precision" },
    { "%d %d %-8.3s", std::make_tuple(8, 10, "hello"), true, "{0:d} {1:d} {2:<8.3s}",
      "Numbers and string with width and precision" },
  };

  std::vector<ArgTestCase<int>> integer_test_cases = {
    // Basic
    { "%d", std::make_tuple(42), true, "{0:d}", "Basic decimal integer" },
    { "%i", std::make_tuple(-42), true, "{0:d}", "Signed integer" },

    // Flags
    { "%+d", std::make_tuple(42), true, "{0:+d}", "Integer with plus sign" },
    { "% d", std::make_tuple(42), true, "{0: d}", "Integer with space sign" },
    { "%-d", std::make_tuple(42), true, "{0:<d}", "Integer with left justification" },
    { "%#d", std::make_tuple(42), false, "%#d", "Integer with alternate form (no allowed)" },

    // Width
    { "%8d", std::make_tuple(42), true, "{0:8d}", "Integer with width" },

    // Precision
    { "%.4d", std::make_tuple(42), true, "{0:04d}", "Integer with precision" },
    { "%.d", std::make_tuple(42), true, "{0:d}", "Integer with precision" },

    // Zero-padding
    { "%08d", std::make_tuple(42), true, "{0:08d}", "Integer with zero-padding" },

    // Left Justification with and/or width and/or precision
    { "%-8d", std::make_tuple(42), true, "{0:<8d}", "Integer with left justification" },

    { "%-08d", std::make_tuple(42), true, "{0:<08d}",
      "Integer with left justification and zero-padding" },
    { "%-.d", std::make_tuple(42), true, "{0:<d}",
      "Integer with left justification and default precision" },
    { "%-.4d", std::make_tuple(42), true, "{0:04d}",
      "Integer with left justification and precision" },

    // Length modifiers
    { "%hd", std::make_tuple(42), true, "{0:d}", "Short integer" },
    { "%hhd", std::make_tuple(42), true, "{0:d}", "Double short integer" },

    { "%ld", std::make_tuple(42), true, "{0:d}", "Long integer" },
    { "%lld", std::make_tuple(42), true, "{0:d}", "Long long integer" },

    // Combinations
    { "%08ld", std::make_tuple(42), true, "{0:08d}", "Long integer with zero-padding" },

    // Invalid
    { "%   d", std::make_tuple(42), false, "%   d", "Invalid spacing" },
    { "%d%", std::make_tuple(42), false, "%d%", "Invalid trailing %" },

    // Width and precision not supported by std::format for integers
    // { "%10.8d", std::make_tuple(42), true, "{0:>08d}", "Integer with width and precision" },
    //{ "%8.d", std::make_tuple(42), true, "{0:8d}", "Integer with width and precision" },
    // Zero padding with precision
    //{ "%010.8d", std::make_tuple(42), true, "{0:>8d}", "Integer with zero-padding and precision"
    //},
    //{ "%08.d", std::make_tuple(42), true, "{0:08d}", "Integer with zero-padding and precision" },
    // { "%-10.8d", std::make_tuple(42), true, "{0:<8d}",
    //   "Integer with left justification and precision" },
    //{ "%-010.8d", std::make_tuple(42), true, "{0:<8d}",
    //"Integer with left justification, zero-padding, and precision" },
    // { "%-010.8d", std::make_tuple(42), true, "{0:<8d}",
    //   "Integer with left justification, zero-padding, and precision" },
    // { "%+10.8d", std::make_tuple(42), true, "{0:+>08d}", "Integer with plus, width, precision" },

  };

  std::vector<ArgTestCase<int, int>> integer_test_cases_two_variables = {
    { "%d%d", std::make_tuple(42, 24), true, "{0:d}{1:d}", "Consecutive integers" },
    { "%0*d", std::make_tuple(8, 42), true, "{1:0{0}d}", "Integer with dynamic zero-padding" },
    { "%*d", std::make_tuple(8, 42), true, "{1:{0}d}", "Integer with dynamic width" },
    { "%.*d", std::make_tuple(8, 42), true, "{1:0{0}d}", "Integer with dynamic precision" },
    // Width and precision not supported by std::format for integers
    //{ "%8.*d", std::make_tuple(8, 42), true, "{1:8d}",
    //"Integer with zero-padding and dynamic precision" },
  };

  // std::vector<ArgTestCase<int, int, int>> integer_test_cases_three_variables = {
  //   { "%0*.*d", std::make_tuple(8, 10, 42), true, "{2:<0{0}.{1}d}",
  //     "Integer with zero padding and dynamic width and precision" },
  //   { "%*.*d", std::make_tuple(8, 10, 42), true, "{2:<0{0}.{1}d}",
  //     "Integer with zero padding and dynamic width and precision" },
  // };

  std::vector<ArgTestCase<unsigned int>> unsigned_test_cases = {
    // Basic
    { "%u", std::make_tuple(42u), true, "{0:d}", "Basic unsigned decimal" },
    { "%o", std::make_tuple(42u), true, "{0:o}", "Unsigned octal" },
    { "%x", std::make_tuple(42u), true, "{0:x}", "Unsigned hex lowercase" },
    { "%X", std::make_tuple(42u), true, "{0:X}", "Unsigned hex uppercase" },

    // Flags
    { "%+u", std::make_tuple(42u), false, "%+u", "Invalid Unsigned with plus sign" },
    { "% u", std::make_tuple(42u), false, "% u", "Invalid Unsigned with space sign" },
    { "%-u", std::make_tuple(42u), true, "{0:<d}", "Unsigned with left justification" },
    { "%#x", std::make_tuple(42u), true, "{0:#x}", "Hex with alternate form (0x)" },

    // Width
    { "%8u", std::make_tuple(42u), true, "{0:8d}", "Unsigned with width" },

    // Precision
    { "%.2u", std::make_tuple(42u), true, "{0:02d}", "Unsigned with precision" },

    // Zero-padding
    { "%08u", std::make_tuple(42u), true, "{0:08d}", "Unsigned with zero-padding" },

    // Length modifiers
    { "%hu", std::make_tuple(42u), true, "{0:d}", "Unsigned short" },
    { "%lu", std::make_tuple(42u), true, "{0:d}", "Unsigned long" },
    { "%llu", std::make_tuple(42u), true, "{0:d}", "Unsigned long long" },

    // Combinations
    { "%+10.8u", std::make_tuple(42u), false, "%+10.8u",
      "Invalid Unsigned with plus, width, precision" },
    { "%#08lu", std::make_tuple(42u), false, "%#08lu",
      "Invalid Unsigned long with alternate and zero-padding" },

    // Text and multiple specifiers
    { "Value: %-10x", std::make_tuple(42u), true, "Value: {0:<10x}",
      "Unsigned with left justification, width and text" },
    { "Value: %-#10x", std::make_tuple(42u), true, "Value: {0:<#10x}",
      "Unsigned with left justification, alternative form, width and text" },
    { "Value: %-#.x", std::make_tuple(42u), true, "Value: {0:<#x}",
      "Unsigned with left justification, alternative form, default precision and text" },

    // Invalid
    { "%u%", std::make_tuple(42u), false, "%u%", "Invalid trailing %" },

    // Width and precision not supported by std::format for unsigned integers
    // { "%-0*.*x", std::make_tuple(42u), true, "{4:<0{0}.{1}x}", "Hex with all flags dynamic" },
  };

  std::vector<ArgTestCase<int, unsigned int>> unsigned_test_cases_two_variables = {
    { "%*o", std::make_tuple(8, 42u), true, "{1:{0}o}", "Octal with dynamic width" },
    { "%.*x", std::make_tuple(8, 42u), true, "{1:.{0}x}", "Hex with dynamic precision" },
    { "%0*X", std::make_tuple(8, 42u), true, "{1:>0{0}X}",
      "Hex uppercase with dynamic zero-padding" },
    { "%x%X", std::make_tuple(50, 42u), true, "{0:x}{1:X}", "Consecutive hex specifiers" },
  };

  std::vector<ArgTestCase<float>> float_test_cases = {
    // Basic
    { "%f", std::make_tuple(3.14189f), true, "{0:f}", "Basic float" },
    { "%e", std::make_tuple(3.14189e-8f), true, "{0:e}", "Scientific lowercase" },
    { "%E", std::make_tuple(3.14189e8f), true, "{0:E}", "Scientific uppercase" },
    { "%g", std::make_tuple(0.0314189f), true, "{0:g}", "General format" },

    // Flags
    { "%+f", std::make_tuple(3.14189f), true, "{0:+f}", "Float with plus sign" },
    { "% f", std::make_tuple(3.14189f), true, "{0: f}", "Float with space sign" },
    { "%-f", std::make_tuple(3.14189f), true, "{0:<f}", "Float with left justification" },
    { "%#f", std::make_tuple(3.14189f), true, "{0:#f}",
      "Float with alternate form (trailing zeros)" },

    // Width
    { "%10f", std::make_tuple(3.14189f), true, "{0:10f}", "Float with width" },

    // Precision
    { "%.2f", std::make_tuple(3.14189f), true, "{0:.2f}", "Float with precision" },

    // Zero-padding
    { "%010f", std::make_tuple(3.14189f), true, "{0:010f}", "Float with zero-padding" },

    // Length modifiers
    { "%Lf", std::make_tuple(3.14189f), true, "{0:f}", "Long double (treated as float here)" },

    // Combinations
    { "%+10.2f", std::make_tuple(3.14189f), true, "{0:+10.2f}",
      "Float with plus, width, precision" },
    { "%#010.8f", std::make_tuple(3.14189f), true, "{0:#010.8f}",
      "Float with alternate and zero-padding" },

    // Invalid
    { "%f%", std::make_tuple(3.14189f), false, "%f%", "Invalid trailing %" },
  };

  std::vector<ArgTestCase<int, float>> float_test_cases_two_variables = {
    { "%*f", std::make_tuple(15, 3.14189f), true, "{1:{0}f}", "Float with dynamic width" },
    { "%.*f", std::make_tuple(15, 3.14189f), true, "{1:.{0}f}", "Float with dynamic precision" },
    { "%0*f", std::make_tuple(15, 3.14189f), true, "{1:0{0}f}", "Float with dynamic zero-padding" },
  };

  std::vector<ArgTestCase<int, int, float>> float_test_cases_three_variables = {
    { "%-0*.*f", std::make_tuple(15, 20, 3.14189f), true, "{2:<0{0}.{1}f}",
      "Float with all flags dynamic" },
  };

  std::vector<ArgTestCase<double>> double_test_cases = {
    // Basic
    { "%f", std::make_tuple(3.1418926838), true, "{0:f}", "Basic double" },
    { "%e", std::make_tuple(3.14189e-10), true, "{0:e}", "Scientific lowercase" },
    { "%E", std::make_tuple(3.14189e10), true, "{0:E}", "Scientific uppercase" },
    { "%g", std::make_tuple(0.0000314189), true, "{0:g}", "General format (scientific)" },
    { "%G", std::make_tuple(31418.9), true, "{0:G}", "General format (decimal, uppercase)" },
    { "%a", std::make_tuple(3.1418926838), true, "{0:a}", "Hex float lowercase" },
    { "%A", std::make_tuple(3.1418926838), true, "{0:A}", "Hex float uppercase" },

    // Flags
    { "%+f", std::make_tuple(3.1418926838), true, "{0:+f}", "Double with plus sign" },
    { "% f", std::make_tuple(3.1418926838), true, "{0: f}", "Double with space sign" },
    { "%-f", std::make_tuple(3.1418926838), true, "{0:<f}", "Double with left justification" },
    { "%#g", std::make_tuple(3.1418926838), true, "{0:#g}", "General with alternate form" },

    // Width
    { "%18f", std::make_tuple(3.1418926838), true, "{0:18f}", "Double with width" },

    // Precision
    { "%.10f", std::make_tuple(3.1418926838), true, "{0:.10f}", "Double with high precision" },

    // Zero-padding
    { "%018f", std::make_tuple(3.1418926838), true, "{0:018f}", "Double with zero-padding" },

    // Length modifiers
    { "%Lf", std::make_tuple(3.1418926838), true, "{0:f}", "Long double (treated as double here)" },
    { "%lf", std::make_tuple(3.1418926838), true, "{0:f}", "Long double (treated as double here)" },

    // Combinations
    { "%-#6.3g", std::make_tuple(3.1418926838), true, "{0:<#6.3g}",
      "Double with left justify, alternate form, width, precision" },
    { "%+18.10f", std::make_tuple(3.1418926838), true, "{0:+18.10f}",
      "Double with plus, width, precision" },
    { "%#018.8g", std::make_tuple(3.1418926838), true, "{0:#018.8g}",
      "Hex float with alternate and zero-padding" },

    // Basic with length modifiers
    { "%Lf", std::make_tuple(3.1418926838), true, "{0:f}", "Long double" },
    { "%Le", std::make_tuple(3.14189e-10), true, "{0:e}", "Long double scientific lowercase" },
    { "%LA", std::make_tuple(3.1418926838), true, "{0:A}", "Long double hex uppercase" },

    // Flags with length modifiers
    { "%+Lf", std::make_tuple(3.1418926838), true, "{0:+f}", "Long double with plus sign" },
    { "%-Lf", std::make_tuple(3.1418926838), true, "{0:<f}",
      "Long double with left justification" },
    { "%#Lg", std::make_tuple(3.1418926838), true, "{0:#g}",
      "Long double general with alternate form" },

    // Width and precision with length modifiers
    { "%18Lf", std::make_tuple(3.1418926838), true, "{0:18f}", "Long double with width" },
    { "%.10Lf", std::make_tuple(3.1418926838), true, "{0:.10f}",
      "Long double with high precision" },

    // Zero-padding with length modifiers
    { "%018Lf", std::make_tuple(3.1418926838), true, "{0:018f}", "Long double with zero-padding" },

    // Combinations
    { "%+18.10Lf", std::make_tuple(3.1418926838), true, "{0:+18.10f}",
      "Long double with plus, width, precision" },

    // Text and multiple specifiers
    { "Pi: %f", std::make_tuple(3.1418926838), true, "Pi: {0:f}", "Double with text" },
    // Invalid
    { "%f%", std::make_tuple(3.1418926838), false, "%f%", "Invalid trailing %" },
  };

  std::vector<ArgTestCase<int, double>> double_test_cases_two_variables = {
    { "%*f", std::make_tuple(20, 3.1418926838), true, "{1:{0}f}", "Double with dynamic width" },
    { "%.*e", std::make_tuple(20, 3.14189e-10), true, "{1:.{0}e}",
      "Scientific with dynamic precision" },
    { "%0*f", std::make_tuple(20, 3.1418926838), true, "{1:0{0}f}",
      "Double with dynamic zero-padding" },
    { "%.*Lg", std::make_tuple(25, 3.1418926838), true, "{1:.{0}g}",
      "Long double general with dynamic precision" },
    { "%*Le", std::make_tuple(20, 3.14189e-10), true, "{1:{0}e}",
      "Long double scientific with dynamic width" },
    { "%0*LA", std::make_tuple(20, 3.1418926838), true, "{1:0{0}A}",
      "Long double hex with dynamic zero-padding" },
    { "%i%G", std::make_tuple(15, 3.14189e-10), true, "{0:d}{1:G}",
      "Consecutive double specifiers" },

  };

  std::vector<ArgTestCase<int, int, double>> double_test_cases_three_variables = {
    { "%-0*.*g", std::make_tuple(20, 25, 3.1418926838), true, "{2:<0{0}.{1}g}",
      "General with all flags dynamic" },
    { "%-0*.*Le", std::make_tuple(20, 25, 3.14189e-10), true, "{2:<0{0}.{1}e}",
      "Long double scientific with all flags dynamic" },
  };

  // Pointer test cases
  std::vector<ArgTestCase<void*>> pointer_test_cases = {
    // Basic
    { "%p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true, "{0:p}",
      "Basic pointer (null)" },
    { "%p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true, "{0:p}",
      "Basic pointer (non-null)" },

    // Flags
    { "%+p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%+p",
      "Invalid Pointer with plus sign" },
    { "% p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "% p",
      "Invalid Pointer with space sign" },
    { "%-p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%-p",
      "Invalid Pointer with left justification" },
    { "%#p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%#p",
      "Invalid Pointer with alternate form (0x prefix)" },

    // Width
    { "%10p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true, "{0:10p}",
      "Pointer with width" },

    // Zero-padding
    { "%010p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%010p",
      "Invalid Pointer with zero-padding" },

    // Length modifiers (non-standard but supported by your regex)
    { "%hp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%hp",
      "Invalid Short pointer" },
    { "%lp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%lp",
      "Invalid Long pointer" },
    { "%llp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%llp",
      "Invalid  Long long pointer" },
    { "%jp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%jp",
      "Invalid intmax_t pointer" },
    { "%zp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%zp",
      "Invalid size_t pointer" },
    { "%tp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%tp",
      "Invalid ptrdiff_t pointer" },
    { "%Lp", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%Lp",
      "Invalid Long pointer" },

    // Combinations
    { "%-10p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%-10p",
      "Invalid Pointer with left justify and width" },
    { "%#018p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%#018p",
      "Invalid Pointer with alternate form and zero-padding" },

    // Text and multiple specifiers
    { "Addr: %p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true, "Addr: {0:p}",
      "Pointer with text" },
    { "Before %p After", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true,
      "Before {0:p} After", "Pointer with text before and after" },
    { "RightBefore%pRightAfter", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true,
      "RightBefore{0:p}RightAfter", "Pointer with text right before and right after" },

    // Invalid cases
    { "%.2p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%.2p",
      "Pointer with precision (not supported)" },
    { "%p%", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%p%",
      "Pointer followed by lone %" },
    { "%   p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%   p",
      "Pointer with duplicate space flags" },
    { "%--p", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%--p",
      "Pointer with duplicate minus flags" },
    { "%p#", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true, "{0:p}#",
      "Pointer with trailing invalid character" },
    { "%pq", std::make_tuple(static_cast<void*>(&pointer_test_cases)), true, "{0:p}q",
      "Pointer with invalid trailing specifier" },
    { "%8", std::make_tuple(static_cast<void*>(&pointer_test_cases)), false, "%8",
      "Incomplete pointer specifier with width" },
  };

  std::vector<ArgTestCase<int, void*>> pointer_test_cases_two_variables = {
    { "%*p", std::make_tuple(25, static_cast<void*>(&pointer_test_cases_two_variables)), true,
      "{1:{0}p}", "Pointer with dynamic width" },
    { "%0*p", std::make_tuple(25, static_cast<void*>(&pointer_test_cases_two_variables)), false,
      "%0*p", "Invalid Pointer with dynamic zero-padding" },
    { "%-0*lp", std::make_tuple(25, static_cast<void*>(&pointer_test_cases_two_variables)), false,
      "%-0*lp", "Invalid Long pointer with dynamic width and left justify" },
  };

  std::vector<ArgTestCase<int, int, void*>> pointer_test_cases_three_variables = { { "%*.*p",
    std::make_tuple(25, 20, static_cast<void*>(&pointer_test_cases_three_variables)), false,
    "%*.*p", "Pointer with invalid dynamic width and precision" } };

  // Test cases with mixed types
  std::vector<ArgTestCase<int, float>> mixed_int_float_test_cases = {
    { "Int: %d, Float: %.2f", std::make_tuple(42, 3.14189f), true, "Int: {0:d}, Float: {1:.2f}",
      "Integer and float" },
    { "%+d %10.2f", std::make_tuple(-10, 8.678f), true, "{0:+d} {1:10.2f}",
      "Signed integer and float with width" },
    //{ "%-*d %.1f", std::make_tuple(42, 1.23f), true, "{0:<1d} {1:.1f}",
    // "Dynamic width integer and float" },
    { "%d%f", std::make_tuple(0, 0.0f), true, "{0:d}{1:f}", "Consecutive specifiers no text" },
    { "abc %lld def %f ghi", std::make_tuple(1234867890, 2.718f), true, "abc {0:d} def {1:f} ghi",
      "Long long and float with text" },
  };

  // Test cases with mixed types
  std::vector<ArgTestCase<const char*, char, unsigned int, double>> mixed_test_cases = {
    { "%s %c %u %f", std::make_tuple("hello", 'A', 42u, 3.14189), true, "{0:s} {1:c} {2:d} {3:f}",
      "String, char, unsigned, double" },
    { "Name: %-10s, Initial: %c, Age: %u, Height: %.2f", std::make_tuple("Alice", 'B', 28u, 1.78),
      true, "Name: {0:<10s}, Initial: {1:c}, Age: {2:d}, Height: {3:.2f}",
      "Mixed with flags and precision" },
    { "%200s%8c%#x%.20f", std::make_tuple("test", 'C', 288u, 2.71828), true,
      "{0:>200s}{1:>8c}{2:#x}{3:.20f}", "Mixed with dynamic width and precision" },
  };

  // Define a separate vector for the two-argument case
  std::vector<ArgTestCase<const char*, unsigned int>> mixed_two_arg_test_cases = {
    { "abc %s def %u ghi", std::make_tuple("xyz", 100u), true, "abc {0:s} def {1:d} ghi",
      "Mixed with text" },
  };

  std::vector<ArgTestCase<int>> edge_test_cases = {
    { "", std::make_tuple(0), false, "", "Empty string" },
    { "abc", std::make_tuple(0), false, "abc", "Plain text only" },
    { "%%", std::make_tuple(0), true, "%", "Escaped percentage" },
    { "abc%%def%d", std::make_tuple(42), true, "abc%def{0:d}",
      "Text with escaped % and specifier" },
    { "%--d", std::make_tuple(42), false, "%--d", "Duplicate minus flags" },
    { "%", std::make_tuple(0), false, "%", "Lone percent" },
    { "%q", std::make_tuple(0), false, "%q", "Invalid specifier" },
    { std::string(10, '%') + "d", std::make_tuple(42), true, "%%%%%d",
      "Multiple escaped % with specifier" },
  };

  std::vector<ArgTestCase<int>> more_edge_test_cases = {
    // 1. Incomplete specifier
    { "%", std::make_tuple(42), false, "%", "Lone percent sign" },

    // 2. Invalid conversion specifier
    { "%q", std::make_tuple(42), false, "%q", "Unsupported specifier q" },

    // 3. Duplicate flags
    { "%--d", std::make_tuple(42), false, "%--d", "Duplicate minus flags" },
    { "%++d", std::make_tuple(42), false, "%++d", "Duplicate plus flags" },
    { "%##d", std::make_tuple(42), false, "%##d", "Duplicate alternate flags" },

    // 4. Invalid flag combination
    { "%+-d", std::make_tuple(42), false, "%+-d", "Conflicting plus and minus flags" },

    // 5. Trailing percent after valid specifier
    { "%d%", std::make_tuple(42), false, "%d%", "Valid specifier followed by lone %" },

    // 6. Invalid length modifier for specifier
    { "%hc", std::make_tuple(42), false, "%hc",
      "Short modifier invalid for char (non-standard)" }, // Depends on strictness

    // 7. Incomplete width/precision
    { "%8", std::make_tuple(42), false, "%8", "Width without specifier" },
    { "%.", std::make_tuple(42), false, "%.", "Precision dot without number or specifier" },
    { "%.*", std::make_tuple(42), false, "%.*", "Dynamic precision without specifier" },

    // 8. Invalid characters in width/precision
    { "%8xd", std::make_tuple(42), true, "{0:8x}d", "Unsigned hex integer with in width" },
    { "%.4#d", std::make_tuple(42), false, "%.4#d", "Invalid character in precision" },

    // 9. Invalid time specifier
    { "%t", std::make_tuple(42), false, "%t", "Incomplete time specifier" },
    { "%tk", std::make_tuple(42), false, "%tk", "Unsupported time specifier k" },

    // 10. Mixed invalid flags and specifiers
    { "%+-#q", std::make_tuple(42), false, "%+-#q", "Conflicting flags with invalid specifier" },

    // 11. Non-standard positional argument
    { "%1$d", std::make_tuple(42), false, "%1$d", "Positional argument not supported" },

    // 12. Grouping flag (non-standard)
    { "%'d", std::make_tuple(42), false, "%'d", "Grouping flag not supported" },

    // 13. Invalid zero-padding syntax
    { "%0-d", std::make_tuple(42), false, "%0-d", "Zero-padding with minus conflict" },

    // 14. Invalid specifier after valid text
    { "abc %q def", std::make_tuple(42), false, "abc %q def", "Invalid specifier in text" },

    // 15. Multiple consecutive percents without escape
    { "%%%", std::make_tuple(42), false, "%%%", "Odd number of % signs" },

    // 16. Invalid length modifier combination
    { "%hlld", std::make_tuple(42), false, "%hlld", "Mixed length modifiers h and ll" },

    // 17. Invalid flag after length modifier
    { "%l#d", std::make_tuple(42), false, "%l#d", "Flag after length modifier" },

    // 18. Empty precision with length modifier
    { "%l.d", std::make_tuple(42), false, "%l.d", "Empty precision with long modifier" },

    // 19. Invalid dynamic width/precision syntax
    { "%*.*", std::make_tuple(42), false, "%*.*", "Dynamic width and precision without specifier" },

    // 20. Non-numeric width/precision
    { "%.bd", std::make_tuple(42), false, "%.bd", "Non-numeric precision" },

    // 21. Invalid specifier with text
    { "%d %q %f", std::make_tuple(42), false, "%d %q %f", "Invalid specifier between valid ones" },

    // 22. Invalid escape sequence
    { "%\\d", std::make_tuple(42), false, "%\\d", "Invalid escape sequence" },

    // 23. Overly long flag sequence
    { "%-----+d", std::make_tuple(42), false, "%-----+d", "Excessive flags" },

    // 24. Invalid character after percent
    { "%@", std::make_tuple(42), false, "%@", "Invalid character after %" },

    // 25. Invalid length modifier for time specifier
    { "%lH", std::make_tuple(42), false, "%lH", "Long modifier invalid for time specifier" },

    // 26. Mixed invalid length and specifier
    { "%j#f", std::make_tuple(42), false, "%j#f", "intmax_t modifier with float specifier" },
  };

  // Combine all test cases with arguments into a tuple
  auto printf_test_cases =
    std::make_tuple(char_test_cases, char_test_cases_two_variables, string_test_cases,
      string_test_cases_two_variables, string_test_cases_three_variables, integer_test_cases,
      integer_test_cases_two_variables, /*integer_test_cases_three_variables,*/ unsigned_test_cases,
      float_test_cases, float_test_cases_two_variables, float_test_cases_three_variables,
      double_test_cases, double_test_cases_two_variables, double_test_cases_three_variables,
      pointer_test_cases, pointer_test_cases_two_variables, pointer_test_cases_three_variables,
      mixed_int_float_test_cases, mixed_test_cases, mixed_two_arg_test_cases, edge_test_cases,
      more_edge_test_cases);

  // Test format conversion and validation
  std::cout << "\n=== Format Conversion and Validation Tests ===\n";
  // Helper lambda to test a specific argument test case collection
  auto test_printf_cases = [](const auto& cases)
  {
    for (const auto& test : cases)
    {
      try
      {
        // Convert printf format to std::format
        const bool is_printf_format = vtk::is_printf_format(test.printf_format);
        std::cout << std::left << std::setw(18) << test.printf_format
                  << " | Expected: " << (test.expected_valid ? "Valid" : "Invalid")
                  << " | Detected: " << (is_printf_format ? "Valid" : "Invalid") << " | "
                  << test.test_description
                  << (is_printf_format == test.expected_valid ? " ✓" : " ✗") << std::endl;
        if (is_printf_format != test.expected_valid)
        {
          vtkLogF(ERROR, "Format conversion test failed %s", test.printf_format.c_str());
          std::exit(EXIT_FAILURE);
        }
        if (is_printf_format)
        {
          const std::string std_format = vtk::printf_to_std_format(test.printf_format);
          if (std_format != test.expected_std_format)
          {
            vtkLogF(ERROR, "Format conversion test failed %s. Expected %s, Converted %s.",
              test.printf_format.c_str(), test.expected_std_format.c_str(), std_format.c_str());
            std::exit(EXIT_FAILURE);
          }
          // Run the test with arguments
          auto [validation_result, printf_result, std_format_result] =
            FormatValidator::validate_printf_format_tuple(
              test.printf_format, std_format, test.args);
          if (!validation_result)
          {
            vtkLogF(ERROR,
              "Format conversion produced different results. printf [in: %s, out: %s] vs "
              "std::format [in: %s, out: %s]",
              test.printf_format.c_str(), printf_result.c_str(), std_format.c_str(),
              std_format_result.c_str());
            std::exit(EXIT_FAILURE);
          }
          // Test unconditional converstion to std::format
          const std::string converted_format = vtk::to_std_format(test.printf_format);
          if (converted_format != test.expected_std_format)
          {
            vtkLogF(ERROR, "Unconditional conversion failed for %s. Expected %s, got %s.",
              test.printf_format.c_str(), test.expected_std_format.c_str(),
              converted_format.c_str());
            std::exit(EXIT_FAILURE);
          }
        }
        else
        {
          // Check that std::format strings remain unchanged when passed to vtk::to_std_format.
          // Only try test cases that produce a valid std::format string.
          if (test.expected_valid)
          {
            const std::string converted_format = vtk::to_std_format(test.expected_std_format);
            if (converted_format != test.expected_std_format)
            {
              vtkLogF(ERROR,
                "Unconditional conversion failed for std::format string %s. Expected %s, got %s.",
                test.expected_std_format.c_str(), test.expected_std_format.c_str(),
                converted_format.c_str());
              std::exit(EXIT_FAILURE);
            }
          }
        }
      }
      catch (const std::exception& e)
      {
        vtkLogF(
          ERROR, "Invalid format detected: %s. Error: %s", test.printf_format.c_str(), e.what());
        std::exit(EXIT_FAILURE);
      }
    }
  };
  // test printf_arg_test_cases
  // Iterate over the tuple and apply the test function to each vector
  std::apply([&](auto&... vectors) { ([&](auto& vec) { test_printf_cases(vec); }(vectors), ...); },
    printf_test_cases);
  return EXIT_SUCCESS;
}

int TestPrintfToStdFormatConversion(int, char*[])
{
  try
  {
    return run_comprehensive_format_tests();
  }
  catch (const std::exception& e)
  {
    vtkLogF(ERROR, "Test failed: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
