// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_UnstructuredGridConverter_h
#define vtkmlib_UnstructuredGridConverter_h

#include "vtkAcceleratorsVTKmDataModelModule.h"

#include "ArrayConverters.h" // For FieldsFlag

#include "vtkmConfigDataModel.h" //required for general viskores setup

#include <viskores/cont/DataSet.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;
class vtkDataSet;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

// convert an unstructured grid type
VTKACCELERATORSVTKMDATAMODEL_EXPORT
viskores::cont::DataSet Convert(vtkUnstructuredGrid* input, FieldsFlag fields = FieldsFlag::None);
VTK_ABI_NAMESPACE_END
}

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN
VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool Convert(
  const viskores::cont::DataSet& voutput, vtkUnstructuredGrid* output, vtkDataSet* input);
VTK_ABI_NAMESPACE_END
}
#endif // vtkmlib_UnstructuredGridConverter_h
