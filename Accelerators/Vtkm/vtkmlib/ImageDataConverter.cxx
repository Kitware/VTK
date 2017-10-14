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
#include "ImageDataConverter.h"

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include "ArrayConverters.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkPointData.h"


namespace tovtkm {

//------------------------------------------------------------------------------
// convert an image data type
vtkm::cont::DataSet Convert(vtkImageData *input, FieldsFlag fields)
{
  int extent[6];
  input->GetExtent(extent);
  double vorigin[3];
  input->GetOrigin(vorigin);
  double vspacing[3];
  input->GetSpacing(vspacing);
  int vdims[3];
  input->GetDimensions(vdims);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(
    (static_cast<double>(extent[0]) * vspacing[0]) + vorigin[0],
    (static_cast<double>(extent[2]) * vspacing[1]) + vorigin[1],
    (static_cast<double>(extent[4]) * vspacing[2]) + vorigin[2]);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(vspacing[0],
                                           vspacing[1],
                                           vspacing[2]);
  vtkm::Id3 dims(vdims[0], vdims[1], vdims[2]);

  vtkm::cont::DataSet dataset =
      vtkm::cont::DataSetBuilderUniform::Create(dims, origin, spacing);

  ProcessFields(input, dataset, fields);

  return dataset;
}

} // tovtkm

namespace fromvtkm {

bool Convert(const vtkm::cont::DataSet& voutput, int extents[6], vtkImageData* output,
             vtkDataSet* input)
{
  vtkm::cont::CoordinateSystem cs = voutput.GetCoordinateSystem();
  if (!cs.GetData().IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
  {
    return false;
  }

  auto points = cs.GetData().Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = points.GetPortalConstControl();

  auto origin = portal.GetOrigin();
  auto spacing = portal.GetSpacing();
  auto dim = portal.GetDimensions();
  VTKM_ASSERT((extents[1] - extents[0] + 1) == dim[0] &&
              (extents[3] - extents[2] + 1) == dim[1] &&
              (extents[5] - extents[4] + 1) == dim[2]);

  origin[0] -= static_cast<vtkm::FloatDefault>(extents[0]) * spacing[0];
  origin[1] -= static_cast<vtkm::FloatDefault>(extents[2]) * spacing[1];
  origin[2] -= static_cast<vtkm::FloatDefault>(extents[4]) * spacing[2];

  output->SetExtent(extents);
  output->SetOrigin(origin[0], origin[1], origin[2]);
  output->SetSpacing(spacing[0], spacing[1], spacing[2]);

  // Next we need to convert any extra fields from vtkm over to vtk
  bool arraysConverted = fromvtkm::ConvertArrays(voutput, output);

  // Pass information about attributes.
  for (int attributeType = 0;
       attributeType < vtkDataSetAttributes::NUM_ATTRIBUTES; attributeType++)
  {
    vtkDataArray* attribute =
        input->GetPointData()->GetAttribute(attributeType);
    if (attribute == nullptr)
    {
      continue;
    }
    output->GetPointData()->SetActiveAttribute(attribute->GetName(),
                                               attributeType);
  }

  return arraysConverted;
}

bool Convert(const vtkm::cont::DataSet& voutput, vtkImageData* output,
             vtkDataSet* input)
{
  vtkm::cont::CoordinateSystem cs = voutput.GetCoordinateSystem();
  if (!cs.GetData().IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
  {
    return false;
  }

  auto points = cs.GetData().Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = points.GetPortalConstControl();

  auto dim = portal.GetDimensions();
  int extents[6] = {0, static_cast<int>(dim[0] - 1),
                    0, static_cast<int>(dim[1] - 1),
                    0, static_cast<int>(dim[2] - 1)};
  return Convert(voutput, extents, output, input);
}

} // fromvtkm
