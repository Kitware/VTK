/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionShiftScale.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPiecewiseFunctionShiftScale.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPiecewiseFunctionShiftScale, "1.1");
vtkStandardNewMacro(vtkPiecewiseFunctionShiftScale);

vtkPiecewiseFunctionShiftScale::vtkPiecewiseFunctionShiftScale()
{
  this->Input = NULL;
  
  this->PositionShift = 0.0;
  this->PositionScale = 1.0;
  this->ValueShift = 0.0;
  this->ValueScale = 1.0;
}

vtkPiecewiseFunctionShiftScale::~vtkPiecewiseFunctionShiftScale()
{
  this->SetInput(NULL);
}

void vtkPiecewiseFunctionShiftScale::Execute()
{
  vtkPiecewiseFunction *input = this->GetInput();
  vtkPiecewiseFunction *output = this->GetOutput();
  
  if ( ! input )
    {
    vtkErrorMacro("No input set.");
    return;
    }

  float *inFunction = input->GetDataPointer();
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
}

void vtkPiecewiseFunctionShiftScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "PositionShift: " << this->PositionShift << "\n";
  os << indent << "PositionScale: " << this->PositionScale << "\n";
  os << indent << "ValueShift: " << this->ValueShift << "\n";
  os << indent << "ValueScale: " << this->ValueScale << "\n";
}
