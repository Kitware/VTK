/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedianFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageMedianFilter.hh"



//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageMedianFilter fitler.
vtkImageMedianFilter::vtkImageMedianFilter()
{
  this->Sort = NULL;
  this->SetRadius(1, 1, 0);
}


//----------------------------------------------------------------------------
// Description:
// Destructor
vtkImageMedianFilter::~vtkImageMedianFilter()
{
  if (this->Sort)
    delete [] this->Sort;
}

//----------------------------------------------------------------------------
// Description:
// Set the radius of each axis.
void vtkImageMedianFilter::SetRadius(int rad0, int rad1, int rad2)
{
  this->Modified();

  // Set the Radius
  this->Radius[0] = rad0;
  this->Radius[1] = rad1;
  this->Radius[2] = rad2;

  // free old sort memeory
  if (this->Sort)
    delete [] this->Sort;

  // compute the numbe4r of pixels in the neighborhood 
  this->NumNeighborhood = (1 + 2 * rad0) * (1 + 2 * rad1) * (1 + 2 * rad2);

  // allocate new sort memory
  this->Sort = new float[(this->NumNeighborhood + 2)];
}


//----------------------------------------------------------------------------
// Description:
// This method computes the Region of the input necessary to generate outRegion.
void vtkImageMedianFilter::RequiredRegion(int *outOffset, int *outSize,
					  int *inOffset, int *inSize)
{
  int idx;

  // ignoring boundaries for now
  for (idx = 0; idx < 3; ++idx)
    {
    inOffset[idx] = outOffset[idx] - this->Radius[idx];
    inSize[idx] = outSize[idx] + 2 * this->Radius[idx];
    }
}

//----------------------------------------------------------------------------
// Description:
// Returns the largest region which can be requested.
// Since borders are not handled yet, the valid image shrinks.
void vtkImageMedianFilter::GetBoundary(int *offset, int *size)
{
  int idx;

  // get the Boundary of the input
  this->vtkImageFilter::GetBoundary(offset, size);
  
  for (idx = 0; idx < 3; ++idx)
    {
    offset[idx] += this->Radius[idx];
    size[idx] -= (2 * this->Radius[idx]);
    }

  vtkDebugMacro(<< "GetBoundary: returning offset = ("
          << offset[0] << ", " << offset[1] << ", " << offset[2]
          << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
          << ")");  
}





//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the Median
// algorithm to fill the output from the input.
// As a place holder, an identity Median is implemented.
void vtkImageMedianFilter::Execute(vtkImageRegion *inRegion, 
				   vtkImageRegion *outRegion)
{
  int size0, size1, size2;
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;

  // Get information to march through data
  inPtr2 = inRegion->GetPointer(inRegion->GetOffset());
  inRegion->GetInc(inInc0, inInc1, inInc2);  
  outPtr2 = outRegion->GetPointer(outRegion->GetOffset());
  outRegion->GetInc(outInc0, outInc1, outInc2);  
  outRegion->GetSize(size0, size1, size2);  

  vtkDebugMacro(<< "Execute: inRegion = (" << inRegion 
		<< "), outRegion = (" << outRegion << ")");
    
  // perform filter for each pixel of output
  for (idx2 = 0; idx2 < size2; ++idx2){
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (idx1 = 0; idx1 < size1; ++idx1){
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (idx0 = 0; idx0 < size0; ++idx0){

        // Replace this pixel with the neighborhood median
        *outPtr0 = this->NeighborhoodMedian(inPtr0, inInc0, inInc1, inInc2);

	outPtr0 += outInc0;
	inPtr0 += inInc0;
      }
      outPtr1 += outInc1;
      inPtr1 += inInc1;
    }
    outPtr2 += outInc2;
    inPtr2 += inInc2;
  }
}




//----------------------------------------------------------------------------
// Description:
// This private method calculates and returns the median of a neighborhood
// around a pixel.  Is this approach faster than a sort?
float vtkImageMedianFilter::NeighborhoodMedian(float *inPtr, 
					       int inc0, int inc1, int inc2)
{
  int idx0, idx1, idx2;
  float *ptr0, *ptr1, *ptr2;
  int diam0, diam1, diam2;

  // allocate sort arrays from dimensions of neighborhood
  diam0 = 1 + 2 * this->Radius[0];
  diam1 = 1 + 2 * this->Radius[1];
  diam2 = 1 + 2 * this->Radius[2];

  // start accumilating a new median
  this->ClearMedian();

  // find the corner of the neighborhood (already at corner)
  ptr2 = inPtr;
  // loop over neighborhood pixels
  for (idx2 = 0; idx2 < diam2; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = 0; idx1 < diam1; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = 0; idx0 < diam0; ++idx0)
	{
	// insert sample in sorted arrays 
	this->AccumulateMedian(*ptr0);
	ptr0 += inc0;
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }

  return *(this->Median);
}



//----------------------------------------------------------------------------
// Description:
// Clear the memory to compute a new median
void vtkImageMedianFilter::ClearMedian()
{
  this->DownNum = this->UpNum = 0;
  this->Median = this->Sort + (this->NumNeighborhood / 2);
}


//----------------------------------------------------------------------------
// Description:
// Add a sample to the median computation
void vtkImageMedianFilter::AccumulateMedian(float val)
{
  int idx, max;
  float temp, *ptr;

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

  




















