/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToUnstructuredGridFilter.cxx
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
#include "vtkUnstructuredGridToUnstructuredGridFilter.h"

vtkCxxRevisionMacro(vtkUnstructuredGridToUnstructuredGridFilter, "1.9");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkUnstructuredGridToUnstructuredGridFilter::SetInput(vtkUnstructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid *vtkUnstructuredGridToUnstructuredGridFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkUnstructuredGrid *)(this->Inputs[0]);
}


