/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionShiftScale.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPiecewiseFunctionShiftScale.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

vtkStandardNewMacro(vtkPiecewiseFunctionShiftScale);

vtkPiecewiseFunctionShiftScale::vtkPiecewiseFunctionShiftScale()
{
  this->PositionShift = 0.0;
  this->PositionScale = 1.0;
  this->ValueShift = 0.0;
  this->ValueScale = 1.0;
}

vtkPiecewiseFunctionShiftScale::~vtkPiecewiseFunctionShiftScale()
{
}

int vtkPiecewiseFunctionShiftScale::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPiecewiseFunction *input = vtkPiecewiseFunction::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPiecewiseFunction *output = vtkPiecewiseFunction::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double *inFunction = input->GetDataPointer();
  int numInValues = input->GetSize();
  
  output->RemoveAllPoints();
  
  int i;
  
  for (i = 0; i < numInValues; i++)
    {
    output->AddPoint((inFunction[2*i] + this->PositionShift) *
                     this->PositionScale,
                     (inFunction[2*i+1] + this->ValueShift) *
                     this->ValueScale);
    }

  return 1;
}

void vtkPiecewiseFunctionShiftScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "PositionShift: " << this->PositionShift << "\n";
  os << indent << "PositionScale: " << this->PositionScale << "\n";
  os << indent << "ValueShift: " << this->ValueShift << "\n";
  os << indent << "ValueScale: " << this->ValueScale << "\n";
}
