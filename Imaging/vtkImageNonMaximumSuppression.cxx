/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.cxx
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

#include "vtkImageNonMaximumSuppression.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageNonMaximumSuppression* vtkImageNonMaximumSuppression::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageNonMaximumSuppression");
  if(ret)
    {
    return (vtkImageNonMaximumSuppression*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageNonMaximumSuppression;
}





//----------------------------------------------------------------------------
// Construct an instance of vtkImageNonMaximumSuppression fitler.
vtkImageNonMaximumSuppression::vtkImageNonMaximumSuppression()
{
  this->Dimensionality= 2;
  this->HandleBoundaries = 1;
}

//----------------------------------------------------------------------------
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageNonMaximumSuppression::ExecuteInformation(vtkImageData **inDatas,
						       vtkImageData *outData)
{
  int extent[6];
  int idx;
  
  inDatas[0]->GetWholeExtent(extent);
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->Dimensionality; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2+1] -= 1;
      }
    }

  
  outData->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
void vtkImageNonMaximumSuppression::ComputeInputUpdateExtent(int inExt[6], 
							     int outExt[6],
							     int whichInput)
{
  int *wholeExtent;
  int idx;

  wholeExtent = this->GetInput(0)->GetWholeExtent();
  memcpy(inExt,outExt,6*sizeof(int));
  if (whichInput == 1)
    {
    return;
    }
  
  // grow input image extent for input 0
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    inExt[idx*2] -= 1;
    inExt[idx*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with whole extent if we hanlde boundaries.
      if (inExt[idx*2] < wholeExtent[idx*2])
	{
	inExt[idx*2] = wholeExtent[idx*2];
	}
      if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
	{
	inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
	}
      }
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageNonMaximumSuppressionExecute(vtkImageNonMaximumSuppression *self,
						 vtkImageData *in1Data, 
						 T *in1Ptr,
						 vtkImageData *in2Data, 
						 T *in2Ptr,
						 vtkImageData *outData, 
						 T *outPtr,
						 int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;
  float d, normalizeFactor, vector[3], *ratio;
  int neighborA, neighborB;
  int *wholeExtent, *inIncs;
  int axesNum;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();
  // get some other info we need
  inIncs = in1Data->GetIncrements(); 
  wholeExtent = in1Data->GetExtent(); 
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  
  // Gradient is computed with data spacing (world coordinates)
  ratio = in2Data->GetSpacing();
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useZMin = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useZMax = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      useYMin = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useYMax = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	useXMin = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
	useXMax = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];

	// calculate the neighbors
	d = vector[0] = (float)*in2Ptr * ratio[0];
	normalizeFactor = (d * d);
	d = vector[1] = (float)in2Ptr[1] * ratio[1];
	normalizeFactor += (d * d);
	if (axesNum == 3)
	  {
	  d = vector[2] = (float)in2Ptr[2] * ratio[2];
	  normalizeFactor += (d * d);
	  }
	if (normalizeFactor)
	  {
	  normalizeFactor = 1.0 / sqrt(normalizeFactor);
	  }
	// Vector points positive along this idx?
	// (can point along multiple axes)
	d = vector[0] * normalizeFactor;  
	
	if (d > 0.5)  
	  {
	  neighborA = useXMax;
	  neighborB = useXMin;
	  }
	else if (d < -0.5)
	  {
	  neighborB = useXMax;
	  neighborA = useXMin;
	  }
	else
	  {
	  neighborA = 0;
	  neighborB = 0;
	  }
	d = vector[1] * normalizeFactor;  
	if (d > 0.5)  
	  {
	  neighborA += useYMax;
	  neighborB += useYMin;
	  }
	else if (d < -0.5)
	  {
	  neighborB += useYMax;
	  neighborA += useYMin;
	  }
	if (axesNum == 3)
	  {
	  d = vector[2] * normalizeFactor;  
	  if (d > 0.5)  
	    {
	    neighborA += useZMax;
	    neighborB += useZMin;
	    }
	  else if (d < -0.5)
	    {
	    neighborB += useZMax;
	    neighborA += useZMin;
	    }
	  }
	
	// now process the components
	for (idxC = 0; idxC < maxC; idxC++)
	  {
	  // Pixel operation
	  // Set Output Magnitude
	  if (in1Ptr[neighborA] > *in1Ptr || in1Ptr[neighborB] > *in1Ptr)
	    {
	    *outPtr = 0;
	    }
	  else
	    {
	    *outPtr = *in1Ptr;
	    // also check for them being equal is neighbor with larger ptr
	    if ((neighborA > neighborB)&&(in1Ptr[neighborA] == *in1Ptr))
	      {
	      *outPtr = 0;
	      }
	    else if ((neighborB > neighborA)&&(in1Ptr[neighborB] == *in1Ptr))
	      {
	      *outPtr = 0;
	      }
	    }
	  outPtr++;
	  in1Ptr++;
	  }
	in2Ptr += axesNum;
	}
      outPtr += outIncY;
      in1Ptr += inIncY;
      in2Ptr += in2IncY;
      }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    in2Ptr += in2IncZ;
    }
}

	  

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageNonMaximumSuppression::ThreadedExecute(vtkImageData **inData, 
						    vtkImageData *outData,
						    int outExt[6], int id)
{
  void *in1Ptr;
  void *in2Ptr;
  void *outPtr;
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  

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
  in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType() ||
      inData[1]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << 
    inData[0]->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageNonMaximumSuppressionExecute, this, inData[0], 
                      (VTK_TT *)(in1Ptr),inData[1], (VTK_TT *)(in2Ptr), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


void vtkImageNonMaximumSuppression::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";

  os << indent << "HandleBoundaries: " << (this->HandleBoundaries ? "On\n" : "Off\n");
}


