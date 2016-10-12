/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatricizeArray.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatricizeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"

#include <numeric>

///////////////////////////////////////////////////////////////////////////////
// vtkMatricizeArray

vtkStandardNewMacro(vtkMatricizeArray);

vtkMatricizeArray::vtkMatricizeArray() :
  SliceDimension(0)
{
}

vtkMatricizeArray::~vtkMatricizeArray()
{
}

void vtkMatricizeArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SliceDimension: " << this->SliceDimension << endl;
}

int vtkMatricizeArray::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
  {
    vtkErrorMacro(<< "vtkMatricizeArray requires vtkArrayData containing exactly one array as input.");
    return 0;
  }

  vtkSparseArray<double>* const input_array = vtkSparseArray<double>::SafeDownCast(
    input->GetArray(static_cast<vtkIdType>(0)));
  if(!input_array)
  {
    vtkErrorMacro(<< "vtkMatricizeArray requires a vtkSparseArray<double> as input.");
    return 0;
  }

  if(this->SliceDimension < 0 || this->SliceDimension >= input_array->GetDimensions())
  {
    vtkErrorMacro(<< "Slice dimension " << this->SliceDimension << " out-of-range for array with " << input_array->GetDimensions() << " dimensions.");
    return 0;
  }

  vtkSparseArray<double>* const output_array = vtkSparseArray<double>::New();

  // Compute the extents of the output array ...
  const vtkArrayExtents input_extents = input_array->GetExtents();
  vtkArrayExtents output_extents(0, 0);
  output_extents[0] = input_extents[this->SliceDimension];
  output_extents[1] = vtkArrayRange(0, input_extents.GetSize() / input_extents[this->SliceDimension].GetSize());
  output_array->Resize(output_extents);

  // "Map" every non-null element in the input array to its position in the output array.
  // Indices in the slice dimension map directly to the row index in the output.
  // The remaining coordinates are multiplied by a "stride" value for each dimension and
  // the results are summed to compute the output column index.
  //
  // Setting the slice-dimension stride to zero simplifies computation of column coordinates
  // later-on and eliminate an inner-loop comparison.
  std::vector<vtkIdType> strides(input_array->GetDimensions());
  for(vtkIdType i = input_array->GetDimensions() - 1, stride = 1; i >= 0; --i)
  {
    if(i == this->SliceDimension)
    {
      strides[i] = 0;
    }
    else
    {
      strides[i] = stride;
      stride *= input_extents[i].GetSize();
    }
  }

  std::vector<vtkIdType> temp(input_array->GetDimensions());

  vtkArrayCoordinates coordinates;
  vtkArrayCoordinates new_coordinates(0, 0);
  const vtkIdType element_count = input_array->GetNonNullSize();
  for(vtkIdType n = 0; n != element_count; ++n)
  {
    input_array->GetCoordinatesN(n, coordinates);

    new_coordinates[0] = coordinates[this->SliceDimension];

    for(vtkIdType i = 0; i != coordinates.GetDimensions(); ++i)
      temp[i] = (coordinates[i] - input_extents[i].GetBegin()) * strides[i];
    new_coordinates[1] = std::accumulate(temp.begin(), temp.end(), 0);

    output_array->AddValue(new_coordinates, input_array->GetValueN(n));
  }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(output_array);
  output_array->Delete();

  return 1;
}

