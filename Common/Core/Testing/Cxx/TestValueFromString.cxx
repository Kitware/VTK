// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkValueFromString.h"

#include <array>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#define Check(expr, message)                                                                       \
  if (!(expr))                                                                                     \
  {                                                                                                \
    std::cout << __FILE__ << " L." << __LINE__ << " | " << #expr << " failed: \n"                  \
              << message << std::endl;                                                             \
    return false;                                                                                  \
  }                                                                                                \
  do                                                                                               \
  {                                                                                                \
  } while (false)

// Convert a value to hex, oct or dec representation and parse it to check if parsed value is equal
// to input
template <typename T>
static bool TestIntParseHelper(T value, std::ios_base::fmtflags flags)
{
  std::ostringstream oss{};
  // don't use showbase for octal because stdlib uses unsupported 0{value} format
  if ((flags & std::ios_base::oct) != 0)
  {
    oss.flags(flags);
    oss << "0o"; // add prefix ourself
  }
  else
  {
    oss.flags(flags | std::ios_base::showbase);
  }

  // Hide msvc "conditional expression is constant" warning
  bool isChar = std::is_same<T, signed char>::value || std::is_same<T, unsigned char>::value;
  if (isChar)
  {
    oss << static_cast<int>(value); // ostream output [un]signed char as ASCII char instead of int
  }
  else
  {
    oss << value;
  }

  const std::string str{ oss.str() };

  T read{};
  auto count = vtkValueFromString(str.data(), str.data() + str.size(), read);
  Check(count != 0, "Parsing failed");
  Check(read == value, "Wrong value");
  Check(count == str.size(), "Input not entirely consumed");

  return true;
}

// Convert a value to binary representation and parse it to check if parsed value is equal to input
template <typename T>
static bool TestIntParseBinaryHelper(T value)
{
  using UnsignedType = typename std::make_unsigned<T>::type;

  std::bitset<sizeof(T) * CHAR_BIT> bitset{ static_cast<unsigned long long>(
    reinterpret_cast<UnsignedType&>(value)) };

  const std::string str{ "0b" + bitset.to_string() };

  T read{};
  auto count = vtkValueFromString(str.data(), str.data() + str.size(), read);
  Check(count != 0, "Parsing failed");
  Check(read == value, "Wrong value");
  Check(count == str.size(), "Input not entirely consumed");

  return true;
}

// Check a predefined set of values for each type
template <typename T, typename std::enable_if<std::is_signed<T>::value>::type* = nullptr>
static bool TestIntParseFor()
{
  constexpr std::array<T, 5> values = { { static_cast<T>(0), static_cast<T>(1), static_cast<T>(42),
    (std::numeric_limits<T>::max)(), (std::numeric_limits<T>::max)() - 1 } };

  for (T value : values)
  {
    Check(TestIntParseHelper(value, std::ios_base::dec), "Test failed");
    Check(TestIntParseHelper(value, std::ios_base::oct), "Test failed");
    Check(TestIntParseHelper(value, std::ios_base::hex), "Test failed");
    Check(TestIntParseBinaryHelper(value), "Test failed");
  }

  // Negative values only make sense in decimal
  Check(TestIntParseHelper(static_cast<T>(-1), std::ios_base::dec), "Test failed");
  Check(TestIntParseHelper(static_cast<T>(-42), std::ios_base::dec), "Test failed");
  Check(TestIntParseHelper((std::numeric_limits<T>::min)(), std::ios_base::dec), "Test failed");
  Check(TestIntParseHelper((std::numeric_limits<T>::min)() + 1, std::ios_base::dec), "Test failed");

  return true;
}

// Check a predefined set of values for each type
template <typename T, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
static bool TestIntParseFor()
{
  constexpr std::array<T, 5> values = { { static_cast<T>(0), static_cast<T>(1), static_cast<T>(42),
    (std::numeric_limits<T>::max)(), (std::numeric_limits<T>::max)() - 1 } };

  for (T value : values)
  {
    Check(TestIntParseHelper(value, std::ios_base::dec), "Test failed");
    Check(TestIntParseHelper(value, std::ios_base::oct), "Test failed");
    Check(TestIntParseHelper(value, std::ios_base::hex), "Test failed");
    Check(TestIntParseBinaryHelper(value), "Test failed");
  }

  return true;
}

// Convert a value to hex, oct or dec representation and parse it to check if parsed value is equal
// to input. HelperType enable to check under/overflow using a larger type.
template <typename RealType, typename HelperType>
static bool TestIntParseLimitHelper(HelperType value, std::ios_base::fmtflags flags)
{
  std::ostringstream oss{};
  oss.setf(flags | std::ios_base::showbase);
  oss << value;

  const std::string str{ oss.str() };

  RealType read{};
  auto count = vtkValueFromString(str.data(), str.data() + str.size(), read);
  Check(count == 0, "Parsing must fail on overflow/underflow");
  Check(read == 0, "Output variable must not change on failure");

  return true;
}

