/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDivergence.cxx
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
#include <math.h>

#include "vtkImageDivergence.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageDivergence* vtkImageDivergence::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDivergence");
  if(ret)
    {
    return (vtkImageDivergence*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDivergence;
}


//----------------------------------------------------------------------------
// This method tells the superclass that the first axis will collapse.
void vtkImageDivergence::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
					    vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
void vtkImageDivergence::ComputeInputUpdateExtent(int inExt[6], 
						  int outExt[6])
{
  int idx;
  int *wholeExtent;
  int dimensionality = this->GetInput()->GetNumberOfScalarComponents();
  
  if (dimensionality > 3)
    {
    vtkErrorMacro("Divergence has to have dimensionality <= 3");
    dimensionality = 3;
    }
  
  // handle XYZ
  memcpy(inExt,outExt,sizeof(int)*6);
  
  wholeExtent = this->GetInput()->GetWholeExtent();
  // update and Clip
  for (idx = 0; idx < dimensionality; ++idx)
    {
    --inExt[idx*2];
    ++inExt[idx*2+1];
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

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageDivergenceExecute(vtkImageDivergence *self,
			       vtkImageData *inData, T *inPtr,
			       vtkImageData *outData, T *outPtr,
			       int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int *wholeExtent, *inIncs;
  float r[3], d, sum;
  int useMin[3], useMax[3];
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  if (maxC > 3)
    {
    vtkGenericWarningMacro("Dimensionality must be less than or equal to 3");
    maxC = 3;
    }
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // The spacing is important for computing the gradient.
  // central differences (2 * ratio).
  // Negative because below we have (min - max) for dx ...
  inData->GetSpacing(r);
  r[0] = -0.5 / r[0];
  r[1] = -0.5 / r[1];
  r[2] = -0.5 / r[2];

  // get some other info we need
  inIncs = inData->GetIncrements(); 
  wholeExtent = inData->GetExtent(); 

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useMin[2] = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useMax[2] = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
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
      useMin[1] = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useMax[1] = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	useMin[0] = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
	useMax[0] = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];
	sum = 0.0;
	for (idxC = 0; idxC < maxC; idxC++)
	  {
	  // do X axis
	  d = (float)(inPtr[useMin[idxC]]);
	  d -= (float)(inPtr[useMax[idxC]]);
	  sum = d * r[idxC];
	  inPtr++;
	  }
	*outPtr = (T)sum;
	outPtr++;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

  
//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageDivergence::ThreadedExecute(vtkImageData *inData, 
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
    vtkTemplateMacro7(vtkImageDivergenceExecute, this, inData, 
                      (VTK_TT *)(inPtr), outData, (VTK_TT *)(outPtr), 
                      outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

