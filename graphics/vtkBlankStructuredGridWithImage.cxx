/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGridWithImage.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkBlankStructuredGridWithImage.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkBlankStructuredGridWithImage* vtkBlankStructuredGridWithImage::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBlankStructuredGridWithImage");
  if(ret)
    {
    return (vtkBlankStructuredGridWithImage*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBlankStructuredGridWithImage;
}


//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkBlankStructuredGridWithImage::SetBlankingInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkBlankStructuredGridWithImage::GetBlankingInput()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[1]);
}

void vtkBlankStructuredGridWithImage::Execute()
{
  vtkStructuredGrid *grid = this->GetInput();
  vtkStructuredGrid *output = this->GetOutput();
  vtkImageData *image = this->GetBlankingInput();
  int gridDims[3], imageDims[3];

  vtkDebugMacro(<< "Adding image blanking");
  
  // Perform error checking
  grid->GetDimensions(gridDims);
  image->GetDimensions(imageDims);
  if ( gridDims[0] != imageDims[0] || gridDims[1] != imageDims[1] ||
       gridDims[2] != imageDims[2] )
    {
    vtkErrorMacro(<< "Blanking dimensions must be identical with grid dimensions");
    return;
    }
  
  if ( image->GetScalarType() != VTK_UNSIGNED_CHAR ||
       image->GetNumberOfScalarComponents() != 1 )
    {
    vtkErrorMacro(<<"This filter requires unsigned char images with one component");
    return;
    }
  
  // Get the image, set it as the blanking array.
  unsigned char *data = (unsigned char *)image->GetScalarPointer();
  vtkUnsignedCharArray *dataArray = vtkUnsignedCharArray::New();
  dataArray->SetArray(data, gridDims[0]*gridDims[1]*gridDims[2], 1);

  output->CopyStructure(grid);
  output->GetPointData()->PassData(grid->GetPointData());
  output->GetCellData()->PassData(grid->GetCellData());
  output->SetPointVisibility(dataArray);
  output->BlankingOn();
  
  dataArray->Delete();
}


//----------------------------------------------------------------------------
void vtkBlankStructuredGridWithImage::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridToStructuredGridFilter::PrintSelf(os,indent);
}
