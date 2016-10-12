/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransposeMatrix.cxx

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
#include "vtkDenseArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkTransposeMatrix.h"

///////////////////////////////////////////////////////////////////////////////
// vtkTransposeMatrix

vtkStandardNewMacro(vtkTransposeMatrix);

vtkTransposeMatrix::vtkTransposeMatrix()
{
}

vtkTransposeMatrix::~vtkTransposeMatrix()
{
}

void vtkTransposeMatrix::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkTransposeMatrix::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
  {
    vtkErrorMacro(<< "vtkTransposeMatrix requires vtkArrayData containing exactly one array as input.");
    return 0;
  }

  if(vtkSparseArray<double>* const input_array = vtkSparseArray<double>::SafeDownCast(
      input->GetArray(static_cast<vtkIdType>(0))))
  {
    if(input_array->GetDimensions() != 2)
    {
      vtkErrorMacro(<< "vtkTransposeMatrix requires a matrix as input.");
      return 0;
    }

    const vtkArrayExtents input_extents = input_array->GetExtents();

    vtkSparseArray<double>* const output_array = vtkSparseArray<double>::New();
    output_array->Resize(vtkArrayExtents(input_extents[1], input_extents[0]));
    output_array->SetDimensionLabel(0, input_array->GetDimensionLabel(1));
    output_array->SetDimensionLabel(1, input_array->GetDimensionLabel(0));

    vtkArrayCoordinates coordinates;
    const vtkIdType element_count = input_array->GetNonNullSize();
    for(vtkIdType n = 0; n != element_count; ++n)
    {
      input_array->GetCoordinatesN(n, coordinates);
      output_array->AddValue(vtkArrayCoordinates(coordinates[1], coordinates[0]), input_array->GetValueN(n));
    }

    vtkArrayData* const output = vtkArrayData::GetData(outputVector);
    output->ClearArrays();
    output->AddArray(output_array);
    output_array->Delete();
  }
  else
  {
    vtkDenseArray<double>* const input_array2=vtkDenseArray<double>::SafeDownCast(
      input->GetArray(static_cast<vtkIdType>(0)));
    if(input_array2!=0)
    {
      if(input_array2->GetDimensions() != 2)
      {
        vtkErrorMacro(<< "vtkTransposeMatrix requires a matrix as input.");
        return 0;
      }

      const vtkArrayExtents input_extents = input_array2->GetExtents();

      vtkDenseArray<double>* const output_array = vtkDenseArray<double>::New();

      output_array->Resize(vtkArrayExtents(input_extents[1],input_extents[0]));
      output_array->SetDimensionLabel(0, input_array2->GetDimensionLabel(1));
      output_array->SetDimensionLabel(1, input_array2->GetDimensionLabel(0));

      for(vtkIdType i = input_extents[0].GetBegin(); i != input_extents[0].GetEnd(); ++i)
      {
        for(vtkIdType j = input_extents[1].GetBegin(); j != input_extents[1].GetEnd(); ++j)
        {
          output_array->SetValue(
            vtkArrayCoordinates(j, i),
            input_array2->GetValue(vtkArrayCoordinates(i, j)));
        }
      }

      vtkArrayData* const output = vtkArrayData::GetData(outputVector);
      output->ClearArrays();
      output->AddArray(output_array);
      output_array->Delete();
    }
    else
    {
      vtkErrorMacro(<< "Unsupported input array type.");
      return 0;
    }
  }

  return 1;
}

