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

vtkStandardNewMacro(vtkNormalizeMatrixVectors);

vtkNormalizeMatrixVectors::vtkNormalizeMatrixVectors() :
  VectorDimension(1),
  PValue(2)
{
}

vtkNormalizeMatrixVectors::~vtkNormalizeMatrixVectors()
{
}

void vtkNormalizeMatrixVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
  os << indent << "PValue: " << this->PValue << endl;
}

int vtkNormalizeMatrixVectors::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  int vector_dimension = std::min(1, std::max(0, this->VectorDimension));
  double p_value = std::max(1.0, this->PValue);

  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
    {
    vtkErrorMacro(<< "vtkNormalizeMatrixVectors requires vtkArrayData containing exactly one array as input.");
    return 0;
    }

  vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(
    input->GetArray(static_cast<vtkIdType>(0)));
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

  const vtkArrayRange vectors = input_array->GetExtent(vector_dimension);
  const vtkIdType value_count = input_array->GetNonNullSize();
  
  // Create temporary storage for computed vector weights ...
  std::vector<double> weight(vectors.GetSize(), 0.0);

  // Store the sum of the squares of each vector value ...
  vtkArrayCoordinates coordinates;
  for(vtkIdType n = 0; n != value_count; ++n)
    {
    output_array->GetCoordinatesN(n, coordinates);
    weight[coordinates[vector_dimension] - vectors.GetBegin()] += pow(output_array->GetValueN(n), p_value);
    }

  // Convert the sums into weights, avoiding divide-by-zero ...
  for(vtkIdType i = 0; i != vectors.GetSize(); ++i)
    {
    const double length = pow(weight[i],1.0/p_value);
    weight[i] = length ? 1.0 / length : 0.0;
    }

  // Apply the weights to each vector ...
  for(vtkIdType n = 0; n != value_count; ++n)
    {
    output_array->GetCoordinatesN(n, coordinates);
    output_array->SetValueN(n, output_array->GetValueN(n) * weight[coordinates[vector_dimension] - vectors.GetBegin()]);
    }
  
  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(output_array);
  output_array->Delete();

  return 1;
}

