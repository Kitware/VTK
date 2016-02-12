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

#ifndef vtkmlib_UnstructuredGridConverter_h
#define vtkmlib_UnstructuredGridConverter_h

#include "vtkAcceleratorsVTKmModule.h"
#include "vtkmConfig.h" //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

class vtkUnstructuredGrid;
class vtkDataSet;

namespace tovtkm {

// convert an unstructured grid type
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DataSet Convert(vtkUnstructuredGrid* input);
}

namespace fromvtkm {
VTKACCELERATORSVTKM_EXPORT
bool Convert(const vtkm::cont::DataSet& voutput, vtkUnstructuredGrid* output,
             vtkDataSet* input);
}
#endif // vtkmlib_UnstructuredGridConverter_h
