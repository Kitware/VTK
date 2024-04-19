// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkVariantExtract
 *
 * Performs an explicit conversion from a vtkVariant to the type that it contains.  Implicit
 * conversions are *not* performed, so casting a vtkVariant containing one type (e.g. double)
 * to a different type (e.g. string) will not convert between types.
 *
 * Callers should use the 'valid' flag to verify whether the extraction succeeded.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkVariantExtract_h
#define vtkVariantExtract_h

#include "vtkVariant.h"
#include <typeinfo> // for typeid

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
T vtkVariantExtract(const vtkVariant& value, bool& valid)
{
  vtkGenericWarningMacro(
    << "Cannot convert vtkVariant containing [" << value.GetTypeAsString() << "] "
    << "to unsupported type [" << typeid(T).name() << "].  "
    << "Create a vtkVariantExtract<> specialization to eliminate this warning.");

  valid = false;

  static T dummy;
  return dummy;
}

template <>
inline char vtkVariantExtract<char>(const vtkVariant& value, bool& valid)
{
  valid = value.IsChar();
  return valid ? value.ToChar() : 0;
}

template <>
inline unsigned char vtkVariantExtract<unsigned char>(const vtkVariant& value, bool& valid)
{
  valid = value.IsUnsignedChar();
  return valid ? value.ToUnsignedChar() : 0;
}

template <>
inline short vtkVariantExtract<short>(const vtkVariant& value, bool& valid)
{
  valid = value.IsShort();
  return valid ? value.ToShort() : 0;
}

template <>
inline unsigned short vtkVariantExtract<unsigned short>(const vtkVariant& value, bool& valid)
{
  valid = value.IsUnsignedShort();
  return valid ? value.ToUnsignedShort() : 0;
}

template <>
inline int vtkVariantExtract<int>(const vtkVariant& value, bool& valid)
{
  valid = value.IsInt();
  return valid ? value.ToInt() : 0;
}

template <>
inline unsigned int vtkVariantExtract<unsigned int>(const vtkVariant& value, bool& valid)
{
  valid = value.IsUnsignedInt();
  return valid ? value.ToUnsignedInt() : 0;
}

template <>
inline long vtkVariantExtract<long>(const vtkVariant& value, bool& valid)
{
  valid = value.IsLong();
  return valid ? value.ToLong() : 0;
}

template <>
inline unsigned long vtkVariantExtract<unsigned long>(const vtkVariant& value, bool& valid)
{
  valid = value.IsUnsignedLong();
  return valid ? value.ToUnsignedLong() : 0;
}

template <>
inline long long vtkVariantExtract<long long>(const vtkVariant& value, bool& valid)
{
  valid = value.IsLongLong();
  return valid ? value.ToLongLong() : 0;
}

template <>
inline unsigned long long vtkVariantExtract<unsigned long long>(
  const vtkVariant& value, bool& valid)
{
  valid = value.IsUnsignedLongLong();
  return valid ? value.ToUnsignedLongLong() : 0;
}

template <>
inline float vtkVariantExtract<float>(const vtkVariant& value, bool& valid)
{
  valid = value.IsFloat();
  return valid ? value.ToFloat() : 0.0f;
}

template <>
inline double vtkVariantExtract<double>(const vtkVariant& value, bool& valid)
{
  valid = value.IsDouble();
  return valid ? value.ToDouble() : 0.0;
}

template <>
inline vtkStdString vtkVariantExtract<vtkStdString>(const vtkVariant& value, bool& valid)
{
  valid = value.IsString();
  return valid ? value.ToString() : vtkStdString();
}

template <>
inline vtkVariant vtkVariantExtract<vtkVariant>(const vtkVariant& value, bool& valid)
{
  valid = true;
  return value;
}

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkVariantExtract.h
