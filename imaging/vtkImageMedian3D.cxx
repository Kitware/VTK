/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedian3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageMedian3D.h"



//----------------------------------------------------------------------------
// Construct an instance of vtkImageMedian3D fitler.
vtkImageMedian3D::vtkImageMedian3D()
{
  this->SetKernelSize(1,1,1);
  this->HandleBoundaries = 1;
}

//----------------------------------------------------------------------------
// This method sets the size of the neighborhood.  It also sets the 
// default middle of the neighborhood 
void vtkImageMedian3D::SetKernelSize(int size0, int size1, int size2)
{  
  int volume;
  
  if (this->KernelSize[0] == size0 && this->KernelSize[1] == size1 && 
      this->KernelSize[2] == size2)
    {
    return;
    }
  
  // Set the kernel size and middle
  volume = 1;
  this->KernelSize[0] = size0;
  this->KernelMiddle[0] = size0 / 2;
  volume *= size0;
  this->KernelSize[1] = size1;
  this->KernelMiddle[1] = size1 / 2;
  volume *= size1;
  this->KernelSize[2] = size2;
  this->KernelMiddle[2] = size2 / 2;
  volume *= size2;

  this->NumberOfElements = volume;
  this->Modified();
}

//----------------------------------------------------------------------------
// Add a sample to the median computation
double *vtkImageMedian3DAccumulateMedian(int &UpNum, int &DownNum,
					 int &UpMax, int &DownMax,
					 int &NumNeighborhood,
					 double *Median, double val)
{
  int idx, max;
  double temp, *ptr;

  // special case: no samples yet 
  if (UpNum == 0)
    {
    *(Median) = val;
    // length of up and down arrays inclusive of current 
    UpNum = DownNum = 1; 
    // median is gaurenteed to be in this range (length of array) 
    DownMax = UpMax = (NumNeighborhood + 1) / 2;
    return Median;
    }

  // Case: value is above median 
  if (val >= *(Median))
    {
    // move the median if necessary
    if (UpNum > DownNum)
      {
      // Move the median Up one 
      ++Median;
      --UpNum;
      ++DownNum;
      --UpMax;
      ++DownMax;
      }
    // find the position for val in the sorted array
    max = (UpNum < UpMax) ? UpNum : UpMax;
    ptr = Median;
    idx = 0;
    while (idx < max && val >= *ptr)
      {
      ++ptr;
      ++idx;
      }
    // place val and move all others up
    while (idx <= max)
      {
      temp = *ptr;
      *ptr = val;
      val = temp;
      ++ptr;
      ++idx;
      }
    // Update counts
    ++UpNum;
    --DownMax;
    return Median;
    }


  // Case: value is below median 
  if (val <= *(Median))
    {
    // move the median if necessary
    if (DownNum > UpNum)
      {
      // Move the median Down one 
      --Median;
      --DownNum;
      ++UpNum;
      --DownMax;
      ++UpMax;
      }
    // find the position for val in the sorted array
    max = (DownNum < DownMax) ? DownNum : DownMax;
    ptr = Median;
    idx = 0;
    while (idx < max && val <= *ptr)
      {
      --ptr;
      ++idx;
      }
    // place val and move all others up
    while (idx <= max)
      {
      temp = *ptr;
      *ptr = val;
      val = temp;
      --ptr;
      ++idx;
      }
    // Update counts
    ++DownNum;
    --UpMax;
    return Median;
    }
  return Median;
}


