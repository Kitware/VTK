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

#ifndef vtkmlib_DataArrayConverters_hxx
#define vtkmlib_DataArrayConverters_hxx

#include "DataArrayConverters.h"

#include <vtkm/cont/ArrayHandleGroupVecVariable.h>

#include "vtkDataArray.h"

namespace tovtkm
{

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

} // tovtkm
#endif
