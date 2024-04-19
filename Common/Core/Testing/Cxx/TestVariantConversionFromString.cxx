// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkVariant.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>

namespace
{

#define Check(expr, message)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cout << __FILE__ << " L." << __LINE__ << " | " << #expr << " failed: \n"                \
                << message << std::endl;                                                           \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

template <typename T>
T ToNumeric(const vtkVariant& v, bool* valid = nullptr);

#define DefineToNumeric(type, name)                                                                \
  template <>                                                                                      \
  type ToNumeric<type>(const vtkVariant& v, bool* valid)                                           \
  {                                                                                                \
    return v.To##name(valid);                                                                      \
  }

DefineToNumeric(short, Short);
DefineToNumeric(unsigned short, UnsignedShort);
DefineToNumeric(int, Int);
DefineToNumeric(unsigned int, UnsignedInt);
DefineToNumeric(long, Long);
DefineToNumeric(unsigned long, UnsignedLong);
DefineToNumeric(long long, LongLong);
DefineToNumeric(unsigned long long, UnsignedLongLong);
DefineToNumeric(float, Float);
DefineToNumeric(double, Double);

#undef DefineToNumeric

template <typename T, typename std::enable_if<std::is_signed<T>::value>::type* = nullptr>
std::array<T, 7> IntegersValues() noexcept
{
  return std::array<T, 7>{ 0, 1, 42, -1, -42, std::numeric_limits<T>::min(),
    std::numeric_limits<T>::max() };
}

template <typename T, typename std::enable_if<!std::is_signed<T>::value>::type* = nullptr>
std::array<T, 4> IntegersValues() noexcept
{
  return std::array<T, 4>{ 0, 1, 42, std::numeric_limits<T>::max() };
}

template <typename T>
bool CheckIntConversionHelper()
{
  bool valid = false;

  for (auto value : IntegersValues<T>())
  {
    vtkVariant variant = std::to_string(value).c_str();
    Check(ToNumeric<T>(variant, &valid) == value && valid, "Conversion failed");
  }

  return true;
}

bool CheckIntConversion()
{
  Check(CheckIntConversionHelper<short>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<unsigned short>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<int>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<unsigned int>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<long>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<unsigned long>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<long long>(), "Failed to perform integer conversion");
  Check(CheckIntConversionHelper<unsigned long long>(), "Failed to perform integer conversion");

  return true;
}

template <typename T>
bool AlmostEqual(T value, T expected, T tolerance = std::numeric_limits<T>::epsilon())
{
  return expected - tolerance < value && value < expected + tolerance;
}

template <typename T>
bool CheckFloatConversionHelper()
{
  bool valid = false;

  Check(ToNumeric<T>("0.0", &valid) == static_cast<T>(0.0) && valid, "Conversion failed");
  Check(ToNumeric<T>("-1.0", &valid) == static_cast<T>(-1.0) && valid, "Conversion failed");
  Check(ToNumeric<T>("1.0", &valid) == static_cast<T>(1.0) && valid, "Conversion failed");
  Check(
    AlmostEqual(ToNumeric<T>("25.12", &valid), static_cast<T>(25.12), static_cast<T>(0.00001)) &&
      valid,
    "Conversion failed");
  Check(AlmostEqual(
          ToNumeric<T>("-62354.2812", &valid), static_cast<T>(-62354.2812), static_cast<T>(0.01)) &&
      valid,
    "Conversion failed");

  return true;
}

bool CheckFloatConversion()
{
  Check(CheckFloatConversionHelper<float>(), "Checks failed for float");
  Check(CheckFloatConversionHelper<double>(), "Checks failed for double");

  return true;
}

bool CheckCharConversion()
{
  bool valid = false;

  vtkVariant c = "V";
  Check(c.ToChar(&valid) == 'V' && valid, "ToChar must return the first non whitespace char");
  Check(c.ToSignedChar(&valid) == static_cast<signed char>('V') && valid,
    "ToSignedChar must return the first non whitespace char");
  Check(c.ToUnsignedChar(&valid) == static_cast<unsigned char>('V') && valid,
    "ToUnsignedChar must return the first non whitespace char");

  return true;
}

bool CheckNonFiniteConversion()
{
  bool valid = false;

  vtkVariant nan = "Nan";
  Check(std::isnan(nan.ToDouble(&valid)) && valid, "Failed to convert Nan to double");
  Check(std::isnan(nan.ToFloat(&valid)) && valid, "Failed to convert Nan to float");

  vtkVariant inf = "InF";
  Check(std::isinf(inf.ToDouble(&valid)) && valid, "Failed to parse Inf to double");
  Check(inf.ToDouble(&valid) > 0 && valid, "Inf must be positive");
  Check(std::isinf(inf.ToFloat(&valid)) && valid, "Failed to parse Inf to float");
  Check(inf.ToFloat(&valid) > 0 && valid, "Inf must be positive");
  Check(inf.ToInt(&valid) == 0 && !valid, "Inf can not be converted to int");

  vtkVariant ninf = "-InF";
  Check(std::isinf(ninf.ToDouble(&valid)) && valid, "Failed to parse -Inf to double");
  Check(ninf.ToDouble(&valid) < 0 && valid, "-Inf must be negative");
  Check(std::isinf(ninf.ToFloat(&valid)) && valid, "Failed to parse -Inf to float");
  Check(ninf.ToFloat(&valid) < 0 && valid, "-Inf must be negative");
  Check(ninf.ToInt(&valid) == 0 && !valid, "-Inf can not be converted to int");

  vtkVariant word = "Hello";
  word.ToDouble(&valid);
  Check(!valid, "\"Hello\" can not be converted to double");
  word.ToFloat(&valid);
  Check(!valid, "\"Hello\" can not be converted to float");
  word.ToInt(&valid);
  Check(!valid, "\"Hello\" can not be converted to int");

  return true;
}

bool CheckTrimming()
{
  bool valid = false;

  vtkVariant c = "\r\t\n\f\v V\r\t\n\f\v ";
  Check(c.ToChar(&valid) == 'V' && valid, "Trimming not performed for chars");

  vtkVariant integer = "\r\t\n\f\v 42\r\t\n\f\v ";
  Check(integer.ToInt(&valid) == 42 && valid, "Trimming not performed for integers");
  vtkVariant floating = "\r\t\n\f\v 42.3\r\t\n\f\v ";
  const auto floating_value = floating.ToDouble(&valid);
  Check(
    43.01 > floating_value && floating_value > 42.29 && valid, "Trimming not performed for floats");

  vtkVariant nan = "\r\t\n\f\v nan\r\t\n\f\v ";
  Check(std::isnan(nan.ToFloat(&valid)) && valid, "Trimming not performed for non-finite floats");

  vtkVariant cMultiple = "\r\t\n\f\v V\r\t\n\f\v tk \r\t\n\f\v";
  cMultiple.ToChar(&valid);
  Check(!valid, "Must fail because string contains multiple values");

  vtkVariant intMultiple = "\r\t\n\f\v 42\r\t\n\f\v 12 \r\t\n\f\v";
  intMultiple.ToInt(&valid);
  Check(!valid, "Must fail because string contains multiple values");
  intMultiple.ToFloat(&valid);
  Check(!valid, "Must fail because string contains multiple values");

  vtkVariant nonfiniteMultiple = "\r\t\n\f\v nan\r\t\n\f\v 12 \r\t\n\f\v";
  nonfiniteMultiple.ToInt(&valid);
  Check(!valid, "Must fail because string contains multiple values");

  return true;
}

}

int TestVariantConversionFromString(int, char*[])
{
  if (!CheckIntConversion())
  {
    return EXIT_FAILURE;
  }

  if (!CheckFloatConversion())
  {
    return EXIT_FAILURE;
  }

  if (!CheckCharConversion())
  {
    return EXIT_FAILURE;
  }

  if (!CheckNonFiniteConversion())
  {
    return EXIT_FAILURE;
  }

  if (!CheckTrimming())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
