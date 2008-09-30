/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNormalizeMatrixVectors.cxx
  
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

#include "vtkArrayCoordinates.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNormalizeMatrixVectors.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTypedArray.h"

///////////////////////////////////////////////////////////////////////////////
// vtkNormalizeMatrixVectors

vtkCxxRevisionMacro(vtkNormalizeMatrixVectors, "1.1");
vtkStandardNewMacro(vtkNormalizeMatrixVectors);

vtkNormalizeMatrixVectors::vtkNormalizeMatrixVectors() :
  VectorDimension(1)
{
}

vtkNormalizeMatrixVectors::~vtkNormalizeMatrixVectors()
{
}

void vtkNormalizeMatrixVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
}

int vtkNormalizeMatrixVectors::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  vtkArrayData* const output = vtkArrayData::GetData(outputVector);

  int vector_dimension = vtkstd::min(1, vtkstd::max(0, this->VectorDimension));

  vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input->GetArray());
  if(!input_array)
    {
    vtkErrorMacro(<< "vtkNormalizeMatrixVectors requires a vtkTypedArray<double> as input.");
    return 0;
    }
  if(input_array->GetDimensions() != 2)
    {
    vtkErrorMacro(<< "vtkNormalizeMatrixVectors requires a matrix as input.");
    return 0;
    }

  vtkTypedArray<double>* const output_array = vtkTypedArray<double>::SafeDownCast(input_array->DeepCopy());
  output->SetArray(output_array);
  output_array->Delete();

  const vtkIdType vector_count = input_array->GetExtents()[vector_dimension];
  const vtkIdType value_count = input_array->GetNonNullSize();
  
  // Create temporary storage for computed vector weights ...
  vtkstd::vector<double> weight(vector_count, 0.0);

  // Store the sum of the squares of each vector value ...
  vtkArrayCoordinates coordinates;
  for(vtkIdType n = 0; n != value_count; ++n)
    {
    output_array->GetCoordinatesN(n, coordinates);
    weight[coordinates[vector_dimension]] += pow(output_array->GetValueN(n), 2);
    }

  // Convert the sums into weights, avoiding divide-by-zero ...
  for(vtkIdType i = 0; i != vector_count; ++i)
    {
    const double length = sqrt(weight[i]);
    weight[i] = length ? 1.0 / length : 0.0;
    }

  // Apply the weights to each vector ...
  for(vtkIdType n = 0; n != value_count; ++n)
    {
    output_array->GetCoordinatesN(n, coordinates);
    output_array->SetValueN(n, output_array->GetValueN(n) * weight[coordinates[vector_dimension]]);
    }
  
  return 1;
}

