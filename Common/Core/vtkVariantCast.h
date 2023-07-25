// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkVariantCast
 *
 * Converts a vtkVariant to some other type.  Wherever possible, implicit conversions are
 * performed, so this method can be used to convert from nearly any type to a string, or
 * from a string to nearly any type.  Note that some conversions may fail at runtime, such
 * as a conversion from the string "abc" to a numeric type.
 *
 * The optional 'valid' flag can be used by callers to verify whether conversion succeeded.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkVariantCast_h
#define vtkVariantCast_h

#include "vtkVariant.h"
#include <typeinfo> // for warnings

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
T vtkVariantCast(const vtkVariant& value, bool* valid = nullptr)
{
  vtkGenericWarningMacro(<< "Cannot convert vtkVariant containing [" << value.GetTypeAsString()
                         << "] "
                         << "to unsupported type [" << typeid(T).name() << "].  "
                         << "Create a vtkVariantCast<> specialization to eliminate this warning.");

  if (valid)
    *valid = false;

  static T dummy;
  return dummy;
}

template <>
inline char vtkVariantCast<char>(const vtkVariant& value, bool* valid)
{
  return value.ToChar(valid);
}

template <>
inline signed char vtkVariantCast<signed char>(const vtkVariant& value, bool* valid)
{
  return value.ToSignedChar(valid);
}

template <>
inline unsigned char vtkVariantCast<unsigned char>(const vtkVariant& value, bool* valid)
{
  return value.ToUnsignedChar(valid);
}

template <>
inline short vtkVariantCast<short>(const vtkVariant& value, bool* valid)
{
  return value.ToShort(valid);
}

template <>
inline unsigned short vtkVariantCast<unsigned short>(const vtkVariant& value, bool* valid)
{
  return value.ToUnsignedShort(valid);
}

template <>
inline int vtkVariantCast<int>(const vtkVariant& value, bool* valid)
{
  return value.ToInt(valid);
}

template <>
inline unsigned int vtkVariantCast<unsigned int>(const vtkVariant& value, bool* valid)
{
  return value.ToUnsignedInt(valid);
}

template <>
inline long vtkVariantCast<long>(const vtkVariant& value, bool* valid)
{
  return value.ToLong(valid);
}

template <>
inline unsigned long vtkVariantCast<unsigned long>(const vtkVariant& value, bool* valid)
{
  return value.ToUnsignedLong(valid);
}

template <>
inline long long vtkVariantCast<long long>(const vtkVariant& value, bool* valid)
{
  return value.ToLongLong(valid);
}

template <>
inline unsigned long long vtkVariantCast<unsigned long long>(const vtkVariant& value, bool* valid)
{
  return value.ToUnsignedLongLong(valid);
}

template <>
inline float vtkVariantCast<float>(const vtkVariant& value, bool* valid)
{
  return value.ToFloat(valid);
}

template <>
inline double vtkVariantCast<double>(const vtkVariant& value, bool* valid)
{
  return value.ToDouble(valid);
}

template <>
inline vtkStdString vtkVariantCast<vtkStdString>(const vtkVariant& value, bool* valid)
{
  if (valid)
    *valid = true;

  return value.ToString();
}

template <>
inline vtkVariant vtkVariantCast<vtkVariant>(const vtkVariant& value, bool* valid)
{
  if (valid)
    *valid = true;

  return value;
}

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkVariantCast.h
