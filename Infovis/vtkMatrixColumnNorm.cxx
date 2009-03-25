/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixColumnNorm.cxx
  
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
#include "vtkMatrixColumnNorm.h"

///////////////////////////////////////////////////////////////////////////////
// vtkMatrixColumnNorm

vtkCxxRevisionMacro(vtkMatrixColumnNorm, "1.2");
vtkStandardNewMacro(vtkMatrixColumnNorm);

vtkMatrixColumnNorm::vtkMatrixColumnNorm() :
  L(2)
{
}

vtkMatrixColumnNorm::~vtkMatrixColumnNorm()
{
}

void vtkMatrixColumnNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "L: " << this->L << endl;
}

void vtkMatrixColumnNorm::SetL(int value)
{
  if(value < 1)
    {
    vtkErrorMacro(<< "Cannot compute vector norm for L < 1");
    return;
    }
  
  if(this->L == value)
    return;

  this->L = value;
  this->Modified();
}

int vtkMatrixColumnNorm::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
    {
    vtkErrorMacro(<< "vtkMatrixColumnNorm requires vtkArrayData containing exactly one vtkArray as input.");
    return 0;
    }

  vtkTypedArray<double>* const input_array = vtkTypedArray<double>::SafeDownCast(input->GetArray(0));
  if(!input_array)
    {
    vtkErrorMacro(<< "vtkMatrixColumnNorm requires a vtkTypedArray<double> input array.");
    return 0;
    }
  if(input_array->GetDimensions() != 2)
    {
    vtkErrorMacro(<< "vtkMatrixColumnNorm requires an input matrix.");
    return 0;
    }

  vtkDenseArray<double>* const output_array = vtkDenseArray<double>::New();

  const vtkArrayExtents input_extents = input_array->GetExtents();

  output_array->Resize(vtkArrayExtents(input_extents[1]));
  output_array->Fill(0.0);

  vtkArrayCoordinates coordinates;
  const vtkIdType element_count = input_array->GetNonNullSize();
  for(vtkIdType n = 0; n != element_count; ++n)
    {
    input_array->GetCoordinatesN(n, coordinates);
    (*output_array)[vtkArrayCoordinates(coordinates[1])] += pow(input_array->GetValueN(n), this->L);
    }

  for(vtkIdType i = 0; i != input_extents[1]; ++i)
    {
    (*output_array)[vtkArrayCoordinates(i)] = pow((*output_array)[vtkArrayCoordinates(i)], 1.0 / this->L);
    }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(output_array);
  output_array->Delete();

  return 1;
}

