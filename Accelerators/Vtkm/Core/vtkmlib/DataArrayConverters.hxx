// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_DataArrayConverters_hxx
#define vtkmlib_DataArrayConverters_hxx

#include "DataArrayConverters.h"

#include <vtkm/cont/ArrayHandleGroupVecVariable.h>

#include "vtkDataArray.h"

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

template <typename DataArrayType>
vtkm::cont::UnknownArrayHandle vtkDataArrayToUnknownArrayHandle(DataArrayType* input)
{
  int numComps = input->GetNumberOfComponents();
  switch (numComps)
  {
    case 1:
      return vtkm::cont::UnknownArrayHandle(DataArrayToArrayHandle<DataArrayType, 1>::Wrap(input));
    case 2:
      return vtkm::cont::UnknownArrayHandle(DataArrayToArrayHandle<DataArrayType, 2>::Wrap(input));
    case 3:
      return vtkm::cont::UnknownArrayHandle(DataArrayToArrayHandle<DataArrayType, 3>::Wrap(input));
    case 4:
      return vtkm::cont::UnknownArrayHandle(DataArrayToArrayHandle<DataArrayType, 4>::Wrap(input));
    case 6:
      return vtkm::cont::UnknownArrayHandle(DataArrayToArrayHandle<DataArrayType, 6>::Wrap(input));
    case 9:
      return vtkm::cont::UnknownArrayHandle(DataArrayToArrayHandle<DataArrayType, 9>::Wrap(input));
    default:
    {
      vtkm::Id numTuples = input->GetNumberOfTuples();
      auto subHandle = DataArrayToArrayHandle<DataArrayType, 1>::Wrap(input);
      auto offsets =
        vtkm::cont::ArrayHandleCounting<vtkm::Id>(vtkm::Id(0), vtkm::Id(numComps), numTuples);
      auto handle = vtkm::cont::make_ArrayHandleGroupVecVariable(subHandle, offsets);
      return vtkm::cont::UnknownArrayHandle(handle);
    }
  }
}

template <typename DataArrayType>
vtkm::cont::Field ConvertPointField(DataArrayType* input)
{
  const char* name = input->GetName();
  if (!name || name[0] == '\0')
  {
    name = NoNameVTKFieldName();
  }

  auto vhandle = vtkDataArrayToUnknownArrayHandle(input);
  return vtkm::cont::make_FieldPoint(name, vhandle);
}

template <typename DataArrayType>
vtkm::cont::Field ConvertCellField(DataArrayType* input)
{
  const char* name = input->GetName();
  if (!name || name[0] == '\0')
  {
    name = NoNameVTKFieldName();
  }

  auto vhandle = vtkDataArrayToUnknownArrayHandle(input);
  return vtkm::cont::make_FieldCell(name, vhandle);
}

VTK_ABI_NAMESPACE_END
} // tovtkm
#endif
