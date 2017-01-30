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

#ifndef vtkmlib_PolyDataConverter_h
#define vtkmlib_PolyDataConverter_h

#include "vtkAcceleratorsVTKmModule.h"
#include "vtkmConfig.h" //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

class vtkPolyData;
class vtkDataSet;

namespace tovtkm {
// convert an polydata type
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DataSet Convert(vtkPolyData* input);
}

namespace fromvtkm {
VTKACCELERATORSVTKM_EXPORT
bool Convert(const vtkm::cont::DataSet& voutput, vtkPolyData* output,
             vtkDataSet* input);
}
#endif // vtkmlib_PolyDataConverter_h
