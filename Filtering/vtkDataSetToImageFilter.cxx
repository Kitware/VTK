/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToImageFilter.cxx
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
#include "vtkDataSetToImageFilter.h"

vtkCxxRevisionMacro(vtkDataSetToImageFilter, "1.1");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToImageFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToImageFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// All the DataSetToImageFilters require all their input.
void vtkDataSetToImageFilter::ComputeInputUpdateExtents(
  vtkDataObject *data)
{
  vtkImageData *output = (vtkImageData *)data;
  vtkDataSet *input = this->GetInput();
  int *ext;
  
  if (input == NULL)
    {
    return;
    }
  
  // Lets just check to see if the outputs UpdateExtent is valid.
  ext = output->GetUpdateExtent();
  if (ext[0] > ext[1] || ext[2] > ext[3] || ext[4] > ext[5])
    {
    return;
    }
  
  input->SetUpdateExtent(0, 1, 0);
}

    





