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

#include "ArrayConverters.h"

#include <vtkm/cont/ArrayHandleGroupVecVariable.h>

#include "vtkDataArray.h"
#include "vtkDataObject.h"

namespace tovtkm
{

template <typename DataArrayType>
vtkm::cont::VariantArrayHandle vtkDataArrayToVariantArrayHandle(DataArrayType* input)
{
  int numComps = input->GetNumberOfComponents();
  switch (numComps)
  {
    case 1:
      return vtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 1>::Wrap(input));
    case 2:
      return vtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 2>::Wrap(input));
    case 3:
      return vtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 3>::Wrap(input));
    case 4:
      return vtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 4>::Wrap(input));
    case 6:
      return vtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 6>::Wrap(input));
    case 9:
      return vtkm::cont::VariantArrayHandle(DataArrayToArrayHandle<DataArrayType, 9>::Wrap(input));
    default:
    {
      vtkm::Id numTuples = input->GetNumberOfTuples();
      auto subHandle = DataArrayToArrayHandle<DataArrayType, 1>::Wrap(input);
      auto offsets =
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(vtkm::Id(0), vtkm::Id(numComps), numTuples);
      auto handle = vtkm::cont::make_ArrayHandleGroupVecVariable(subHandle, offsets);
      return vtkm::cont::VariantArrayHandle(handle);
    }
  }
}

template <typename DataArrayType>
vtkm::cont::Field ConvertPointField(DataArrayType* input)
{
  auto vhandle = vtkDataArrayToVariantArrayHandle(input);
  return vtkm::cont::make_FieldPoint(input->GetName(), vhandle);
}

template <typename DataArrayType>
vtkm::cont::Field ConvertCellField(DataArrayType* input)
{
  auto vhandle = vtkDataArrayToVariantArrayHandle(input);
  return vtkm::cont::make_FieldCell(input->GetName(), vhandle);
}

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
  extern template vtkm::cont::Field Convert<ArrayType<ValueType> >(                                \
    ArrayType<ValueType> * input, int association);
#else
#define VTK_EXPORT_ARRAY_CONVERSION_TO_VTKM_DETAIL(ArrayType, ValueType)                           \
  template vtkm::cont::Field Convert<ArrayType<ValueType> >(                                       \
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
