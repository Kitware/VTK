/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.cxx
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

#include "vtkImagePadFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImagePadFilter* vtkImagePadFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImagePadFilter");
  if(ret)
    {
    return (vtkImagePadFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImagePadFilter;
}






//----------------------------------------------------------------------------
// Constructor sets default values
vtkImagePadFilter::vtkImagePadFilter()
{
  int idx;

  // Initialize output image extent to INVALID
  for (idx = 0; idx < 3; ++idx)
    {
    this->OutputWholeExtent[idx * 2] = 0;
    this->OutputWholeExtent[idx * 2 + 1] = -1;
    }
  // Set Output numberOfScalarComponents to INVALID
  this->OutputNumberOfScalarComponents = -1;
}

//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
      }
    }

  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkImagePadFilter::SetOutputWholeExtent(int minX, int maxX, 
					     int minY, int maxY,
					     int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImagePadFilter::GetOutputWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Just change the Image extent.
void vtkImagePadFilter::ExecuteInformation(vtkImageData *inData, 
					   vtkImageData *outData)
{
  if (this->OutputWholeExtent[0] > this->OutputWholeExtent[1])
    {
    // invalid setting, it has not been set, so default to whole Extent
    inData->GetWholeExtent(this->OutputWholeExtent);
    }
  outData->SetWholeExtent(this->OutputWholeExtent);
  
  if (this->OutputNumberOfScalarComponents < 0)
    {
    // invalid setting, it has not been set, so default to input.
    this->OutputNumberOfScalarComponents 
      = inData->GetNumberOfScalarComponents();
    }
  outData->SetNumberOfScalarComponents(this->OutputNumberOfScalarComponents);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImagePadFilter::ComputeInputUpdateExtent(int inExt[6], 
						 int outExt[6])
{
  int idx;
  int *wholeExtent;
  
  // handle XYZ
  memcpy(inExt,outExt,sizeof(int)*6);
  
  wholeExtent = this->GetInput()->GetWholeExtent();
  // Clip
  for (idx = 0; idx < 3; ++idx)
    {
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }
    if (inExt[idx*2] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2] = wholeExtent[idx*2 + 1];
      }
    if (inExt[idx*2+1] < wholeExtent[idx*2])
      {
      inExt[idx*2+1] = wholeExtent[idx*2];
      }
    if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
      inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
}

void vtkImagePadFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "OutputNumberOfScalarComponents: " 
     << this->OutputNumberOfScalarComponents << "\n";
}


