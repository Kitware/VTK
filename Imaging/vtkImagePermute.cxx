/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePermute.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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

#include "vtkImagePermute.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImagePermute* vtkImagePermute::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImagePermute");
  if(ret)
    {
    return (vtkImagePermute*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImagePermute;
}




//----------------------------------------------------------------------------
vtkImagePermute::vtkImagePermute()
{
  this->FilteredAxes[0] = 0;
  this->FilteredAxes[1] = 1;
  this->FilteredAxes[2] = 2;
}

//----------------------------------------------------------------------------
void vtkImagePermute::ExecuteInformation(vtkImageData *inData, 
					 vtkImageData *outData) 
{
  int idx, axis;
  int ext[6];
  float spacing[3];
  float origin[3];
  float *inOrigin;
  float *inSpacing;
  int *inExt;
  
  inExt = inData->GetWholeExtent();
  inSpacing = inData->GetSpacing();
  inOrigin = inData->GetOrigin();
  
  for (idx = 0; idx < 3; ++idx)
    {
    axis = this->FilteredAxes[idx];
    origin[idx] = inOrigin[axis];
    spacing[idx] = inSpacing[axis];
    ext[idx*2] = inExt[axis*2];
    ext[idx*2+1] = inExt[axis*2+1];
    }
  
  outData->SetWholeExtent(ext);
  outData->SetSpacing(spacing);
  outData->SetOrigin(origin);
}


//----------------------------------------------------------------------------
void vtkImagePermute::ComputeInputUpdateExtent(int inExt[6], 
					       int outExt[6])
{
  int idx, axis;

  for (idx = 0; idx < 3; ++idx)
    {
    axis = this->FilteredAxes[idx];
    inExt[axis*2] = outExt[idx*2];
    inExt[axis*2+1] = outExt[idx*2+1];
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImagePermuteExecute(vtkImagePermute *self,
				   vtkImageData *inData, T *inPtr,
				   vtkImageData *outData, T *outPtr,
				   int outExt[6], int id)
{
  int idxX, idxY, idxZ, idxC;
  int maxX, maxY, maxZ;
  int inInc[3];
  int inInc0, inInc1, inInc2;
  int outIncX, outIncY, outIncZ;
  T *inPtr0, *inPtr1, *inPtr2;
  unsigned long count = 0;
  unsigned long target;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outIncX = inData->GetNumberOfScalarComponents();
  
  // adjust the increments for the permute
  int *fe = self->GetFilteredAxes();
  inInc[0] = inInc0;
  inInc[1] = inInc1;
  inInc[2] = inInc2;
  inInc0 = inInc[fe[0]];
  inInc1 = inInc[fe[1]];
  inInc2 = inInc[fe[2]];

  // turn into a 'continuous' increment
  inInc0 -= outIncX;
    
  // Loop through ouput pixels
  inPtr2 = inPtr;
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtr1 = inPtr2;
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      inPtr0 = inPtr1;
      if (outIncX == 1) // optimization for single component
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  // Pixel operation
	  *outPtr++ = *inPtr0++;
	  inPtr0 += inInc0;
	  }
	}
      else // multiple components
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  // Pixel operation
	  for (idxC = 0; idxC < outIncX; idxC++)
	    { 
	    *outPtr++ = *inPtr0++;
	    }
	  inPtr0 += inInc0;
	  }
	}
      outPtr += outIncY;
      inPtr1 += inInc1;
      }
    outPtr += outIncZ;
    inPtr2 += inInc2;
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImagePermute::ThreadedExecute(vtkImageData *inData, 
				      vtkImageData *outData,
				      int outExt[6], int id)
{
  int inExt[6];
  
  this->ComputeInputUpdateExtent(inExt,outExt);
  
  void *inPtr = inData->GetScalarPointerForExtent(inExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImagePermuteExecute, this, inData, (VTK_TT *)(inPtr), 
                      outData, (VTK_TT *)(outPtr),outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImagePermute::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

    os << indent << "FilteredAxes: ( "
     << this->FilteredAxes[0] << ", "
     << this->FilteredAxes[1] << ", "
     << this->FilteredAxes[2] << " )\n";
}