//----------------------------------------------------------------------------
// This method contains the second switch statement that calls the correct
// templated function for the mask types.
template <class T>
static void vtkImageMedian3DExecute(vtkImageMedian3D *self,
				    vtkImageData *inData, T *inPtr, 
				    vtkImageData *outData, T *outPtr,
				    int outExt[6], int id)
{
  int *kernelMiddle, *kernelSize;
  // For looping though output (and input) pixels.
  int outIdx0, outIdx1, outIdx2;
  int inInc0, inInc1, inInc2;
  int outIdxC, outIncX, outIncY, outIncZ;
  T *inPtr0, *inPtr1, *inPtr2;
  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodStartMin0, hoodStartMax0, hoodStartMin1, hoodStartMax1;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *tmpPtr0, *tmpPtr1, *tmpPtr2;
  // The portion of the out image that needs no boundary processing.
  int middleMin0, middleMax0, middleMin1, middleMax1, middleMin2, middleMax2;
  int numComp;
  // variables for the median calc
  int UpNum, DownNum, UpMax, DownMax;
  double *Median;
  double *Sort = new double[(self->NumberOfElements + 8)];
  int *inExt;
  unsigned long count = 0;
  unsigned long target;
  
  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2); 
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  kernelMiddle = self->KernelMiddle;
  kernelSize = self->KernelSize;
  
  numComp = inData->GetNumberOfScalarComponents();

  hoodMin0 = outExt[0] - kernelMiddle[0]; 
  hoodMin1 = outExt[2] - kernelMiddle[1]; 
  hoodMin2 = outExt[4] - kernelMiddle[2]; 
  hoodMax0 = kernelSize[0] + hoodMin0 - 1;
  hoodMax1 = kernelSize[1] + hoodMin1 - 1;
  hoodMax2 = kernelSize[2] + hoodMin2 - 1;
  
  // Clip by the input image extent
  inExt = inData->GetExtent();
  hoodMin0 = (hoodMin0 > inExt[0]) ? hoodMin0 : inExt[0];
  hoodMin1 = (hoodMin1 > inExt[2]) ? hoodMin1 : inExt[2];
  hoodMin2 = (hoodMin2 > inExt[4]) ? hoodMin2 : inExt[4];
  hoodMax0 = (hoodMax0 < inExt[1]) ? hoodMax0 : inExt[1];
  hoodMax1 = (hoodMax1 < inExt[3]) ? hoodMax1 : inExt[3];
  hoodMax2 = (hoodMax2 < inExt[5]) ? hoodMax2 : inExt[5];

  // Save the starting neighborhood dimensions (2 loops only once)
  hoodStartMin0 = hoodMin0;    hoodStartMax0 = hoodMax0;
  hoodStartMin1 = hoodMin1;    hoodStartMax1 = hoodMax1;
  
  // The portion of the output that needs no boundary computation.
  middleMin0 = inExt[0] + kernelMiddle[0];
  middleMax0 = inExt[1] - (kernelSize[0] - 1) + kernelMiddle[0];
  middleMin1 = inExt[2] + kernelMiddle[1];
  middleMax1 = inExt[3] - (kernelSize[1] - 1) + kernelMiddle[1];
  middleMin2 = inExt[4] + kernelMiddle[2];
  middleMax2 = inExt[5] - (kernelSize[2] - 1) + kernelMiddle[2];
  
  target = (unsigned long)((outExt[5] - outExt[4] + 1)*
    (outExt[3] - outExt[2] + 1)/50.0);
  target++;
  
  // loop through pixel of output
  inPtr = (T *)inData->GetScalarPointer(hoodMin0,hoodMin1,hoodMin2);
  inPtr2 = inPtr;
  for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
    {
    inPtr1 = inPtr2;
    hoodMin1 = hoodStartMin1;
    hoodMax1 = hoodStartMax1;
    for (outIdx1 = outExt[2]; 
	 !self->AbortExecute && outIdx1 <= outExt[3]; ++outIdx1)
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
      hoodMin0 = hoodStartMin0;
      hoodMax0 = hoodStartMax0;
      for (outIdx0 = outExt[0]; outIdx0 <= outExt[1]; ++outIdx0)
	{
	for (outIdxC = 0; outIdxC < numComp; outIdxC++)
	  {
	  // Compute median of neighborhood
	  // Note: For boundary, NumNeighborhood could be changed for
	  // a faster sort.
	  DownNum = UpNum = 0;
          Median = Sort + (self->NumberOfElements / 2) + 4;
	  // loop through neighborhood pixels
	  tmpPtr2 = inPtr0 + outIdxC;
	  for (hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
	    {
	    tmpPtr1 = tmpPtr2;
	    for (hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
	      {
	      tmpPtr0 = tmpPtr1;
	      for (hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
		{
		// Add this pixel to the median
		Median = vtkImageMedian3DAccumulateMedian(UpNum, DownNum, 
							  UpMax, DownMax,
							  self->NumberOfElements,
							  Median,
							  double(*tmpPtr0));
		
		tmpPtr0 += inInc0;
		}
	      tmpPtr1 += inInc1;
	      }
	    tmpPtr2 += inInc2;
	    }
	
	  // Replace this pixel with the hood median
	  *outPtr = (T)(*Median);
	  outPtr++;
	  }
	
	// shift neighborhood considering boundaries
	if (outIdx0 >= middleMin0)
	  {
	  inPtr0 += inInc0;
	  ++hoodMin0;
	  }
	if (outIdx0 < middleMax0)
	  {
	  ++hoodMax0;
	  }
	}
      // shift neighborhood considering boundaries
      if (outIdx1 >= middleMin1)
	{
	inPtr1 += inInc1;
	++hoodMin1;
	}
      if (outIdx1 < middleMax1)
	{
	++hoodMax1;
	}
      outPtr += outIncY;
    }
    // shift neighborhood considering boundaries
    if (outIdx2 >= middleMin2)
      {
      inPtr2 += inInc2;
      ++hoodMin2;
      }
    if (outIdx2 < middleMax2)
      {
      ++hoodMax2;
      }
    outPtr += outIncZ;
    }

  delete [] Sort;
  
}

//----------------------------------------------------------------------------
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void vtkImageMedian3D::ThreadedExecute(vtkImageData *inData, 
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
    case VTK_FLOAT:
      vtkImageMedian3DExecute(this, inData, (float *)(inPtr), 
			      outData, (float *)(outPtr),outExt, id);
      break;
    case VTK_INT:
      vtkImageMedian3DExecute(this, inData, (int *)(inPtr), 
			      outData, (int *)(outPtr),outExt, id);
      break;
    case VTK_SHORT:
      vtkImageMedian3DExecute(this, inData, (short *)(inPtr), 
			      outData, (short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMedian3DExecute(this, inData, (unsigned short *)(inPtr), 
			      outData, (unsigned short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMedian3DExecute(this, inData, (unsigned char *)(inPtr), 
			      outData, (unsigned char *)(outPtr),outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}


