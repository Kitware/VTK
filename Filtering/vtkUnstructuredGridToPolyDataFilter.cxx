/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToPolyDataFilter.cxx
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
#include "vtkUnstructuredGridToPolyDataFilter.h"

vtkCxxRevisionMacro(vtkUnstructuredGridToPolyDataFilter, "1.4");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkUnstructuredGridToPolyDataFilter::SetInput(vtkUnstructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid *vtkUnstructuredGridToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkUnstructuredGrid *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkUnstructuredGridToPolyDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input)
    {
    this->vtkPolyDataSource::ComputeInputUpdateExtents(output);
    input->RequestExactExtentOn();
    }
}
