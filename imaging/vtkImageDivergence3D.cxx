/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDivergence3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageDivergence3D.h"


//----------------------------------------------------------------------------
vtkImageDivergence3D::vtkImageDivergence3D()
{
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS,
			 VTK_IMAGE_COMPONENT_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
  
}

//----------------------------------------------------------------------------
void vtkImageDivergence3D::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
void vtkImageDivergence3D::ComputeRequiredInputUpdateExtent()
{
  int extent[4];
  int *wholeExtent;
  int idx;

  // Expand all but the time axis
  wholeExtent = this->Input->GetWholeExtent();
  this->Output->GetUpdateExtent(extent);
  // Expand rest
  for (idx = 0; idx < 3; ++idx)
    {
    if (extent[2*idx] > wholeExtent[2*idx])
      {
      --extent[2*idx];
      }
    if (extent[2*idx + 1] < wholeExtent[2*idx + 1])
      {
      ++extent[2*idx + 1];
      }
    }
  this->Input->SetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
void vtkImageDivergence3D::Execute(vtkImageRegion *inRegion, 
				   vtkImageRegion *outRegion)
{
  float d, sum;
  float r[3];
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2, minV, maxV;
  int outIdx0, outIdx1, outIdx2, inIdxV;
  int outInc0, outInc1, outInc2;
  float *outPtr0, *outPtr1, *outPtr2;
  int inInc0, inInc1, inInc2, inIncV;
  float *inPtr0, *inPtr1, *inPtr2, *inPtrV;
  // For computation of divergence (everything has to be arrays for loop).
  int *incs, *wholeExtent, *idxs, outIdxs[3];

  // Error checking
  if (inRegion->GetScalarType() != VTK_FLOAT ||
      outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: In and out must be type float.");
    return;
    }
  
  // Get information to march through data (skip component)
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  inRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, inIncV);
  inRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetExtent(min0,max0, min1,max1, min2,max2);

  if ( minV != 0 || maxV > 2)
    {
    vtkErrorMacro(<< "Execute: Unable to handle vector");
    return;
    }
  
  // The spacing is important for computing the gradient.
  // central differences (2 * ratio).
  // Negative because below we have (min - max) for dx ...
  inRegion->GetSpacing(3, r);
  r[0] = -0.5 / r[0];
  r[1] = -0.5 / r[1];
  r[2] = -0.5 / r[2];
  
  // loop through pixels of output
  // We want the input pixel to correspond to output
  inPtr2 = (float *)(inRegion->GetScalarPointer(min0,min1,min2));
  outPtr2 = (float *)(outRegion->GetScalarPointer(min0,min1,min2));
  for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
    {
    outIdxs[2] = outIdx2;
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
      {
      outIdxs[1] = outIdx1;
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	{
	*outIdxs = outIdx0;
	
	// compute divergence for this vector
	sum = 0.0;
	inPtrV = inPtr0;
	idxs = outIdxs;
	incs = inRegion->GetIncrements(); 
	wholeExtent = inRegion->GetWholeExtent(); 
	for(inIdxV = minV; inIdxV <= maxV; ++inIdxV)
	  {
	  // Compute difference using central differences (if in extent).
	  d = (*idxs == *wholeExtent++) ? *inPtrV : inPtrV[-*incs];
	  d -= (*idxs == *wholeExtent++) ? *inPtrV : inPtrV[*incs];
	  sum += d * r[inIdxV]; // multiply by spacing
	  ++idxs;
	  ++incs;
	  inPtrV += inIncV;
	  }
	*outPtr0 = sum;
	
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

