/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion2d.cxx
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
#include <math.h>
#include "vtkImageAnisotropicDiffusion2d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageAnisotropicDiffusion2d fitler.
vtkImageAnisotropicDiffusion2d::vtkImageAnisotropicDiffusion2d()
{
  this->SetAxes2d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  
  this->UseExecuteCenterOff();
  this->HandleBoundariesOn();
  this->SetNumberOfIterations(4);
  this->DiffusionThreshold = 5.0;
  this->DiffusionFactor = 0.3;
}


//----------------------------------------------------------------------------
void 
vtkImageAnisotropicDiffusion2d::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";
  os << indent << "DiffusionThreshold: " << this->DiffusionThreshold << "\n";
}



//----------------------------------------------------------------------------
// Description:
// This method sets the number of inputs which also affects the
// input neighborhood needed to compute one output pixel.
void vtkImageAnisotropicDiffusion2d::SetNumberOfIterations(int num)
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
void vtkImageAnisotropicDiffusion2d::Execute2d(vtkImageRegion *inRegion, 
						     vtkImageRegion *outRegion)
{
  int idx;
  float ar0, ar1;
  vtkImageRegion *in;
  vtkImageRegion *out;
  vtkImageRegion *temp;
  int bounds[6]; 

  inRegion->GetAspectRatio2d(ar0, ar1);
  inRegion->GetBounds3d (bounds);

  // make the temporary regions to iterate over.
  in = new vtkImageRegion;
  out = new vtkImageRegion;
  
  // might as well make these floats
  in->SetBounds3d(bounds);
  in->SetDataType(VTK_IMAGE_FLOAT);
  in->CopyRegionData(inRegion);
  out->SetBounds3d(bounds);
  out->SetDataType(VTK_IMAGE_FLOAT);
  out->Allocate();

  // Loop performing the diffusion
  // Note: region bounds could get smaller as the diffusion progresses
  // (but never get smaller than output region).
  for (idx = 0; idx < this->NumberOfIterations; ++idx)
    {
    this->Iterate(in, out, ar0, ar1);
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
// and have the same bounds.
void vtkImageAnisotropicDiffusion2d::Iterate(vtkImageRegion *inRegion, 
						   vtkImageRegion *outRegion,
						   float ar0, float ar1)
{
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  int min0, max0, min1, max1;
  float *inPtr0, *inPtr1;
  float *outPtr0, *outPtr1;
  float ar01, diff;

  inRegion->GetBounds2d(min0, max0, min1, max1);
  inRegion->GetIncrements2d(inInc0, inInc1);
  outRegion->GetIncrements2d(outInc0, outInc1);
  ar01 = sqrt(ar0 * ar0 + ar1 * ar1);
  inPtr1 = (float *)(inRegion->GetVoidPointer2d());
  outPtr1 = (float *)(outRegion->GetVoidPointer2d());
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
	if (fabs(diff) < ar0 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // right
      if (idx0 != max0)
	{
	diff = inPtr0[inInc0] - *inPtr0;
	if (fabs(diff) < ar0 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // up
      if (idx1 != min1)
	{
	diff = inPtr0[-inInc1] - *inPtr0;
	if (fabs(diff) < ar1 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // down
      if (idx1 != max1)
	{
	diff = inPtr0[inInc1] - *inPtr0;
	if (fabs(diff) < ar1 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // upper left
      if (idx0 != min0 && idx1 != min1)
	{
	diff = inPtr0[-inInc0-inInc1] - *inPtr0;
	if (fabs(diff) < ar01 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // upper right
      if (idx0 != max0 && idx1 != min1)
	{
	diff = inPtr0[inInc0-inInc1] - *inPtr0;
	if (fabs(diff) < ar01 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // lower left
      if (idx0 != min0 && idx1 != max1)
	{
	diff = inPtr0[-inInc0+inInc1] - *inPtr0;
	if (fabs(diff) < ar01 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      // lower right
      if (idx0 != max0 && idx1 != max1)
	{
	diff = inPtr0[inInc0+inInc1] - *inPtr0;
	if (fabs(diff) < ar01 * this->DiffusionThreshold)
	  {
	  *outPtr0 += diff * this->DiffusionFactor;
	  }
	}
      }
    }
}








