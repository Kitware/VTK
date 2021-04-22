//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkmlib_ArrayConverters_hxx
#define vtkmlib_ArrayConverters_hxx

#include "ArrayConverters.h"
#include "vtkmlib/DataArrayConverters.hxx"

#include <vtkm/cont/ArrayHandleGroupVecVariable.h>

#include "vtkDataArray.h"
#include "vtkDataObject.h"

namespace tovtkm
{

template <typename DataArrayType>
vtkm::cont::Field Convert(DataArrayType* input, int association)
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

  return vtkm::cont::Field();
}

#if !defined(vtkmlib_ArrayConverterExport_cxx)
#define VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, ValueType)                           \
  extern template vtkm::cont::Field Convert<ArrayType<ValueType>>(                                 \
    ArrayType<ValueType> * input, int association);
#else
#define VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, ValueType)                           \
  template vtkm::cont::Field Convert<ArrayType<ValueType>>(                                        \
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

} // tovtkm
#endif
