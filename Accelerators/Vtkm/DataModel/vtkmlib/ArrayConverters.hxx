// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_ArrayConverters_hxx
#define vtkmlib_ArrayConverters_hxx

#include "ArrayConverters.h"
#include "vtkmlib/DataArrayConverters.hxx"

#include <viskores/cont/ArrayHandleGroupVecVariable.h>

#include "vtkDataArray.h"
#include "vtkDataObject.h"

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

template <typename DataArrayType>
viskores::cont::Field Convert(DataArrayType* input, int association)
{
  // we need to switch on if we are a cell or point field first!
  // The problem is that the constructor signature for fields differ based
  // on if they are a cell or point field.
  if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    return ConvertPointField(input);
  }
  else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    return ConvertCellField(input);
  }

  return viskores::cont::Field();
}

#if !defined(vtkmlib_ArrayConverterExport_cxx)
#define VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, ValueType)                           \
  extern template viskores::cont::Field Convert<ArrayType<ValueType>>(                             \
    ArrayType<ValueType> * input, int association);
#else
#define VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, ValueType)                           \
  template viskores::cont::Field Convert<ArrayType<ValueType>>(                                    \
    ArrayType<ValueType> * input, int association);
#endif

#define VTK_EXPORT_SIGNED_ARRAY_CONVERSION_TO_VTKM(ArrayType)                                      \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, char)                                      \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, int)                                       \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, long)                                      \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, long long)                                 \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, short)                                     \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, signed char)

#define VTK_EXPORT_UNSIGNED_ARRAY_CONVERSION_TO_VTKM(ArrayType)                                    \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, unsigned char)                             \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, unsigned int)                              \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, unsigned long)                             \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, unsigned long long)                        \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, unsigned short)

#define VTK_EXPORT_REAL_ARRAY_CONVERSION_TO_VTKM(ArrayType)                                        \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, double)                                    \
  VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, float)

#if !defined(vtkmlib_ArrayConverterExport_cxx)
#define VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM(ArrayType)                                             \
  VTK_EXPORT_SIGNED_ARRAY_CONVERSION_TO_VTKM(ArrayType)                                            \
  VTK_EXPORT_UNSIGNED_ARRAY_CONVERSION_TO_VTKM(ArrayType)                                          \
  VTK_EXPORT_REAL_ARRAY_CONVERSION_TO_VTKM(ArrayType)

VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM(vtkAOSDataArrayTemplate)
VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM(vtkSOADataArrayTemplate)

#undef VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM
#undef VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL

#endif // !defined(ArrayConverterExport_cxx)

VTK_ABI_NAMESPACE_END
} // tovtkm
#endif
