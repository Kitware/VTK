/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.cxx
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
#include "vtkImageHybridMedian2D.h"



//----------------------------------------------------------------------------
vtkImageHybridMedian2D::vtkImageHybridMedian2D()
{
  this->KernelSize[0] = 5;
  this->KernelSize[1] = 5;
  this->SetNumberOfElements(9);
}


//----------------------------------------------------------------------------
vtkImageHybridMedian2D::~vtkImageHybridMedian2D()
{
}

  
//----------------------------------------------------------------------------
void vtkImageHybridMedian2D::SetFilteredAxes(int axis0, int axis1)
{
  int axes[2];

  axes[0] = axis0;
  axes[1] = axis1;
  this->vtkImageSpatialFilter::SetFilteredAxes(2, axes);
}

//----------------------------------------------------------------------------
void vtkImageHybridMedian2D::Execute(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion)
{
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  int min0, max0, min1, max1;
  int wholeMin0, wholeMax0, wholeMin1, wholeMax1;
  float *inPtr0, *inPtr1;
  float *outPtr0, *outPtr1, *ptr;
  float median1, median2, temp;

  if (inRegion->GetScalarType() != VTK_FLOAT || 
      outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: Both input and output have to be float for now.");
    return;
    }

  inRegion->GetIncrements(inInc0, inInc1);
  inRegion->GetWholeExtent(wholeMin0, wholeMax0, wholeMin1, wholeMax1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);
  outPtr1 = (float *)(outRegion->GetScalarPointer(min0, min1));
  inPtr1 = (float *)(inRegion->GetScalarPointer(min0, min1));

  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    inPtr0 = inPtr1;
    outPtr0 = outPtr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      // compute median of + neighborhood
      this->ClearMedian();
      // Center
      this->AccumulateMedian(*inPtr0);
      // left
      ptr = inPtr0;
      if (idx0 > wholeMin0)
        {
        ptr -= inInc0;
        this->AccumulateMedian(*ptr);
        }
      if (idx0 - 1 > wholeMin0)
        {
        ptr -= inInc0;
        this->AccumulateMedian(*ptr);
        }
      // right
      ptr = inPtr0;
      if (idx0 < wholeMax0)
        {
        ptr += inInc0;
        this->AccumulateMedian(*ptr);
        }
      if (idx0 + 1 < wholeMax0)
        {
        ptr += inInc0;
        this->AccumulateMedian(*ptr);
        }
      // up
      ptr = inPtr0;
      if (idx1 > wholeMin1)
        {
        ptr -= inInc1;
        this->AccumulateMedian(*ptr);
        }
      if (idx1 - 1 > wholeMin1)
        {
        ptr -= inInc1;
        this->AccumulateMedian(*ptr);
        }
      // right
      ptr = inPtr0;
      if (idx1 < wholeMax1)
        {
        ptr += inInc1;
        this->AccumulateMedian(*ptr);
        }
      if (idx1 + 1 < wholeMax1)
        {
        ptr += inInc1;
        this->AccumulateMedian(*ptr);
        }
      median1 = this->GetMedian();
    
      // Cross median
      this->ClearMedian();
      // Center
      this->AccumulateMedian(*inPtr0);   
      // Lower Left
      ptr = inPtr0;
      if (idx0 > wholeMin0 && idx1 > wholeMin1)
        {
        ptr -= inInc0 + inInc1;
        this->AccumulateMedian(*ptr);
        }
      if (idx0-1 > wholeMin0 && idx1-1 > wholeMin1)
        {
        ptr -= inInc0 + inInc1;
        this->AccumulateMedian(*ptr);
        }
      // Upper Right       
      ptr = inPtr0;
      if (idx0 < wholeMax0 && idx1 < wholeMax1)
        {
        ptr += inInc0 + inInc1;
        this->AccumulateMedian(*ptr);
        }
      if (idx0+1 > wholeMax0 && idx1+1 > wholeMax1)
        {
        ptr -= inInc0 + inInc1;
        this->AccumulateMedian(*ptr);
        }
      // Upper Left
      ptr = inPtr0;
      if (idx0 > wholeMin0 && idx1 < wholeMax1)
        {
        ptr += -inInc0 + inInc1;
        this->AccumulateMedian(*ptr);
        }
      if (idx0-1 > wholeMin0 && idx1+1 < wholeMax1)
        {
        ptr += -inInc0 + inInc1;
        this->AccumulateMedian(*ptr);
        }
      // Lower Right
      ptr = inPtr0;
      if (idx0 < wholeMax0 && idx1 > wholeMin1)
        {
        ptr += inInc0 - inInc1;
        this->AccumulateMedian(*ptr);
        }
      if (idx0+1 < wholeMax0 && idx1-1 > wholeMin1)
        {
        ptr += inInc0 - inInc1;
        this->AccumulateMedian(*ptr);
        }
      median2 = this->GetMedian();

      // Compute the median of the three. (med1, med2 and center)
      if (median1 > median2)
        {
        temp = median1;
        median1 = median2;
        median2 = temp;
        }
      if (*inPtr0 < median1)
        {
        *outPtr0 = median1;
        }
      else if (*inPtr0 < median2)
        {
        *outPtr0 = *inPtr0;
        }
      else
        {
        *outPtr0 = median2;
        }
      inPtr0 += inInc0;
      outPtr0 += outInc0;
      }
     inPtr1 += inInc1;
     outPtr1 += outInc1;
    }
}
        


