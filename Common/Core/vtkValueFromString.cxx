// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2017 Elias Kosunen
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
#include "vtkValueFromString.h"

#include <cstring>
#include <type_traits>

#include <vtkfast_float.h>

/**
 * @brief Implementation namespace for integer parser details
 *
 * This integer parsing algorithm was inspired by scnlib.
 * https://github.com/eliaskosunen/scnlib
 */

VTK_ABI_NAMESPACE_BEGIN
namespace Impl
{
static const char* DetectBase(const char* it, const char* end, int& base) noexcept
{
  // If base can be detected, it should start with '0'
  if (*it == '0')
  {
    // if we reach end now, input is "0"
    ++it;
    if (it == end)
    {
      base = 0;
      return it;
    }

    if (*it == 'x' || *it == 'X') // Check hex (format: 0x{value})
    {
      ++it;
      if (it == end) // "0x"/"0X" is not a valid number
      {
        base = 0;
        return --it;
      }

      base = 16;
    }
    else if (*it == 'b' || *it == 'B') // Check binary (format: 0b{value})
    {
      ++it;
      if (it == end) // "0b"/"0B" not a valid number
      {
        base = 0;
        return --it;
      }

      base = 2;
    }
    else if (*it == 'o' || *it == 'O') // Check octal (format: 0o{value})
    {
      ++it;
      if (it == end) // "0o"/"0O" not a valid number,
      {
        base = 0;
        return --it;
      }

      base = 8;
    }
    else
    {
      base = 0; // Next character is not a digit
    }
  }
  else
  {
    base = 10;
  }

  return it;
}
}

// Overload for integers
template <typename T,
  typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, bool>::type =
    true>
std::size_t FromStringInternal(const char* begin, const char* end, T& output) noexcept
{
  if (begin == end)
  {
    return 0;
  }

  auto it = begin;

  // Handle sign
  bool minus_sign = false;
  if (*it == '-')
  {
    if (std::is_unsigned_v<T>) // Unsigned can't be negative
    {
      return 0;
    }
    minus_sign = true;
    ++it;
  }
  else if (*it == '+')
  {
    ++it;
  }

  if (it == end) // "-" is not a valid number
  {
    return 0;
  }

  int base = 0;
  it = Impl::DetectBase(it, end, base);

  if (base == 0)
  {
    output = 0;
    return static_cast<std::size_t>(std::distance(begin, it));
  }
  if (base != 10 && minus_sign) // Negative sign is not allowed for non-decimal
  {
    return 0;
  }
  fast_float::parse_options_t<char> options;
  options.base = base;
  options.format |= fast_float::chars_format::allow_leading_plus;
  // Use base 10 parsing for the full string including sign, otherwise start from after prefix
  const char* parse_start = (base == 10) ? begin : it;
  const auto result = fast_float::from_chars_int_advanced(parse_start, end, output, options);
  if (result.ec != std::errc{})
  {
    return 0;
  }

  return static_cast<std::size_t>(std::distance(begin, result.ptr));
}

// Overload for floats
template <typename T,
  typename std::enable_if<std::is_floating_point<T>::value && !std::is_same<T, long double>::value,
    bool>::type = true>
std::size_t FromStringInternal(const char* begin, const char* end, T& output) noexcept
{
  static constexpr fast_float::parse_options_t<char> options(fast_float::chars_format::general);
  const auto result = fast_float::from_chars_float_advanced(begin, end, output, options);
  if (result.ec != std::errc{})
  {
    return 0;
  }

  return static_cast<std::size_t>(std::distance(begin, result.ptr));
}

// overload for bool
template <typename T, typename std::enable_if<std::is_same<T, bool>::value, bool>::type = true>
std::size_t FromStringInternal(const char* begin, const char* end, T& output) noexcept
{
  const auto size = static_cast<std::size_t>(std::distance(begin, end));

  if (size == 0)
  {
    return 0;
  }

  // check integer 0 and 1
  if (*begin == '0')
  {
    output = false;
    return 1;
  }
  else if (*begin == '1')
  {
    output = true;
    return 1;
  }

  // check true
  if (size < 4)
  {
    return 0;
  }

  // "True" or "true"
  if ((*begin == 'T' || *begin == 't') && std::strncmp("rue", begin + 1, 3) == 0)
  {
    output = true;
    return 4;
  }

  // check false
  if (size < 5)
  {
    return 0;
  }

  // "False" or "false"
  if ((*begin == 'F' || *begin == 'f') && std::strncmp("alse", begin + 1, 4) == 0)
  {
    output = false;
    return 5;
  }

  // No match
  return 0;
}

template <typename T>
std::size_t vtkValueFromString(const char* begin, const char* end, T& output) noexcept
{
  return FromStringInternal(begin, end, output);
}

//------------------------------------------------------------------------------
// explicit instantiation for all supported types
#define INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(type)                                               \
  template std::size_t vtkValueFromString<type>(const char* begin, const char* end, type&) noexcept

// Declare explicit instantiation for all supported types
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(signed char);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(unsigned char);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(short);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(unsigned short);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(int);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(unsigned int);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(long);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(unsigned long);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(long long);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(unsigned long long);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(float);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(double);
INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE(bool);

#undef INSTANTIATE_FROMSTRING_EXTERN_TEMPLATE

VTK_ABI_NAMESPACE_END
