/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedianFilter.cxx
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
#include "vtkImageMedianFilter.h"



//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageMedianFilter fitler.
vtkImageMedianFilter::vtkImageMedianFilter()
{
  this->Sort = NULL;
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
// This method sets the size of the neighborhood.  It also sets the 
// default middle of the neighborhood 
void vtkImageMedianFilter::SetNumberOfElements(int num)
{  
  if (this->NumNeighborhood == num)
     {
     return;
     }
  this->NumNeighborhood = num;
  
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
// Get the current median of all accumilated values.
double vtkImageMedianFilter::GetMedian()
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
void vtkImageMedianFilter::ClearMedian()
{
  this->DownNum = this->UpNum = 0;
  this->Median = this->Sort + (this->NumNeighborhood / 2);
}


//----------------------------------------------------------------------------
// Description:
// Add a sample to the median computation
void vtkImageMedianFilter::AccumulateMedian(double val)
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



  




