/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToPolyDataFilter.cxx
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
#include "vtkDataSetToPolyDataFilter.h"

vtkCxxRevisionMacro(vtkDataSetToPolyDataFilter, "1.13");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToPolyDataFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkDataSetToPolyDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input == NULL)
    {
    return;
    }
  
  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);
  input->RequestExactExtentOn();
}
