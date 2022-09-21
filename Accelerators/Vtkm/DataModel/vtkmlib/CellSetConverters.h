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

#ifndef vtkmlib_CellSetConverters_h
#define vtkmlib_CellSetConverters_h

#include "vtkAcceleratorsVTKmDataModelModule.h"
#include "vtkmConfigDataModel.h" //required for general vtkm setup

#include <vtkType.h>
#include <vtkm/cont/UnknownCellSet.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkUnsignedCharArray;
class vtkIdTypeArray;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
VTKACCELERATORSVTKMDATAMODEL_EXPORT
vtkm::cont::UnknownCellSet ConvertSingleType(
  vtkCellArray* cells, int cellType, vtkIdType numberOfPoints);

VTKACCELERATORSVTKMDATAMODEL_EXPORT
vtkm::cont::UnknownCellSet Convert(
  vtkUnsignedCharArray* types, vtkCellArray* cells, vtkIdType numberOfPoints);
VTK_ABI_NAMESPACE_END
}

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool Convert(const vtkm::cont::UnknownCellSet& toConvert, vtkCellArray* cells,
  vtkUnsignedCharArray* types = nullptr);
VTK_ABI_NAMESPACE_END
}

#endif // vtkmlib_CellSetConverters_h
