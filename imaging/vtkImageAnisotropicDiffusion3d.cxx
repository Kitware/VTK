/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion3d.cxx
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
#include <math.h>
#include "vtkImageAnisotropicDiffusion3d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageAnisotropicDiffusion3d fitler.
vtkImageAnisotropicDiffusion3d::vtkImageAnisotropicDiffusion3d()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  
  this->UseExecuteCenterOff();
  this->HandleBoundariesOn();
  this->SetNumberOfIterations(4);
  this->DiffusionThreshold = 5.0;
  this->DiffusionFactor = 0.3;
}


//----------------------------------------------------------------------------
void 
vtkImageAnisotropicDiffusion3d::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";
  os << indent << "DiffusionThreshold: " << this->DiffusionThreshold << "\n";
}



//----------------------------------------------------------------------------
// Description:
// This method sets the number of inputs which also affects the
// input neighborhood needed to compute one output pixel.
void vtkImageAnisotropicDiffusion3d::SetNumberOfIterations(int num)
{
  int temp;
  
  this->Modified();
  vtkDebugMacro(<< "SetNumberOfIterations: " << num);
  temp = num*2 + 1;
  this->KernelSize[0] = temp;
  this->KernelSize[1] = temp;
  this->KernelMiddle[0] = num;
  this->KernelMiddle[1] = num;

  this->NumberOfIterations = num;
}


  
  
  
  
//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The input and output regions
// must have the same data type.
void vtkImageAnisotropicDiffusion3d::Execute(vtkImageRegion *inRegion, 
						     vtkImageRegion *outRegion)
{
  int idx;
  float ar0, ar1, ar2;
  vtkImageRegion *in;
  vtkImageRegion *out;
  vtkImageRegion *temp;
  int extent[6]; 

  inRegion->GetAspectRatio(ar0, ar1, ar2);
  inRegion->GetExtent(extent, 3);

  // make the temporary regions to iterate over.
  in = new vtkImageRegion;
  out = new vtkImageRegion;
  
  // might as well make these floats
  in->SetExtent(extent, 3);
  in->SetDataType(VTK_FLOAT);
  in->CopyRegionData(inRegion);
  out->SetExtent(extent, 3);
  out->SetDataType(VTK_FLOAT);
  out->Allocate();

  // Loop performing the diffusion
  // Note: region extent could get smaller as the diffusion progresses
  // (but never get smaller than output region).
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iterate(in, out, ar0, ar1, ar2);
    temp = in;
    in = out;
    out = temp;
    }
  
  // copy results into output.
  outRegion->CopyRegionData(in);
  in->Delete ();
  out->Delete ();
}





