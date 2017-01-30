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

#include "vtkAcceleratorsVTKmModule.h"
#include "vtkmConfig.h" //required for general vtkm setup

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkType.h>

class vtkCellArray;
class vtkUnsignedCharArray;
class vtkIdTypeArray;

namespace tovtkm {
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DynamicCellSet ConvertSingleType(vtkCellArray* cells, int cellType,
                                             vtkIdType numberOfPoints);

VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DynamicCellSet Convert(vtkUnsignedCharArray* types,
                                   vtkCellArray* cells,
                                   vtkIdTypeArray* locations,
                                   vtkIdType numberOfPoints);
}

namespace fromvtkm {

VTKACCELERATORSVTKM_EXPORT
bool Convert(const vtkm::cont::DynamicCellSet& toConvert,
             vtkCellArray* cells,
             vtkUnsignedCharArray* types = NULL,
             vtkIdTypeArray* locations = NULL);
}

#endif // vtkmlib_CellSetConverters_h
