/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToPolyDataFilter.cxx
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
#include "vtkStructuredPointsToPolyDataFilter.h"

vtkCxxRevisionMacro(vtkStructuredPointsToPolyDataFilter, "1.24");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsToPolyDataFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkStructuredPointsToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToPolyDataFilter::ComputeInputUpdateExtents( 
                                                        vtkDataObject *output)
{
  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);

  if (!this->GetInput())
    {
    return;
    }
  // assume that we cannot handle more than the requested extent.
  this->GetInput()->RequestExactExtentOn();
}
