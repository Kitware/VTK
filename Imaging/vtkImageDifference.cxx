/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.cxx
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
#include "vtkImageDifference.h"
#include "stdlib.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageDifference* vtkImageDifference::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDifference");
  if(ret)
    {
    return (vtkImageDifference*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDifference;
}




// Construct object to extract all of the input data.
vtkImageDifference::vtkImageDifference()
{
  int i;
  for ( i = 0; i < this->NumberOfThreads; i++ )
    {
    this->ErrorPerThread[i] = 0;
    this->ThresholdedErrorPerThread[i] = 0.0;
    }
  this->Threshold = 16;
  this->AllowShift = 1;
  this->Averaging = 1;
}



// not so simple macro for calculating error
#define vtkImageDifferenceComputeError(c1,c2) \
  r1 = abs(((int)(c1)[0] - (int)(c2)[0])); \
  g1 = abs(((int)(c1)[1] - (int)(c2)[1])); \
  b1 = abs(((int)(c1)[2] - (int)(c2)[2]));\
  if ((r1+g1+b1) < (tr+tg+tb)) { tr = r1; tg = g1; tb = b1; } \
  if (this->Averaging && \
      (idx0 > inMinX + 1) && (idx0 < inMaxX - 1) && \
      (idx1 > inMinY + 1) && (idx1 < inMaxY - 1)) \
    {\
  ar1 = (int)(c1)[0]; \
  ag1 = (int)(c1)[1]; \
  ab1 = (int)(c1)[2]; \
  ar2 = (int)(c2)[0] + (int)(c2 - in2Inc0)[0] + (int)(c2 + in2Inc0)[0] + \
        (int)(c2-in2Inc1)[0] + (int)(c2-in2Inc1-in2Inc0)[0] + (int)(c2-in2Inc1+in2Inc0)[0] + \
        (int)(c2+in2Inc1)[0] + (int)(c2+in2Inc1-in2Inc0)[0] + (int)(c2+in2Inc1+in2Inc0)[0]; \
  ag2 = (int)(c2)[1] + (int)(c2 - in2Inc0)[1] + (int)(c2 + in2Inc0)[1] + \
        (int)(c2-in2Inc1)[1] + (int)(c2-in2Inc1-in2Inc0)[1] + (int)(c2-in2Inc1+in2Inc0)[1] + \
        (int)(c2+in2Inc1)[1] + (int)(c2+in2Inc1-in2Inc0)[1] + (int)(c2+in2Inc1+in2Inc0)[1]; \
  ab2 = (int)(c2)[2] + (int)(c2 - in2Inc0)[2] + (int)(c2 + in2Inc0)[2] + \
        (int)(c2-in2Inc1)[2] + (int)(c2-in2Inc1-in2Inc0)[2] + (int)(c2-in2Inc1+in2Inc0)[2] + \
        (int)(c2+in2Inc1)[2] + (int)(c2+in2Inc1-in2Inc0)[2] + (int)(c2+in2Inc1+in2Inc0)[2]; \
  r1 = abs(ar1 - ar2/9); \
  g1 = abs(ag1 - ag2/9); \
  b1 = abs(ab1 - ab2/9); \
  if ((r1+g1+b1) < (tr+tg+tb)) { tr = r1; tg = g1; tb = b1; } \
  ar1 = (int)(c1)[0] + (int)(c1 - in1Inc0)[0] + (int)(c1 + in1Inc0)[0] + \
        (int)(c1-in1Inc1)[0] + (int)(c1-in1Inc1-in1Inc0)[0] + (int)(c1-in1Inc1+in1Inc0)[0] + \
        (int)(c1+in1Inc1)[0] + (int)(c1+in1Inc1-in1Inc0)[0] + (int)(c1+in1Inc1+in1Inc0)[0]; \
  ag1 = (int)(int)(c1)[1] + (int)(c1 - in1Inc0)[1] + (int)(c1 + in1Inc0)[1] + \
        (int)(c1-in1Inc1)[1] + (int)(c1-in1Inc1-in1Inc0)[1] + (int)(c1-in1Inc1+in1Inc0)[1] + \
        (int)(c1+in1Inc1)[1] + (int)(c1+in1Inc1-in1Inc0)[1] + (int)(c1+in1Inc1+in1Inc0)[1]; \
  ab1 = (int)(int)(c1)[2] + (int)(c1 - in1Inc0)[2] + (int)(c1 + in1Inc0)[2] + \
        (int)(c1-in1Inc1)[2] + (int)(c1-in1Inc1-in1Inc0)[2] + (int)(c1-in1Inc1+in1Inc0)[2] + \
        (int)(c1+in1Inc1)[2] + (int)(c1+in1Inc1-in1Inc0)[2] + (int)(c1+in1Inc1+in1Inc0)[2]; \
  r1 = abs(ar1/9 - ar2/9); \
  g1 = abs(ag1/9 - ag2/9); \
  b1 = abs(ab1/9 - ab2/9); \
  if ((r1+g1+b1) < (tr+tg+tb)) { tr = r1; tg = g1; tb = b1; } \
  ar2 = (int)(c2)[0]; \
  ag2 = (int)(c2)[1]; \
  ab2 = (int)(c2)[2]; \
  r1 = abs(ar1/9 - ar2); \
  g1 = abs(ag1/9 - ag2); \
  b1 = abs(ab1/9 - ab2); \
  if ((r1+g1+b1) < (tr+tg+tb)) { tr = r1; tg = g1; tb = b1; } \
    }




//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
void vtkImageDifference::ComputeInputUpdateExtent(int inExt[6],
							  int outExt[6],
							  int whichInput)
{
  int *wholeExtent;
  int idx;

  wholeExtent = this->GetInput(whichInput)->GetWholeExtent();
  
  memcpy(inExt,outExt,6*sizeof(int));
  
  // grow input whole extent.
  for (idx = 0; idx < 2; ++idx)
    {
    inExt[idx*2] -= 2;
    inExt[idx*2+1] += 2;

    // we must clip extent with whole extent is we hanlde boundaries.
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

//----------------------------------------------------------------------------
void vtkImageDifference::ThreadedExecute(vtkImageData **inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  unsigned char *in1Ptr0, *in1Ptr1, *in1Ptr2;
  unsigned char *in2Ptr0, *in2Ptr1, *in2Ptr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  int min0, max0, min1, max1, min2, max2;
  int idx0, idx1, idx2;
  int in1Inc0, in1Inc1, in1Inc2;
  int in2Inc0, in2Inc1, in2Inc2;
  int outInc0, outInc1, outInc2;
  int tr, tg, tb, r1, g1, b1;
  int ar1, ag1, ab1, ar2, ag2, ab2;
  int inMinX, inMaxX, inMinY, inMaxY;
  int *inExt;
  int matched;
  unsigned long count = 0;
  unsigned long target;
  
  this->ErrorPerThread[id] = 0;
  this->ThresholdedErrorPerThread[id] = 0;
  
  if (inData[0] == NULL || inData[1] == NULL || outData == NULL)
    {
    if (!id)
      {
      vtkErrorMacro(<< "Execute: Missing data");
      }
    this->ErrorPerThread[id] = 1000;
    this->ThresholdedErrorPerThread[id] = 1000;
    return;
    }

  if (inData[0]->GetNumberOfScalarComponents() != 3 ||
      inData[1]->GetNumberOfScalarComponents() != 3 ||
      outData->GetNumberOfScalarComponents() != 3)
    {
    if (!id)
      {
      vtkErrorMacro(<< "Execute: Expecting 3 components (RGB)");
      }
    this->ErrorPerThread[id] = 1000;
    this->ThresholdedErrorPerThread[id] = 1000;
    return;
    }
    
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != VTK_UNSIGNED_CHAR || 
      inData[1]->GetScalarType() != VTK_UNSIGNED_CHAR || 
      outData->GetScalarType() != VTK_UNSIGNED_CHAR)
      {
      if (!id)
	{
	vtkErrorMacro(<< "Execute: All ScalarTypes must be unsigned char");
	}
      this->ErrorPerThread[id] = 1000;
      this->ThresholdedErrorPerThread[id] = 1000;
      return;
      }
  
  in1Ptr2 = (unsigned char *) inData[0]->GetScalarPointerForExtent(outExt);  
  in2Ptr2 = (unsigned char *) inData[1]->GetScalarPointerForExtent(outExt);  
  outPtr2 = (unsigned char *) outData->GetScalarPointerForExtent(outExt);  

  inData[0]->GetIncrements(in1Inc0, in1Inc1, in1Inc2);
  inData[1]->GetIncrements(in2Inc0, in2Inc1, in2Inc2);
  outData->GetIncrements(outInc0, outInc1, outInc2);
  
  min0 = outExt[0];  max0 = outExt[1];
  min1 = outExt[2];  max1 = outExt[3];
  min2 = outExt[4];  max2 = outExt[5];
  
  inExt = inData[0]->GetExtent();
  // we set min and Max to be one pixel in from actual values to support 
  // the 3x3 averaging we do
  inMinX = inExt[0]; inMaxX = inExt[1];
  inMinY = inExt[2]; inMaxY = inExt[3];

  target = (unsigned long)((max2 - min2 +1)*(max1 - min1 + 1)/50.0);
  target++;

  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    in1Ptr1 = in1Ptr2;
    in2Ptr1 = in2Ptr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; !this->AbortExecute && idx1 <= max1; ++idx1)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  this->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      in1Ptr0 = in1Ptr1;
      in2Ptr0 = in2Ptr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	tr = 1000;
	tg = 1000;
	tb = 1000;
      
	/* check the exact match pixel */
	vtkImageDifferenceComputeError(in1Ptr0,in2Ptr0);
	
	// do a quick check to see if this match is exact, if so
	// we can save some seious time by skipping the eight
	// connected neighbors
	matched = 0;
	if ((tr <= 0)&&(tg <= 0)&&(tb <= 0))
	  {
	  matched = 1;
	  }
	
	/* If AllowShift, then we examine neighboring pixels to 
	   find the least difference.  This feature is used to 
	   allow images to shift slightly between different graphics
	   systems, like between opengl and starbase. */
	if (!matched && this->AllowShift) 
	  {
	  /* lower row */
 	  if (idx1 > inMinY)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0-in1Inc1,in2Ptr0);
	    if (idx0 > inMinX)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0-in1Inc0-in1Inc1,in2Ptr0);
	      }
	    if (idx0 < inMaxX)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0+in1Inc0-in1Inc1,in2Ptr0);
	      }
	    }
	  /* middle row (center already considered) */
	  if (idx0 > inMinX)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0-in1Inc0,in2Ptr0);
	    }
	  if (idx0 < inMaxX)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0+in1Inc0,in2Ptr0);
	    }
	  /* upper row */
	  if (idx1 < inMaxY)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0+in1Inc1,in2Ptr0);
	    if (idx0 > inMinX)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0-in1Inc0+in1Inc1,in2Ptr0);
	      }
	    if (idx0 < inMaxX)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0+in1Inc0+in1Inc1,in2Ptr0);
	      }
	    }
	  }
	
	this->ErrorPerThread[id] = this->ErrorPerThread[id] + (tr + tg + tb)/(3.0*255);
	tr -= this->Threshold;
	if (tr < 0)
	  {
	  tr = 0;
	  }
	tg -= this->Threshold;
	if (tg < 0)
	  {
	  tg = 0;
	  }
	tb -= this->Threshold;
	if (tb < 0)
	  {
	  tb = 0;
	  }
	*outPtr0++ = (unsigned char)tr;
	*outPtr0++ = (unsigned char)tg;
	*outPtr0++ = (unsigned char)tb;
	this->ThresholdedErrorPerThread[id] += (tr + tg + tb)/(3.0*255.0);

	in1Ptr0 += in1Inc0;
	in2Ptr0 += in2Inc0;
	}
      outPtr1 += outInc1;
      in1Ptr1 += in1Inc1;
      in2Ptr1 += in2Inc1;
      }
    outPtr2 += outInc2;
    in1Ptr2 += in1Inc2;
    in2Ptr2 += in2Inc2;
    }
}

