/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.cxx
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
#include "vtkImageHybridMedian2D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageHybridMedian2D* vtkImageHybridMedian2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageHybridMedian2D");
  if(ret)
    {
    return (vtkImageHybridMedian2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageHybridMedian2D;
}






//----------------------------------------------------------------------------
vtkImageHybridMedian2D::vtkImageHybridMedian2D()
{
  this->KernelSize[0] = 5;
  this->KernelSize[1] = 5;
  this->KernelSize[2] = 1;
  this->KernelMiddle[0] = 2;
  this->KernelMiddle[1] = 2;
  this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}


void vtkImageHybridMedian2D::ThreadedExecute(vtkImageData *inData, 
					     vtkImageData *outData,
					     int outExt[6], int id)
{
  int inExt[6];
  int idx0, idx1, idx2, idxC;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2, numComps;
  int wholeMin0, wholeMax0, wholeMin1, wholeMax1, wholeMin2, wholeMax2;
  float *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  float *outPtr0, *outPtr1, *outPtr2, *outPtrC, *ptr;
  float median1, median2, temp;
  float array[9];
  int icount;
  unsigned long count = 0;
  unsigned long target;

  id = id;
  
  if (inData->GetScalarType() != VTK_FLOAT || 
      outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: Both input and output have to be float for now.");
    return;
    }

  this->ComputeInputUpdateExtent(inExt, outExt); 
  inData->GetIncrements(inInc0, inInc1, inInc2);
  this->GetInput()->GetWholeExtent(wholeMin0, wholeMax0, wholeMin1, wholeMax1,
			      wholeMin2, wholeMax2);
  numComps = inData->GetNumberOfScalarComponents();
  outData->GetIncrements(outInc0, outInc1, outInc2);
  min0 = outExt[0];   max0 = outExt[1];
  min1 = outExt[2];   max1 = outExt[3];
  min2 = outExt[4];   max2 = outExt[5];
  outPtr2 = (float *)(outData->GetScalarPointer(min0, min1, min2));
  inPtr2 = (float *)(inData->GetScalarPointer(min0, min1, min2));
  
  target = (unsigned long)((max2-min2+1)*(max1-min1+1)/50.0);
  target++;

  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
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
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{
	inPtrC = inPtr0;
	outPtrC = outPtr0;
	for (idxC = 0; idxC < numComps; ++idxC)
	  {
	  // compute median of + neighborhood
	  icount = 0;
	  // Center
	  ptr = inPtrC;
	  array[icount++] = *ptr;
	  // left
	  ptr = inPtrC;
	  if (idx0 > wholeMin0)
	    {
	    ptr -= inInc0;
	    array[icount++] = *ptr;
	    }
	  if (idx0 - 1 > wholeMin0)
	    {
	    ptr -= inInc0;
	    array[icount++] = *ptr;
	    }
	  // right
	  ptr = inPtrC;
	  if (idx0 < wholeMax0)
	    {
	    ptr += inInc0;
	    array[icount++] = *ptr;
	    }
	  if (idx0 + 1 < wholeMax0)
	    {
	    ptr += inInc0;
	    array[icount++] = *ptr;
	    }
	  // up
	  ptr = inPtrC;
	  if (idx1 > wholeMin1)
	    {
	    ptr -= inInc1;
	    array[icount++] = *ptr;
	    }
	  if (idx1 - 1 > wholeMin1)
	    {
	    ptr -= inInc1;
	    array[icount++] = *ptr;
	    }
	  // right
	  ptr = inPtrC;
	  if (idx1 < wholeMax1)
	    {
	    ptr += inInc1;
	    array[icount++] = *ptr;
	    }
	  if (idx1 + 1 < wholeMax1)
	    {
	    ptr += inInc1;
	    array[icount++] = *ptr;
	    }
	  median1 = this->ComputeMedian(array, icount);
	  
	  // Cross median
	  icount = 0;
	  // Center
	  array[icount++] = *ptr;
	  // Lower Left
	  ptr = inPtrC;
	  if (idx0 > wholeMin0 && idx1 > wholeMin1)
	    {
	    ptr -= inInc0 + inInc1;
	    array[icount++] = *ptr;
	    }
	  if (idx0-1 > wholeMin0 && idx1-1 > wholeMin1)
	    {
	    ptr -= inInc0 + inInc1;
	    array[icount++] = *ptr;
	    }
	  // Upper Right       
	  ptr = inPtrC;
	  if (idx0 < wholeMax0 && idx1 < wholeMax1)
	    {
	    ptr += inInc0 + inInc1;
	    array[icount++] = *ptr;
	    }
	  if (idx0+1 > wholeMax0 && idx1+1 > wholeMax1)
	    {
	    ptr -= inInc0 + inInc1;
	    array[icount++] = *ptr;
	    }
	  // Upper Left
	  ptr = inPtrC;
	  if (idx0 > wholeMin0 && idx1 < wholeMax1)
	    {
	    ptr += -inInc0 + inInc1;
	    array[icount++] = *ptr;
	    }
	  if (idx0-1 > wholeMin0 && idx1+1 < wholeMax1)
	    {
	    ptr += -inInc0 + inInc1;
	    array[icount++] = *ptr;
	    }
	  // Lower Right
	  ptr = inPtrC;
	  if (idx0 < wholeMax0 && idx1 > wholeMin1)
	    {
	    ptr += inInc0 - inInc1;
	    array[icount++] = *ptr;
	    }
	  if (idx0+1 < wholeMax0 && idx1-1 > wholeMin1)
	    {
	    ptr += inInc0 - inInc1;
	    array[icount++] = *ptr;
	    }
	  median2 = this->ComputeMedian(array, icount);
	  
	  // Compute the median of the three. (med1, med2 and center)
	  if (median1 > median2)
	    {
	    temp = median1;
	    median1 = median2;
	    median2 = temp;
	    }
	  if (*inPtrC < median1)
	    {
	    *outPtrC = median1;
	    }
	  else if (*inPtrC < median2)
	    {
	    *outPtrC = *inPtrC;
	    }
	  else
	    {
	    *outPtrC = median2;
	    }
	  ++inPtrC;
	  ++outPtrC;
	  }
	inPtr0 += inInc0;
	outPtr0 += outInc0;
	}
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}

  
        

// stupid little bubble sort
float vtkImageHybridMedian2D::ComputeMedian(float *array, int size)
{
  int idx, flag;
  float temp;
  
  flag = 1;
  while (flag)
    {
    flag = 0;
    for (idx = 1; idx < size; ++idx)
      {
      if (array[idx-1] > array[idx])
	{
	flag = 1;
	temp = array[idx-1];
	array[idx-1] = array[idx];
	array[idx] = temp;
	}
      }
    }
  
  // now return the median
  return array[size / 2];
}




