/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTranslateExtent.cxx
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

#include "vtkImageTranslateExtent.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageTranslateExtent* vtkImageTranslateExtent::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageTranslateExtent");
  if(ret)
    {
    return (vtkImageTranslateExtent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageTranslateExtent;
}





//----------------------------------------------------------------------------
vtkImageTranslateExtent::vtkImageTranslateExtent()
{
  int idx;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Translation[idx]  = 0;
    }
}


//----------------------------------------------------------------------------
void vtkImageTranslateExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "Translation: (" << this->Translation[0]
     << "," << this->Translation[1] << "," << this->Translation[2] << endl;
}
  



//----------------------------------------------------------------------------
// Change the WholeExtent
void vtkImageTranslateExtent::ExecuteInformation(vtkImageData *inData, 
						 vtkImageData *outData)
{
  int idx, extent[6];
  float *spacing, origin[3];
  
  inData->GetWholeExtent(extent);
  inData->GetOrigin(origin);
  spacing = inData->GetSpacing();

  // TranslateExtent the OutputWholeExtent with the input WholeExtent
  for (idx = 0; idx < 3; ++idx)
    {
    // change extent
    extent[2*idx] += this->Translation[idx];
    extent[2*idx+1] += this->Translation[idx];
    // change origin so the data does not shift
    origin[idx] -= (float)(this->Translation[idx]) * spacing[idx];
    }
  
  outData->SetWholeExtent(extent);
  outData->SetOrigin(origin);
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageTranslateExtent::ExecuteData(vtkDataObject *data)
{
  vtkImageData *inData = this->GetInput();
  vtkImageData *outData = (vtkImageData *)(data);
  int extent[6];
  
  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (int i = 0; i < 3; ++i)
    {
    extent[i*2] += this->Translation[i];
    extent[i*2+1] += this->Translation[i];
    }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());
}

//----------------------------------------------------------------------------
void vtkImageTranslateExtent::ComputeInputUpdateExtent(int extent[6], 
						       int inExtent[6])
{
  extent[0] = inExtent[0] - this->Translation[0];
  extent[1] = inExtent[1] - this->Translation[0];
  extent[2] = inExtent[2] - this->Translation[1];
  extent[3] = inExtent[3] - this->Translation[1];
  extent[4] = inExtent[4] - this->Translation[2];
  extent[5] = inExtent[5] - this->Translation[2];
}
