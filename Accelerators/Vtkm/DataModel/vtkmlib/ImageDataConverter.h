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

#include "vtkAcceleratorsVTKmDataModelModule.h"

#include "ArrayConverters.h" // for FieldsFlag

#include "vtkmConfigDataModel.h" //required for general vtkm setup

#include <vtkm/cont/DataSet.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkDataSet;
VTK_ABI_NAMESPACE_END

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMDATAMODEL_EXPORT
vtkm::cont::DataSet Convert(vtkImageData* input, FieldsFlag fields = FieldsFlag::None);

VTK_ABI_NAMESPACE_END
}

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool Convert(const vtkm::cont::DataSet& voutput, vtkImageData* output, vtkDataSet* input);

VTKACCELERATORSVTKMDATAMODEL_EXPORT
bool Convert(
  const vtkm::cont::DataSet& voutput, int extents[6], vtkImageData* output, vtkDataSet* input);

VTK_ABI_NAMESPACE_END
}
#endif // vtkmlib_ImageDataConverter_h