// Convert a value to binary representation and parse it to check if parsed value is equal to input.
// HelperType enable to check under/overflow using a larger type.
template <typename RealType, typename HelperType>
static bool TestIntParseLimitBinaryHelper(HelperType value)
{
  using UnsignedType = typename std::make_unsigned<HelperType>::type;

  std::bitset<sizeof(HelperType) * CHAR_BIT> bitset{ static_cast<unsigned long long>(
    reinterpret_cast<UnsignedType&>(value)) };

  const std::string str{ "0b" + bitset.to_string() };

  RealType read{};
  auto count = vtkValueFromString(str.data(), str.data() + str.size(), read);
  Check(count == 0, "Parsing must fail on overflow/underflow");
  Check(read == 0, "Output variable must not change on failure");

  return true;
}

// Try to parse any input string and check if the result is expected
template <typename T>
bool TestParseHelper(const std::string& str, bool expectedSuccess, T expectedValue)
{
  T value{};
  auto count = vtkValueFromString(str.data(), str.data() + str.size(), value);

  // Check that output value and pointer are coherent
  if (expectedSuccess)
  {
    Check(count != 0, "Expected success but parsing failed");
    Check(value == expectedValue, "Value don't match");
    Check(count == str.size(), "Range not entirely consumed");
  }
  else
  {
    Check(count == 0, "Expected failure but parsing succeeded");
    Check(value == T{}, "Output variable must not be modified in case of failure");
  }

  return true;
}

// Main int parsing test
static bool TestIntParse()
{
  Check(TestIntParseFor<signed char>(), "Test failed");
  Check(TestIntParseFor<unsigned char>(), "Test failed");
  Check(TestIntParseFor<signed short>(), "Test failed");
  Check(TestIntParseFor<unsigned short>(), "Test failed");
  Check(TestIntParseFor<signed int>(), "Test failed");
  Check(TestIntParseFor<unsigned int>(), "Test failed");
  Check(TestIntParseFor<signed long>(), "Test failed");
  Check(TestIntParseFor<unsigned long>(), "Test failed");
  Check(TestIntParseFor<signed long long>(), "Test failed");
  Check(TestIntParseFor<unsigned long long>(), "Test failed");

  // Check on 16-bits integers, assume it does work for any size < intmax_t
  Check(TestIntParseLimitHelper<std::int16_t>(32768, std::ios_base::dec), "Test failed");
  Check(TestIntParseLimitHelper<std::int16_t>(-32769, std::ios_base::dec), "Test failed");
  Check(TestIntParseLimitHelper<std::int16_t>(65536, std::ios_base::oct), "Test failed");
  Check(TestIntParseLimitHelper<std::int16_t>(65536, std::ios_base::hex), "Test failed");
  Check(TestIntParseLimitBinaryHelper<std::int16_t>(65536), "Test failed");

  // Check intmax_t (long long in practice)
  Check(TestParseHelper("9223372036854775808", false, 0ll), "Test failed");
  Check(TestParseHelper("-9223372036854775809", false, 0ll), "Test failed");
  Check(TestParseHelper("0x10000000000000000", false, 0ll), "Test failed");
  Check(TestParseHelper("0o2000000000000000000000", false, 0ll), "Test failed");
  Check(TestParseHelper(
          "10000000000000000000000000000000000000000000000000000000000000000", false, 0ll),
    "Test failed");

  return true;
}

// Main float parsing test
static bool TestFloatParse()
{
  // Since low level float parsing is handled by fast_float, this is a basic sanity check

  { // Check valid input
    float f{};
    std::string str{ "-3.14e2" };
    auto count = vtkValueFromString(str.data(), str.data() + str.size(), f);

    Check(count != 0, "Parsing failed");
    Check(-3.140001e2 <= f && f <= -3.13999e2, "Wrong value, expected -314.0 got " << f);
    Check(count == str.size(), "Invalid output pointer");
  }

  { // Check invalid input
    float f{ -3.0f };
    std::string str{ "abc -3.14e2" };
    auto count = vtkValueFromString(str.data(), str.data() + str.size(), f);

    Check(count == 0, "Parsing must fail");
    Check(-3.00001 <= f && f <= -2.99999, "Input must not be modified");
  }

  return true;
}

// Main bool parsing test
static bool TestBoolParse()
{
  Check(TestParseHelper("true", true, true), "Test failed");
  Check(TestParseHelper("1", true, true), "Test failed");
  Check(TestParseHelper("false", true, false), "Test failed");
  Check(TestParseHelper("0", true, false), "Test failed");
  Check(TestParseHelper("False", true, false), "Test failed");
  Check(TestParseHelper("True", true, true), "Test failed");
  Check(TestParseHelper("2", false, false), "Must fail for 2");
  Check(TestParseHelper("-1", false, false), "Must fail for -1");
  Check(TestParseHelper(" 1", false, false), "Must not trim string");
  Check(TestParseHelper("TRUE", false, false), "Must be case sensitive");
  Check(TestParseHelper("FalSe", false, false), "Must be case sensitive");
  Check(TestParseHelper(" true", false, false), "Must not trim string");
  Check(TestParseHelper(" 1", false, false), "Must not trim string");
  Check(TestParseHelper(" false", false, false), "Must not trim string");
  Check(TestParseHelper(" 0", false, false), "Must not trim string");

  return true;
}

int TestValueFromString(int, char*[])
{
  if (!TestIntParse())
  {
    return EXIT_FAILURE;
  }

  if (!TestFloatParse())
  {
    return EXIT_FAILURE;
  }

  if (!TestBoolParse())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
