/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToUnstructuredGridFilter.cxx
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
#include "vtkStructuredPointsToUnstructuredGridFilter.h"

vtkCxxRevisionMacro(vtkStructuredPointsToUnstructuredGridFilter, "1.12");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsToUnstructuredGridFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkStructuredPointsToUnstructuredGridFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToUnstructuredGridFilter::ComputeInputUpdateExtents( 
                                                        vtkDataObject *output)
{
  this->vtkUnstructuredGridSource::ComputeInputUpdateExtents(output);

  // assume that we cannot handle more than the requested extent.
  if (this->GetInput())
    {
    this->GetInput()->RequestExactExtentOn();
    }
}