//----------------------------------------------------------------------------
// Make sure both the inputs are the same size. Doesn't really change 
// the output. Just performs a sanity check
void vtkImageDifference::ExecuteInformation(vtkImageData **inputs,
					    vtkImageData *vtkNotUsed(output))
{
  int *in1Ext, *in2Ext;
  
  // Make sure the Input has been set.
  // we require that input 1 be set.
  if ( ! inputs[0] || ! inputs[1])
    {
    vtkErrorMacro(<< "ExecuteInformation: Input is not set.");
    return;
    }
  
  in1Ext = inputs[0]->GetWholeExtent();
  in2Ext = inputs[1]->GetWholeExtent();

  if (in1Ext[0] != in2Ext[0] || in1Ext[1] != in2Ext[1] || 
      in1Ext[2] != in2Ext[2] || in1Ext[3] != in2Ext[3] || 
      in1Ext[4] != in2Ext[4] || in1Ext[5] != in2Ext[5])
    {
    for (int i = 0; i < this->NumberOfThreads; i++)
      {
      this->ErrorPerThread[i] = 1000;
      this->ThresholdedErrorPerThread[i] = 1000;
      }
    vtkErrorMacro("ExecuteInformation: Input are not the same size.");
    return;
    }
}

float vtkImageDifference::GetError()
{
  float error = 0.0;
  int i;

  for ( i= 0; i < this->NumberOfThreads; i++ )
    {
    error += this->ErrorPerThread[i];
    }

  return error;
}

float vtkImageDifference::GetThresholdedError()
{
  float error = 0.0;
  int i;

  for ( i= 0; i < this->NumberOfThreads; i++ )
    {
    error += this->ThresholdedErrorPerThread[i];
    }

  return error;
}

void vtkImageDifference::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;

  vtkImageTwoInputFilter::PrintSelf(os,indent);

  for ( i= 0; i < this->NumberOfThreads; i++ )
    {
    os << indent << "Error for thread " << i << ": " << this->ErrorPerThread[i] << "\n";
    os << indent << "ThresholdedError for thread " << i << ": " 
       << this->ThresholdedErrorPerThread[i] << "\n";
    }
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "AllowShift: " << this->AllowShift << "\n";
  os << indent << "Averaging: " << this->Averaging << "\n";
}


