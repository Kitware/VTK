// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkValueFromString_h
#define vtkValueFromString_h

#include "vtkCommonCoreModule.h"

#include "vtkWrappingHints.h" // for VTK_WRAPEXCLUDE

#include <cstdlib> // for std::size_t

VTK_ABI_NAMESPACE_BEGIN

/**
 * @brief Low-level function to convert a string to ints, floats or bools
 *
 * This function is the low-level high-performance parsing function.
 * This function does not trim any data from input before parsing.
 *
 * Supported types are `signed char`, `unsigned char`, `short`, `unsigned short`, `int`,
 * `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`,
 * `bool`.
 *
 * ---
 * `signed char`, `unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`,
 * `unsigned long`, `long long` and `unsigned long long` parsing support decimal, hexadecimal
 * (`0x{value}` or `0X{value}`), octal (`0o{value}` or `0O{value}`) and binary
 * (`0b{value}` or `0B{value}`) integers.
 * Overflow will return an error.
 *
 * Limitations:
 * - When parsing a hexadecimal, octal or binary number, if a leading `-` is parsed, it will return
 * an error. When parsing a signed type, hexadecimal, octal or binary number parsing uses the
 * unsigned variant of the type, then reinterpreted to the signed type. For example parsing `"0xFF"`
 * to a `int8_t` will parse -1.
 * - Octal old format (`0{value}`) is **not** supported.
 * - When parsing a unsigned type, if a leading `-` is parsed, it will return an error.
 * - Leading `+` is not supported and will return an error.
 *
 * ---
 * `float`, `double` parsing always uses '.' as separator of the integer and decimal part of a
 * number. Scientific format is supported, both `e` and `E` are supported. `nan` and `[-]inf` are
 * supported and case insensitive.
 *
 * Limitations:
 * - Leading `+` is not supported and will return an error.
 *
 * ---
 * `bool` parsing supports the following syntaxes: "0", "1", "false", "False", "true" and "True".
 *
 * @param begin Begin of the range to convert
 * @param end End of the range to convert
 * @param output Variable to write output to. If parsing failed, output is left unmodified.
 *
 * @return The number of consumed characters.
 * 0 is returned to indicate failure, or empty range.
 * If 0 is returned, output is not modified, otherwise it contains the parsed value.
 */
template <typename T>
VTK_WRAPEXCLUDE std::size_t vtkValueFromString(
  const char* begin, const char* end, T& output) noexcept;

#define DECLARE_FROMSTRING_EXTERN_TEMPLATE(type)                                                   \
  extern template VTKCOMMONCORE_EXPORT std::size_t vtkValueFromString<type>(                       \
    const char* begin, const char* end, type&) noexcept

// Declare explicit instantiation for all supported types
DECLARE_FROMSTRING_EXTERN_TEMPLATE(signed char);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(unsigned char);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(short);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(unsigned short);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(int);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(unsigned int);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(long);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(unsigned long);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(long long);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(unsigned long long);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(float);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(double);
DECLARE_FROMSTRING_EXTERN_TEMPLATE(bool);

#undef DECLARE_FROMSTRING_EXTERN_TEMPLATE

VTK_ABI_NAMESPACE_END

#endif
