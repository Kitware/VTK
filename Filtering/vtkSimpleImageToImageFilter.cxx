/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleImageToImageFilter.cxx
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
#include "vtkSimpleImageToImageFilter.h"

vtkCxxRevisionMacro(vtkSimpleImageToImageFilter, "1.8");

//----------------------------------------------------------------------------
vtkSimpleImageToImageFilter::vtkSimpleImageToImageFilter()
{
  this->NumberOfRequiredInputs = 1;
}

//----------------------------------------------------------------------------
vtkSimpleImageToImageFilter::~vtkSimpleImageToImageFilter()
{
}

//----------------------------------------------------------------------------
void vtkSimpleImageToImageFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkSimpleImageToImageFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

void vtkSimpleImageToImageFilter::ExecuteInformation()
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();

  // Make sure the Input has been set.
  if ( input == NULL)
    {
    vtkErrorMacro(<< "ExecuteInformation: Input is not set.");
    return;
    }

  // Start with some defaults.
  output->CopyTypeSpecificInformation( input );
}


void vtkSimpleImageToImageFilter::ComputeInputUpdateExtent( int inExt[6], 
                                                            int vtkNotUsed(outExt)[6] )
{
  vtkImageData *input = this->GetInput();
  // Make sure the Input has been set.
  if ( input == NULL)
    {
    vtkErrorMacro(<< "ComputeInputUpdateExtent: Input is not set.");
    return;
    }
  int* wholeExtent = input->GetWholeExtent();
  memcpy(inExt,wholeExtent,sizeof(int)*6);
}

void vtkSimpleImageToImageFilter::ExecuteData(vtkDataObject *vtkNotUsed(out))
{

  vtkDebugMacro("Executing.");
  vtkImageData* output = this->GetOutput();
  vtkImageData* input = this->GetInput();

  if (!input)
    {
    vtkErrorMacro("No input is specified!");
    return;
    }

  // Set the extent of the output and allocate memory.
  output->SetExtent(output->GetWholeExtent());
  output->AllocateScalars();

  this->SimpleExecute(input, output);
}
