/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCorrelation.cxx
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

#include "vtkImageCorrelation.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageCorrelation* vtkImageCorrelation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCorrelation");
  if(ret)
    {
    return (vtkImageCorrelation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCorrelation;
}






//----------------------------------------------------------------------------
vtkImageCorrelation::vtkImageCorrelation()
{
  this->Dimensionality = 2;
}


//----------------------------------------------------------------------------
// Grow the output image 
void vtkImageCorrelation::ExecuteInformation(
                    vtkImageData **vtkNotUsed(inDatas), vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
  outData->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// Grow
void vtkImageCorrelation::ComputeInputUpdateExtent(int inExt[6], 
						   int outExt[6],
						   int whichInput)
{
  if (whichInput == 1)
    {
    // get the whole image for input 2
    memcpy(inExt,this->GetInput(whichInput)->GetWholeExtent(),6*sizeof(int));
    }
  else
    {
    // try to get all the data required to handle the boundaries
    // but limit to the whole extent
    int idx;
    int *i0WExtent = this->GetInput(0)->GetWholeExtent();
    int *i1WExtent = this->GetInput(1)->GetWholeExtent();
    memcpy(inExt,outExt,6*sizeof(int));
    for (idx = 0; idx < 3; idx++)
      {
      inExt[idx*2+1] = outExt[idx*2+1] +
	(i1WExtent[idx*2+1] - i1WExtent[idx*2]);

      // clip to whole extent
      if (inExt[idx*2+1] > i0WExtent[idx*2+1])
	{
	inExt[idx*2+1] = i0WExtent[idx*2+1];
	}
      }
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageCorrelationExecute(vtkImageCorrelation *self,
				       vtkImageData *in1Data, T *in1Ptr,
				       vtkImageData *in2Data, T *in2Ptr,
				       vtkImageData *outData, float *outPtr,
				       int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int in1IncX, in1IncY, in1IncZ;
  int in1CIncX, in1CIncY, in1CIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int *in2Extent;
  T *in2Ptr2, *in1Ptr2;
  int kIdxX, kIdxY, kIdxZ;
  int xKernMax, yKernMax, zKernMax;
  int maxIX, maxIY, maxIZ;
  int *wExtent;
  
  // find the region to loop over
  maxC = in1Data->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // get some other info we need
  in2Extent = self->GetInput2()->GetWholeExtent(); 
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, in1CIncX, in1CIncY, in1CIncZ);
  in1Data->GetIncrements(in1IncX, in1IncY, in1IncZ);
  in2Data->GetIncrements(in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // how far can we gon with input data
  // this may be farther that the outExt because of 
  // subpieces etc.
  wExtent = in1Data->GetExtent();
  maxIZ = wExtent[5] - outExt[4];
  maxIY = wExtent[3] - outExt[2];
  maxIX = wExtent[1] - outExt[0];
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    // how much of kernel to use
    zKernMax = maxIZ - idxZ;
    if (zKernMax > in2Extent[5])
      {
      zKernMax = in2Extent[5];
      }
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
      yKernMax = maxIY - idxY;
      if (yKernMax > in2Extent[3])
	{
	yKernMax = in2Extent[3];
	}
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// determine the extent of input 1 that contributes to this pixel
	*outPtr = 0.0;
	xKernMax = maxIX - idxX;
	if (xKernMax > in2Extent[1])
	  {
	  xKernMax = in2Extent[1];
	  }

	// sumation
	for (kIdxZ = 0; kIdxZ <= zKernMax; kIdxZ++)
	  {
	  for (kIdxY = 0; kIdxY <= yKernMax; kIdxY++)
	    {
	    in1Ptr2 = in1Ptr + kIdxY*in1IncY + kIdxZ*in1IncZ;
	    in2Ptr2 = in2Ptr + kIdxY*in2IncY + kIdxZ*in2IncZ;
	    for (kIdxX = 0; kIdxX <= xKernMax; kIdxX++)
	      {
	      for (idxC = 0; idxC < maxC; idxC++)
		{
		*outPtr = *outPtr + (float)((*in1Ptr2) * (*in2Ptr2));
		in1Ptr2++;
		in2Ptr2++;
		}
	      }
	    }
	  }
	in1Ptr += maxC;
	outPtr++;
	}
      in1Ptr += in1CIncY;
      outPtr += outIncY;
      }
    in1Ptr += in1CIncZ;
    outPtr += outIncZ;
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageCorrelation::ThreadedExecute(vtkImageData **inData, 
					  vtkImageData *outData,
					  int outExt[6], int id)
{
  int *in2Extent;
  void *in1Ptr;
  void *in2Ptr;
  float *outPtr;
  
  vtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);
  
  if (inData[0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }
  if (inData[1] == NULL)
    {
    vtkErrorMacro(<< "Input " << 1 << " must be specified.");
    return;
    }
  in2Extent = this->GetInput(1)->GetWholeExtent();
  in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  in2Ptr = inData[1]->GetScalarPointerForExtent(in2Extent);
  outPtr = (float *)outData->GetScalarPointerForExtent(outExt);

  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != inData[1]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << 
    inData[0]->GetScalarType() << " and input2 ScalarType " <<
    inData[1]->GetScalarType() << ", should match");
    return;
    }
  
  // input depths must match
  if (inData[0]->GetNumberOfScalarComponents() != 
      inData[1]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input depths must match");
    return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageCorrelationExecute, this, inData[0], 
                      (VTK_TT *)(in1Ptr), inData[1], (VTK_TT *)(in2Ptr), 
                      outData, outPtr, outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageCorrelation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

