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

#include "../Core/vtkmlib/DataArrayConverters.h"

#include "vtkAcceleratorsVTKmDataModelModule.h" //required for correct implementation
#include "vtkmConfigDataModel.h"                //required for general vtkm setup

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/Field.h>

#include <type_traits> // for std::underlying_type

class vtkDataArray;
class vtkDataSet;
class vtkPoints;

namespace vtkm
{
namespace cont
{
class DataSet;
class CoordinateSystem;
}
}

namespace tovtkm
{

VTKACCELERATORSVTKMDATAMODEL_EXPORT
void ProcessFields(vtkDataSet* input, vtkm::cont::DataSet& dataset, tovtkm::FieldsFlag fields);

// determine the type and call the proper Convert routine
VTKACCELERATORSVTKMDATAMODEL_EXPORT
vtkm::cont::Field Convert(vtkDataArray* input, int association);
}

namespace fromvtkm
{

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool ConvertArrays(const vtkm::cont::DataSet& input, vtkDataSet* output);
}

#endif // vtkmlib_ArrayConverters_h
