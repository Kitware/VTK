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

#include "vtkAcceleratorsVTKmCoreModule.h"

#include "ArrayConverters.h" // for FieldsFlag

#include "vtkmConfigCore.h" //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

class vtkDataSet;
class vtkDataSetAttributes;
class vtkImageData;
class vtkPoints;
class vtkRectilinearGrid;
class vtkStructuredGrid;

namespace tovtkm
{

// convert a vtkPoints array into a coordinate system
VTKACCELERATORSVTKMCORE_EXPORT
vtkm::cont::CoordinateSystem Convert(vtkPoints* points);

// convert an structured grid type
VTKACCELERATORSVTKMCORE_EXPORT
vtkm::cont::DataSet Convert(vtkStructuredGrid* input, FieldsFlag fields = FieldsFlag::None);

// determine the type and call the proper Convert routine
VTKACCELERATORSVTKMCORE_EXPORT
vtkm::cont::DataSet Convert(vtkDataSet* input, FieldsFlag fields = FieldsFlag::None);
}

namespace fromvtkm
{

VTKACCELERATORSVTKMCORE_EXPORT
void PassAttributesInformation(vtkDataSetAttributes* input, vtkDataSetAttributes* output);

VTKACCELERATORSVTKMCORE_EXPORT
bool Convert(const vtkm::cont::DataSet& vtkmOut, vtkRectilinearGrid* output, vtkDataSet* input);

VTKACCELERATORSVTKMCORE_EXPORT
bool Convert(const vtkm::cont::DataSet& vtkmOut, vtkStructuredGrid* output, vtkDataSet* input);

}

#endif // vtkmlib_DataSetConverters_h
