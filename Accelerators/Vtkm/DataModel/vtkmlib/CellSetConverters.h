// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef vtkmlib_CellSetConverters_h
#define vtkmlib_CellSetConverters_h

#include "vtkAcceleratorsVTKmDataModelModule.h"
#include "vtkSmartPointer.h"     // For vtkSmartPointer
#include "vtkmConfigDataModel.h" // required for general viskores setup

#include <viskores/cont/UnknownCellSet.h>
#include <vtkType.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkPolyData;
class vtkUnstructuredGrid;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
VTKACCELERATORSVTKMDATAMODEL_EXPORT
vtkIdType IsHomogeneous(vtkCellArray* cells);

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool IsHomogeneous(vtkUnstructuredGrid* ugrid);

VTKACCELERATORSVTKMDATAMODEL_EXPORT
vtkSmartPointer<vtkDataArray> CreatePolygonalCellTypes(vtkCellArray* input);

VTKACCELERATORSVTKMDATAMODEL_EXPORT
viskores::cont::UnknownCellSet ConvertSingleType(
  vtkCellArray* cells, int cellType, vtkIdType numberOfPoints, bool forceViskores = false);

VTKACCELERATORSVTKMDATAMODEL_EXPORT
viskores::cont::UnknownCellSet Convert(
  vtkDataArray* types, vtkCellArray* cells, vtkIdType numberOfPoints, bool forceViskores = false);
VTK_ABI_NAMESPACE_END
}

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool Convert(
  const viskores::cont::UnknownCellSet& toConvert, vtkCellArray* cells, bool forceViskores = false);
bool Convert(const viskores::cont::UnknownCellSet& toConvert, vtkCellArray* cells,
  vtkSmartPointer<vtkDataArray>& types, bool forceViskores = false);
VTK_ABI_NAMESPACE_END
}

#endif // vtkmlib_CellSetConverters_h