//----------------------------------------------------------------------------
// Description:
// This method performs one pass of the diffusion filter.
// The inRegion and outRegion are assumed to have data type float,
// and have the same extent.
void vtkImageAnisotropicDiffusion3d::Iterate(vtkImageRegion *inRegion, 
					     vtkImageRegion *outRegion,
					     float ar0, float ar1, float ar2)
{
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2;
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;
  float ar01, ar02, ar12, ar012, diff;

  inRegion->GetExtent(min0, max0, min1, max1, min2, max2);
  inRegion->GetIncrements(inInc0, inInc1, inInc2);
  outRegion->GetIncrements(outInc0, outInc1, outInc2);
  ar01 = sqrt(ar0 * ar0 + ar1 * ar1) * this->DiffusionThreshold;
  ar02 = sqrt(ar0 * ar0 + ar2 * ar2) * this->DiffusionThreshold;
  ar12 = sqrt(ar1 * ar1 + ar2 * ar2) * this->DiffusionThreshold;
  ar012 = sqrt(ar0 * ar0 + ar1 * ar1 + ar2 * ar2) * this->DiffusionThreshold;
  ar0 *= this->DiffusionThreshold;
  ar1 *= this->DiffusionThreshold;
  ar2 *= this->DiffusionThreshold;
  

  // I appolgize for explicitely diffusing each neighbor, but it is the easiest
  // way to deal with the boundary conditions.  Besides it is fast.
  // (Are you sure every one is correct?!!!)
  inPtr2 = (float *)(inRegion->GetScalarPointer());
  outPtr2 = (float *)(outRegion->GetScalarPointer());
  for (idx2 = min2; idx2 <= max2; ++idx2, inPtr2+=inInc2, outPtr2+=outInc2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;    
    for (idx1 = min1; idx1 <= max1; ++idx1, inPtr1+=inInc1, outPtr1+=outInc1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;    
      for (idx0 = min0; idx0 <= max0; ++idx0, inPtr0+=inInc0, outPtr0+=outInc0)
	{
	// Copy center
	*outPtr0 = *inPtr0;
	// Start diffusing
	// left
	if (idx0 != min0)
	  {
	  diff = inPtr0[-inInc0] - *inPtr0;
	  if (fabs(diff) < ar0)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right
	if (idx0 != max0)
	  {
	  diff = inPtr0[inInc0] - *inPtr0;
	  if (fabs(diff) < ar0)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// up
	if (idx1 != min1)
	  {
	  diff = inPtr0[-inInc1] - *inPtr0;
	  if (fabs(diff) < ar1)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// down
	if (idx1 != max1)
	  {
	  diff = inPtr0[inInc1] - *inPtr0;
	  if (fabs(diff) < ar1)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// in
	if (idx2 != min2)
	  {
	  diff = inPtr0[-inInc2] - *inPtr0;
	  if (fabs(diff) < ar2)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// out
	if (idx2 != max2)
	  {
	  diff = inPtr0[inInc2] - *inPtr0;
	  if (fabs(diff) < ar2)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }

	// left up
	if (idx0 != min0 && idx1 != min1)
	  {
	  diff = inPtr0[-inInc0-inInc1] - *inPtr0;
	  if (fabs(diff) < ar01)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right up
	if (idx0 != max0 && idx1 != min1)
	  {
	  diff = inPtr0[inInc0-inInc1] - *inPtr0;
	  if (fabs(diff) < ar01)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// left down
	if (idx0 != min0 && idx1 != max1)
	  {
	  diff = inPtr0[-inInc0+inInc1] - *inPtr0;
	  if (fabs(diff) < ar01)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right down
	if (idx0 != max0 && idx1 != max1)
	  {
	  diff = inPtr0[inInc0+inInc1] - *inPtr0;
	  if (fabs(diff) < ar01)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	
	// left in
	if (idx0 != min0 && idx2 != min2)
	  {
	  diff = inPtr0[-inInc0-inInc2] - *inPtr0;
	  if (fabs(diff) < ar02)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right in
	if (idx0 != max0 && idx2 != min2)
	  {
	  diff = inPtr0[inInc0-inInc2] - *inPtr0;
	  if (fabs(diff) < ar02)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// left out
	if (idx0 != min0 && idx2 != max2)
	  {
	  diff = inPtr0[-inInc0+inInc2] - *inPtr0;
	  if (fabs(diff) < ar02)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right out
	if (idx0 != max0 && idx2 != max2)
	  {
	  diff = inPtr0[inInc0+inInc2] - *inPtr0;
	  if (fabs(diff) < ar02)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	
	// up in
	if (idx1 != min1 && idx2 != min2)
	  {
	  diff = inPtr0[-inInc1-inInc2] - *inPtr0;
	  if (fabs(diff) < ar12)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// down in
	if (idx1 != max1 && idx2 != min2)
	  {
	  diff = inPtr0[inInc1-inInc2] - *inPtr0;
	  if (fabs(diff) < ar12)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// up out
	if (idx1 != min1 && idx2 != max2)
	  {
	  diff = inPtr0[-inInc1+inInc2] - *inPtr0;
	  if (fabs(diff) < ar12)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// down out
	if (idx1 != max1 && idx2 != max2)
	  {
	  diff = inPtr0[inInc1+inInc2] - *inPtr0;
	  if (fabs(diff) < ar12)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }


	// left up in
	if (idx0 != min0 && idx1 != min1 && idx2 != min2)
	  {
	  diff = inPtr0[-inInc0-inInc1-inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right up in
	if (idx0 != max0 && idx1 != min1 && idx2 != min2)
	  {
	  diff = inPtr0[inInc0-inInc1-inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// left down in
	if (idx0 != min0 && idx1 != max1 && idx2 != min2)
	  {
	  diff = inPtr0[-inInc0+inInc1-inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right down in
	if (idx0 != max0 && idx1 != max1 && idx2 != min2)
	  {
	  diff = inPtr0[inInc0+inInc1-inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// left up out
	if (idx0 != min0 && idx1 != min1 && idx2 != max2)
	  {
	  diff = inPtr0[-inInc0-inInc1+inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right up out
	if (idx0 != max0 && idx1 != min1 && idx2 != max2)
	  {
	  diff = inPtr0[inInc0-inInc1+inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// left down out
	if (idx0 != min0 && idx1 != max1 && idx2 != max2)
	  {
	  diff = inPtr0[-inInc0+inInc1+inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }
	// right down out
	if (idx0 != max0 && idx1 != max1 && idx2 != max2)
	  {
	  diff = inPtr0[inInc0+inInc1+inInc2] - *inPtr0;
	  if (fabs(diff) < ar123)
	    {
	    *outPtr0 += diff * this->DiffusionFactor;
	    }
	  }


  






