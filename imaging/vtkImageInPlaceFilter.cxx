/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.cxx
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

#include "vtkImageInPlaceFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageInPlaceFilter* vtkImageInPlaceFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageInPlaceFilter");
  if(ret)
    {
    return (vtkImageInPlaceFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageInPlaceFilter;
}



//----------------------------------------------------------------------------

void vtkImageInPlaceFilter::ExecuteData(vtkDataObject *vtkNotUsed(out))
{
  vtkImageData *output = this->GetOutput();
  int *inExt, *outExt;
  
  inExt = this->GetInput()->GetUpdateExtent();
  outExt = this->GetOutput()->GetUpdateExtent();

  vtkImageData *input;
  input = this->GetInput();

  if ((inExt[0] == outExt[0])&&(inExt[1] == outExt[1])&&
      (inExt[2] == outExt[2])&&(inExt[3] == outExt[3])&&
      (inExt[4] == outExt[4])&&(inExt[5] == outExt[5])&&
      this->GetInput()->ShouldIReleaseData())
    {
    // pass the data
    output->GetPointData()->PassData(input->GetPointData());
    output->SetExtent(this->GetInput()->GetExtent());
    }
  else
    {
    output->SetExtent(output->GetUpdateExtent());
    output->AllocateScalars();
    this->CopyData(input,output);
    }
}
  
  

void vtkImageInPlaceFilter::CopyData(vtkImageData *inData,
				     vtkImageData *outData)
{
  int *outExt = this->GetOutput()->GetUpdateExtent();
  char *inPtr = (char *) inData->GetScalarPointerForExtent(outExt);
  char *outPtr = (char *) outData->GetScalarPointerForExtent(outExt);
  int rowLength, size;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;
  
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  
  // adjust increments for this loop
  inIncY = inIncY*size + rowLength;
  outIncY = outIncY*size + rowLength;
  inIncZ *= size;
  outIncZ *= size;
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr,rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}
