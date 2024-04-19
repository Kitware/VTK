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
