/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogarithmicScale.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>

#include "vtkImageLogarithmicScale.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageLogarithmicScale* vtkImageLogarithmicScale::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageLogarithmicScale");
  if(ret)
    {
    return (vtkImageLogarithmicScale*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageLogarithmicScale;
}






//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageLogarithmicScale::vtkImageLogarithmicScale()
{
  this->Constant = 10.0;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageLogarithmicScaleExecute(vtkImageLogarithmicScale *self,
					    vtkImageData *inData, T *inPtr,
					    vtkImageData *outData, T *outPtr, 
					    int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  float c;

  c = self->GetConstant();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
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
      for (idxR = 0; idxR < rowLength; idxR++)
	{

	// Pixel operation
	if (*inPtr > 0)
	  {
	  *outPtr = (T)(c*log((double)(*inPtr)+1.0));
	  }
	else
	  {
	  *outPtr = (T)(-c*log(1.0-(double)(*inPtr)));
	  }

	outPtr++;
	inPtr++;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageLogarithmicScale::ThreadedExecute(vtkImageData *inData, 
					vtkImageData *outData,
					int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
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
    case VTK_DOUBLE:
      vtkImageLogarithmicScaleExecute(this, inData, (double *)(inPtr), 
			       outData, (double *)(outPtr),outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageLogarithmicScaleExecute(this, inData, (float *)(inPtr), 
			       outData, (float *)(outPtr),outExt, id);
      break;
    case VTK_LONG:
      vtkImageLogarithmicScaleExecute(this, inData, (long *)(inPtr), 
			       outData, (long *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageLogarithmicScaleExecute(this, inData, 
			       (unsigned long *)(inPtr), 
			       outData, (unsigned long *)(outPtr),outExt, id);
      break;
    case VTK_INT:
      vtkImageLogarithmicScaleExecute(this, inData, (int *)(inPtr), 
			       outData, (int *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageLogarithmicScaleExecute(this, inData, 
			       (unsigned int *)(inPtr), 
			       outData, (unsigned int *)(outPtr),outExt, id);
      break;
    case VTK_SHORT:
      vtkImageLogarithmicScaleExecute(this, inData, (short *)(inPtr), 
			       outData, (short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageLogarithmicScaleExecute(this, inData, 
			       (unsigned short *)(inPtr), 
			       outData, (unsigned short *)(outPtr),outExt, id);
      break;
    case VTK_CHAR:
      vtkImageLogarithmicScaleExecute(this, inData, (char *)(inPtr), 
			       outData, (char *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageLogarithmicScaleExecute(this, inData, 
			       (unsigned char *)(inPtr), 
			       outData, (unsigned char *)(outPtr),outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageLogarithmicScale::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "Constant: " << this->Constant << "\n";
}

