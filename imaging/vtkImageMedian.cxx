/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedian.cxx
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
#include "vtkImageMedian.h"



//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageMedian fitler.
vtkImageMedian::vtkImageMedian()
{
  this->Sort = NULL;
  this->SetKernelSize(1, 1, 1);
  this->HandleBoundariesOn();
  this->UseExecuteCenterOff();
}


//----------------------------------------------------------------------------
// Description:
// Destructor
vtkImageMedian::~vtkImageMedian()
{
  if (this->Sort)
    delete [] this->Sort;
}

//----------------------------------------------------------------------------
// Description:
// This method sets the size of the neighborhood.  It also sets the 
// default middle of the neighborhood 
void vtkImageMedian::SetKernelSize(int num, int *size)
{
  int idx, volume;
  
  // Limit dimensionality to be 3 or less.
  if (num > 3)
    {
    vtkWarningMacro(<< "SetKernelSize: Neighborhood can't have dimensionality "
                    << num);
    num = 3;
    }

  // Set the kernel size and middle
  volume = 1;
  for (idx = 0; idx < num; ++idx)
    {
    this->KernelSize[idx] = size[idx];
    this->KernelMiddle[idx] = size[idx] / 2;
    volume *= size[idx];
    }
  this->NumNeighborhood = volume;
  
  // free old sort memeory
  if (this->Sort)
    delete [] this->Sort;
  this->Sort = NULL;

  // allocate new sort memory
  if (this->NumNeighborhood > 0)
    {
    this->Sort = new double[(this->NumNeighborhood + 2)];
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains the second switch statement that calls the correct
// templated function for the mask types.
template <class T>
void vtkImageMedianExecute(vtkImageMedian *self,
			   vtkImageRegion *inRegion, T *inPtr, 
			   vtkImageRegion *outRegion, T *outPtr)
{
  int *kernelMiddle, *kernelSize;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  T *outPtr0, *outPtr1, *outPtr2;
  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodStartMin0, hoodStartMax0, hoodStartMin1, hoodStartMax1;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *tmpPtr0, *tmpPtr1, *tmpPtr2;
  // The portion of the out image that needs no boundary processing.
  int middleMin0, middleMax0, middleMin1, middleMax1, middleMin2, middleMax2;
  
  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  kernelMiddle = self->GetKernelMiddle();
  kernelSize = self->GetKernelSize();
  
  hoodMin0 = outMin0 - kernelMiddle[0]; 
  hoodMin1 = outMin1 - kernelMiddle[1]; 
  hoodMin2 = outMin2 - kernelMiddle[2]; 
  hoodMax0 = kernelSize[0] + hoodMin0 - 1;
  hoodMax1 = kernelSize[1] + hoodMin1 - 1;
  hoodMax2 = kernelSize[2] + hoodMin2 - 1;
  
  // Clip by the input image extent
  inRegion->GetImageExtent(middleMin0, middleMax0, 
			   middleMin1, middleMax1, 
			   middleMin2, middleMax2);
  hoodMin0 = (hoodMin0 > middleMin0) ? hoodMin0 : middleMin0;
  hoodMin1 = (hoodMin1 > middleMin1) ? hoodMin1 : middleMin1;
  hoodMin2 = (hoodMin2 > middleMin2) ? hoodMin2 : middleMin2;
  hoodMax0 = (hoodMax0 < middleMax0) ? hoodMax0 : middleMax0;
  hoodMax1 = (hoodMax1 < middleMax1) ? hoodMax1 : middleMax1;
  hoodMax2 = (hoodMax2 < middleMax2) ? hoodMax2 : middleMax2;

  // Save the starting neighborhood dimensions (2 loops only once)
  hoodStartMin0 = hoodMin0;    hoodStartMax0 = hoodMax0;
  hoodStartMin1 = hoodMin1;    hoodStartMax1 = hoodMax1;
  
  // The portion of the output that needs no boundary computation.
  middleMin0 += kernelMiddle[0];
  middleMax0 -= (kernelSize[0] - 1) - kernelMiddle[0];
  middleMin1 += kernelMiddle[1];
  middleMax1 -= (kernelSize[1] - 1) - kernelMiddle[1];
  middleMin2 += kernelMiddle[2];
  middleMax2 -= (kernelSize[2] - 1) - kernelMiddle[2];
  
  // loop through pixel of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
    {
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    hoodMin1 = hoodStartMin1;
    hoodMax1 = hoodStartMax1;
    for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
      {
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      hoodMin0 = hoodStartMin0;
      hoodMax0 = hoodStartMax0;
      for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
	{
	
	// Compute median of neighborhood
	// Note: For boundary, NumNeighborhood could be changed for
	// a faster sort.
	self->ClearMedian();
	// loop through neighborhood pixels
	tmpPtr2 = inPtr0;
	for (hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
	  {
	  tmpPtr1 = tmpPtr2;
	  for (hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
	    {
	    tmpPtr0 = tmpPtr1;
	    for (hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
	      {
	      // Add this pixel to the median
	      self->AccumulateMedian(double(*tmpPtr0));
	      
	      tmpPtr0 += inInc0;
	      }
	    tmpPtr1 += inInc1;
	    }
	  tmpPtr2 += inInc2;
	  }
	
	// Replace this pixel with the hood median
        *outPtr0 = (T)(self->GetMedian());
	
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
	outPtr0 += outInc0;
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
      outPtr1 += outInc1;
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
    outPtr2 += outInc2;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void vtkImageMedian::Execute(vtkImageRegion *inRegion, 
			     vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarWritePointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
             << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageMedianExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageMedianExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageMedianExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMedianExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMedianExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



//----------------------------------------------------------------------------
// Description:
// Get the current median of all accumilated values.
double vtkImageMedian::GetMedian()
{
  if ( ! this->Median)
    {
    vtkErrorMacro(<< "GetMedian: No median memory!");
    return 0.0;
    }
  
  return *(this->Median);
}
  


//----------------------------------------------------------------------------
// Description:
// Clear the memory to compute a new median
void vtkImageMedian::ClearMedian()
{
  this->DownNum = this->UpNum = 0;
  this->Median = this->Sort + (this->NumNeighborhood / 2);
}


//----------------------------------------------------------------------------
// Description:
// Add a sample to the median computation
void vtkImageMedian::AccumulateMedian(double val)
{
  int idx, max;
  double temp, *ptr;

  // special case: no samples yet 
  if (this->UpNum == 0)
    {
    *(this->Median) = val;
    // length of up and down arrays inclusive of current 
    this->UpNum = this->DownNum = 1; 
    // median is gaurenteed to be in this range (length of array) 
    this->DownMax = this->UpMax = (this->NumNeighborhood + 1) / 2;
    return;
    }

  // Case: value is above median 
  if (val >= *(this->Median))
    {
    // move the median if necessary
    if (this->UpNum > this->DownNum)
      {
      // Move the median Up one 
      ++this->Median;
      --this->UpNum;
      ++this->DownNum;
      --this->UpMax;
      ++this->DownMax;
      }
    // find the position for val in the sorted array
    max = (this->UpNum < this->UpMax) ? this->UpNum : this->UpMax;
    ptr = this->Median;
    idx = 0;
    while (val >= *ptr && idx < max)
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
    ++this->UpNum;
    --this->DownMax;
    return;
    }


  // Case: value is below median 
  if (val <= *(this->Median))
    {
    // move the median if necessary
    if (this->DownNum > this->UpNum)
      {
      // Move the median Down one 
      --this->Median;
      --this->DownNum;
      ++this->UpNum;
      --this->DownMax;
      ++this->UpMax;
      }
    // find the position for val in the sorted array
    max = (this->DownNum < this->DownMax) ? this->DownNum : this->DownMax;
    ptr = this->Median;
    idx = 0;
    while (val <= *ptr && idx < max)
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
    ++this->DownNum;
    --this->UpMax;
    return;
    }
}



  




