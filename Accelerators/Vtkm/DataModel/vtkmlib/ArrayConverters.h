// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_ArrayConverters_h
#define vtkmlib_ArrayConverters_h

#include "vtkmlib/DataArrayConverters.h"

#include "vtkAcceleratorsVTKmDataModelModule.h" //required for correct implementation
#include "vtkmConfigDataModel.h"                //required for general viskores setup

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/Field.h>

#include <type_traits> // for std::underlying_type

namespace viskores
{
namespace cont
{
class DataSet;
class CoordinateSystem;
}
}

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
class vtkPoints;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMDATAMODEL_EXPORT
void ProcessFields(vtkDataSet* input, viskores::cont::DataSet& dataset, tovtkm::FieldsFlag fields);

// determine the type and call the proper Convert routine
VTKACCELERATORSVTKMDATAMODEL_EXPORT
viskores::cont::Field Convert(vtkDataArray* input, int association);
VTK_ABI_NAMESPACE_END
}

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool ConvertArrays(const viskores::cont::DataSet& input, vtkDataSet* output);
VTK_ABI_NAMESPACE_END
}

#endif // vtkmlib_ArrayConverters_h
