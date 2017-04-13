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

#ifndef vtkmlib_DataSetConverters_h
#define vtkmlib_DataSetConverters_h

#include "vtkAcceleratorsVTKmModule.h"
#include "vtkmConfig.h" //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

class vtkImageData;
class vtkStructuredGrid;
class vtkPoints;
class vtkDataSet;

namespace tovtkm {

// convert a vtkPoints array into a coordinate system
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::CoordinateSystem Convert(vtkPoints* points);

// convert an image data type
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DataSet Convert(vtkImageData* input);

// convert an structured grid type
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DataSet Convert(vtkStructuredGrid* input);


// determine the type and call the proper Convert routine
VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DataSet Convert(vtkDataSet* input);
}

#endif // vtkmlib_DataSetConverters_h
