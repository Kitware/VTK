// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2017 Elias Kosunen
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
#include "vtkValueFromString.h"

#include <array>
#include <cstring>
#include <limits>
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
// clang-format off
static const std::array<unsigned char, 256> DigitsLUT =
{{
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,   255, 255, 255, 255, 255, 255,
  255, 10,  11,  12,  13,  14,  15,  16,
  17,  18,  19,  20,  21,  22,  23,  24,
  25,  26,  27,  28,  29,  30,  31,  32,
  33,  34,  35,  255, 255, 255, 255, 255,
  255, 10,  11,  12,  13,  14,  15,  16,
  17,  18,  19,  20,  21,  22,  23,  24,
  25,  26,  27,  28,  29,  30,  31,  32,
  33,  34,  35,  255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255,
}};
// clang-format on

static unsigned char CharToInt(char ch) noexcept
{
  return DigitsLUT[static_cast<unsigned char>(ch)];
}

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

template <typename T>
const char* ParseInt(const char* it, const char* end, bool minus_sign, int base, T& val) noexcept
{
  using UnsignedType = typename std::make_unsigned<T>::type;
  using SignedType = typename std::make_signed<T>::type;

  constexpr UnsignedType umax = std::numeric_limits<UnsignedType>::max();
  constexpr UnsignedType imax = static_cast<UnsignedType>(std::numeric_limits<SignedType>::max());
  constexpr UnsignedType absimin = static_cast<UnsignedType>(1)
    << ((sizeof(UnsignedType) * CHAR_BIT) - 1);

  const auto limit = [=]() {
    if (std::is_signed<T>::value)
    {
      if (minus_sign)
      {
        return absimin;
      }

      return imax;
    }

    return umax;
  }();

  const auto ubase = static_cast<UnsignedType>(base);
  const auto cutoff = limit / ubase;
  const auto cutlim = limit % ubase;

  UnsignedType tmp{};
  while (it != end)
  {
    const auto digit = CharToInt(*it);
    if (digit >= ubase)
    {
      break;
    }

    if (tmp > cutoff || (tmp == cutoff && digit > cutlim))
    {
      return nullptr;
    }

    tmp *= ubase;
    tmp += digit;
    ++it;
  }

  // Hide msvc "conditional expression is constant" warning
  bool isSigned = std::is_signed<T>::value;
  if (isSigned && minus_sign)
  {
    if (tmp == absimin)
    {
      val = static_cast<T>(std::numeric_limits<SignedType>::min());
    }
    else
    {
      val = static_cast<T>(-static_cast<SignedType>(tmp));
    }
  }
  else
  {
    val = static_cast<T>(tmp);
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

  // Hide msvc "conditional expression is constant" warning
  bool isUnsigned = std::is_unsigned<T>::value;
  // Unsigned can't be negative
  if (isUnsigned && *it == '-')
  {
    return 0;
  }

  bool minus_sign{};
  if (*it == '-')
  {
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

  int base{};
  it = Impl::DetectBase(it, end, base);

  if (base == 0)
  {
    output = 0;
    return static_cast<std::size_t>(std::distance(begin, it));
  }

  if (base != 10 && minus_sign)
  {
    return 0;
  }

  // Parse non decimal number as unsigned ints (c.f. doc)
  if (base != 10 && std::is_signed<T>::value)
  {
    using UnsignedType = typename std::make_unsigned<T>::type;

    std::uintmax_t tmp{}; // parse it as an unsigned int (intmax to support all types)
    auto ptr = Impl::ParseInt(it, end, minus_sign, base, tmp);
    if (!ptr || it == ptr)
    {
      return 0;
    }

    constexpr auto umax = (std::numeric_limits<UnsignedType>::max)();
    constexpr auto fitMask = ~static_cast<std::uintmax_t>(umax);
    constexpr auto cutMask = static_cast<std::uintmax_t>(umax);

    // Check if number can actually fit, i.e all bits leftmost bit are either all 0 or all 1.
    if ((tmp & fitMask) == fitMask || (tmp & fitMask) == static_cast<std::uintmax_t>(0))
    {
      const auto realValue = static_cast<UnsignedType>(tmp & cutMask);
      output = reinterpret_cast<const T&>(realValue);

      return static_cast<std::size_t>(std::distance(begin, ptr));
    }

    return 0;
  }

  // parse the actual number
  T tmp;
  auto ptr = Impl::ParseInt(it, end, minus_sign, base, tmp);
  if (!ptr || it == ptr)
  {
    return 0;
  }

  output = tmp;

  return static_cast<std::size_t>(std::distance(begin, ptr));
}

// Overload for floats
template <typename T,
  typename std::enable_if<std::is_floating_point<T>::value && !std::is_same<T, long double>::value,
    bool>::type = true>
std::size_t FromStringInternal(const char* begin, const char* end, T& output) noexcept
{
  const auto result = fast_float::from_chars(begin, end, output);
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

  // check interger 0 and 1
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

#undef INSTANCIATE_FROMSTRING_EXTERN_TEMPLATE

VTK_ABI_NAMESPACE_END
