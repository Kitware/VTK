/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConstantPad.cxx
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
#include "vtkImageConstantPad.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageConstantPad* vtkImageConstantPad::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageConstantPad");
  if(ret)
    {
    return (vtkImageConstantPad*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageConstantPad;
}






//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageConstantPad::vtkImageConstantPad()
{
  this->Constant = 0.0;
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageConstantPadExecute(vtkImageConstantPad *self,
				       vtkImageData *inData, T *inPtr,
				       vtkImageData *outData, T *outPtr,
				       int outExt[6], int inExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  T constant;
  int inMinX, inMaxX, inMaxC;
  constant = (T)(self->GetConstant());
  int state0, state1, state2, state3;
  unsigned long count = 0;
  unsigned long target;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  inMaxC = inData->GetNumberOfScalarComponents();
  inMinX = inExt[0] - outExt[0];
  inMaxX = inExt[1] - outExt[0];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = outExt[4]; idxZ <= outExt[5]; idxZ++)
    {
    state3 = (idxZ < inExt[4] || idxZ > inExt[5]);
    for (idxY = outExt[2]; !self->AbortExecute && idxY <= outExt[3]; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      state2 = (state3 || idxY < inExt[2] || idxY > inExt[3]);
      if ((maxC == inMaxC) && (maxC == 1))
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  state1 = (state2 || idxX < inMinX || idxX > inMaxX);
	  if (state1)
	    {
	    *outPtr = constant;
	    }
	  else
	    {
	    *outPtr = *inPtr;
	    inPtr++;
	    }
	  outPtr++;
	  }
	}
      else
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  state1 = (state2 || idxX < inMinX || idxX > inMaxX);
	  for (idxC = 0; idxC < maxC; idxC++)
	    {
	    // Pixel operation
	    // Copy Pixel
	    state0 = (state1 || idxC >= inMaxC);
	    if (state0)
	      {
	      *outPtr = constant;
	      }
	    else
	      {
	      *outPtr = *inPtr;
	      inPtr++;
	      }
	    outPtr++;
	    }
	  }
	}
      outPtr += outIncY;
      if (!state2)
	{
	inPtr += inIncY;
	}
      }
    outPtr += outIncZ;
    if (!state3)
      {
      inPtr += inIncZ;
      }
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageConstantPad::ThreadedExecute(vtkImageData *inData, 
					vtkImageData *outData,
					int outExt[6], int id)
{
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
  
  // need to get the correct pointer for the input data
  int inExt[6];
  this->ComputeInputUpdateExtent(inExt,outExt);
  void *inPtr = inData->GetScalarPointerForExtent(inExt);

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro8(vtkImageConstantPadExecute, this, 
                      inData, (VTK_TT *)(inPtr), outData, 
                      (VTK_TT *)(outPtr), outExt, inExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageConstantPad::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImagePadFilter::PrintSelf(os,indent);

  os << indent << "Constant: " << this->Constant << "\n";

}

