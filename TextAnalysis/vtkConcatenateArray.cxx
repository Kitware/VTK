/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConcatenateArray.cxx
  
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
#include "vtkConcatenateArray.h"
#include "vtkDenseArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"

#include <vtksys/stl/numeric>

#include <stdexcept>

static void CopyValues(vtkArray* const source, vtkArray* const target, const vtkIdType dimension, vtkIdType& offset)
{
  const vtkIdType source_count = source->GetNonNullSize();

  vtkArrayCoordinates target_coordinates;
  for(vtkIdType source_index = 0; source_index != source_count; ++source_index)
    {
    source->GetCoordinatesN(source_index, target_coordinates);
    target_coordinates[dimension] += offset;
    target->CopyValue(source, source_index, target_coordinates);
    }

  offset += source->GetExtent(dimension).GetSize();
}

///////////////////////////////////////////////////////////////////////////////
// vtkConcatenateArray

vtkStandardNewMacro(vtkConcatenateArray);

vtkConcatenateArray::vtkConcatenateArray() :
  AdjacentDimension(0)
{
  this->SetNumberOfInputPorts(2);
}

vtkConcatenateArray::~vtkConcatenateArray()
{
}

void vtkConcatenateArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AdjacentDimension: " << this->AdjacentDimension << endl;
}

int vtkConcatenateArray::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  try
    {
    vtkArrayData* const input1 = vtkArrayData::GetData(inputVector[0]);
    if(!input1)
      throw vtkstd::runtime_error("missing first input array data.");

    if(input1->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayData containing exactly one vtkArray required for first input.");

    vtkArrayData* const input2 = vtkArrayData::GetData(inputVector[1]);
    if(!input2)
      throw vtkstd::runtime_error("missing second input array data.");

    if(input2->GetNumberOfArrays() != 1)
      throw vtkstd::runtime_error("vtkArrayData containing exactly one vtkArray required for second input.");

    vtkArray* const array1 = input1->GetArray(0);
    if(!array1)
      throw vtkstd::runtime_error("missing first input array.");

    vtkArray* const array2 = input2->GetArray(0);
    if(!array2)
      throw vtkstd::runtime_error("missing second input array.");

    if(vtkStdString(array1->GetClassName()) != vtkStdString(array2->GetClassName()))
      throw vtkstd::runtime_error("first and second input arrays must be identical types.");

    if(array1->GetDimensions() != array2->GetDimensions())
      throw vtkstd::runtime_error("input arrays must match dimensions.");

    if(this->AdjacentDimension < 0 || this->AdjacentDimension >= array1->GetDimensions())
      throw vtkstd::runtime_error("adjacent dimension out-of-bounds.");

    for(vtkIdType i = 0; i != array1->GetDimensions(); ++i)
      {
      if(i == this->AdjacentDimension)
        continue;

      if(array1->GetExtent(i) != array2->GetExtent(i))
        throw vtkstd::runtime_error("array extent mismatch");
      }

    vtkArrayExtents output_extents = array1->GetExtents();
    output_extents[this->AdjacentDimension] =
      vtkArrayRange(
        array1->GetExtent(this->AdjacentDimension).GetBegin(),
        array1->GetExtent(this->AdjacentDimension).GetEnd() + array2->GetExtent(this->AdjacentDimension).GetSize());

    vtkArray* const output_array = array1->NewInstance();
    output_array->Resize(output_extents);

    vtkIdType offset = 0;
    CopyValues(array1, output_array, this->AdjacentDimension, offset);
    CopyValues(array2, output_array, this->AdjacentDimension, offset);

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();

    return 1;
    } 
  catch(vtkstd::exception& e)
    {
    vtkErrorMacro(<< "caught exception: " << e.what() << endl);
    }
  catch(...)
    {
    vtkErrorMacro(<< "caught unknown exception." << endl);
    }

  return 0;
}

