/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageDataConverter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkmlib_ImageDataConverter_h
#define vtkmlib_ImageDataConverter_h

#include "vtkAcceleratorsVTKmModule.h"

#include "ArrayConverters.h" // for FieldsFlag

#include "vtkmConfig.h" //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

class vtkImageData;
class vtkDataSet;

namespace tovtkm
{

VTKACCELERATORSVTKM_EXPORT
vtkm::cont::DataSet Convert(vtkImageData* input, FieldsFlag fields = FieldsFlag::None);

}

namespace fromvtkm
{

VTKACCELERATORSVTKM_EXPORT
bool Convert(const vtkm::cont::DataSet& voutput, vtkImageData* output, vtkDataSet* input);

VTKACCELERATORSVTKM_EXPORT
bool Convert(
  const vtkm::cont::DataSet& voutput, int extents[6], vtkImageData* output, vtkDataSet* input);

}
#endif // vtkmlib_ImageDataConverter_h
