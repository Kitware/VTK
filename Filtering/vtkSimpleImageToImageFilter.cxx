/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleImageToImageFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSimpleImageToImageFilter.h"
#include "vtkObjectFactory.h"



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
