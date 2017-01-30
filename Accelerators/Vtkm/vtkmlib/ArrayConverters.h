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

#ifndef vtkmlib_ArrayConverters_h
#define vtkmlib_ArrayConverters_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkmConfig.h"                //required for general vtkm setup

#include <vtkm/cont/Field.h>

class vtkDataArray;
class vtkDataSet;
class vtkPoints;

namespace vtkm {
namespace cont {
class DataSet;
class CoordinateSystem;
}
}

namespace tovtkm {

// determine the type and call the proper Convert routine
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::Field Convert(vtkDataArray* input, int association);
}

namespace fromvtkm {

VTKACCELERATORSVTKM_EXPORT
vtkDataArray* Convert(const vtkm::cont::Field& input);

VTKACCELERATORSVTKM_EXPORT
vtkPoints* Convert(const vtkm::cont::CoordinateSystem& input);

VTKACCELERATORSVTKM_EXPORT
bool ConvertArrays(const vtkm::cont::DataSet& input, vtkDataSet* output);
}

#endif // vtkmlib_ArrayConverters_h