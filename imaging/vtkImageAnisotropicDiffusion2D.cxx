/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion2D.cxx
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
#include "vtkImageAnisotropicDiffusion2D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageAnisotropicDiffusion2D fitler.
vtkImageAnisotropicDiffusion2D::vtkImageAnisotropicDiffusion2D()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  
  this->HandleBoundariesOn();
  this->SetNumberOfIterations(4);
  this->DiffusionThreshold = 5.0;
  this->DiffusionFactor = 1;
  this->EdgesOn();
  this->CornersOn();
  this->GradientMagnitudeThresholdOff();

  this->ExecuteDimensionality = 2;
  this->Dimensionality = 2;
}


//----------------------------------------------------------------------------
void 
vtkImageAnisotropicDiffusion2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";
  os << indent << "DiffusionThreshold: " << this->DiffusionThreshold << "\n";
  os << indent << "DiffusionFactor: " << this->DiffusionFactor << "\n";

  if (this->Edges)
    {
    os << indent << "Edges: On\n";
    }
  else
    {
    os << indent << "Edges: Off\n";
    }

  if (this->Corners)
    {
    os << indent << "Corners: On\n";
    }
  else
    {
    os << indent << "Corners: Off\n";
    }

  if (this->GradientMagnitudeThreshold)
    {
    os << indent << "GradientMagnitudeThreshold: On\n";
    }
  else
    {
    os << indent << "GradientMagnitudeThreshold: Off\n";
    }
}



//----------------------------------------------------------------------------
// Description:
// This method sets the number of inputs which also affects the
// input neighborhood needed to compute one output pixel.
void vtkImageAnisotropicDiffusion2D::SetNumberOfIterations(int num)
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
void vtkImageAnisotropicDiffusion2D::Execute(vtkImageRegion *inRegion, 
					     vtkImageRegion *outRegion)
{
  int idx;
  int extent[4];
  float ar0, ar1;
  vtkImageRegion *in;
  vtkImageRegion *out;
  vtkImageRegion *temp;


  inRegion->GetSpacing(ar0, ar1);

  // make the temporary regions to iterate over.
  in = vtkImageRegion::New();
  out = vtkImageRegion::New();
  
  // might as well make these floats
  in->SetExtent(VTK_IMAGE_DIMENSIONS, inRegion->GetExtent());
  in->SetScalarType(VTK_FLOAT);
  in->CopyRegionData(inRegion);
  out->SetExtent(VTK_IMAGE_DIMENSIONS, inRegion->GetExtent());
  out->SetScalarType(VTK_FLOAT);
  out->AllocateScalars();
  
  // To compute extent of diffusion which will shrink.
  outRegion->GetExtent(2, extent);
  
  // Loop performing the diffusion
  // Note: region extent could get smaller as the diffusion progresses
  // (but never get smaller than output region).
  for (idx = this->NumberOfIterations - 1; idx >= 0; --idx)
    {
    this->Iterate(in, out, ar0, ar1, extent, idx);
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
void vtkImageAnisotropicDiffusion2D::Iterate(vtkImageRegion *inRegion, 
					     vtkImageRegion *outRegion,
					     float ar0, float ar1,
					     int *coreExtent, int count)
{
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  int inMin0, inMax0, inMin1, inMax1;
  int min0, max0, min1, max1;
  float *inPtr0, *inPtr1;
  float *outPtr0, *outPtr1;
  float th0, th1, th01;
  float df0, df1, df01;
  float temp, sum;

  inRegion->GetExtent(inMin0, inMax0, inMin1, inMax1);
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);

  // Avoid warnings.
  th0 = th1 = th01 = df0 = df1 = df01 = 0.0;
  
  // Compute direction specific diffusion thresholds and factors.
  sum = 0.0;
  if (this->Edges)
    {
    th0 = ar0 * this->DiffusionThreshold;
    df0 = 1.0 / ar0;
    th1 = ar1 * this->DiffusionThreshold;
    df1 = 1.0 / ar1;
    // two edges per direction.
    sum += 2.0 * (df0 + df1);
    }
  if (this->Corners)
    {
    temp = sqrt(ar0*ar0 + ar1*ar1);
    th01 = temp * this->DiffusionThreshold;
    df01 = 1 / temp;
    // four corners per plane
    sum += 4 * (df01);
    }

  if (sum > 0.0)
    {
    temp = this->DiffusionFactor / sum;
    df0 *= temp;
    df1 *= temp;
    df01 *= temp;
    }
  else
    {
    vtkWarningMacro(<< "Iterate: NO NEIGHBORS");
    return;
    }

  // Compute the shrinking extent to loop over.
  min0 = coreExtent[0] - count;
  max0 = coreExtent[1] + count;
  min1 = coreExtent[2] - count;
  max1 = coreExtent[3] + count;
  // intersection
  min0 = (min0 > inMin0) ? min0 : inMin0;
  max0 = (max0 < inMax0) ? max0 : inMax0;
  min1 = (min1 > inMin1) ? min1 : inMin1;
  max1 = (max1 < inMax1) ? max1 : inMax1;
  
  vtkDebugMacro(<< "Iteration count: " << count << " ("
  << min0 << ", " << max0 << ", " << min1 << ", " << max1 << ")");
  
  // I apologize for explicitely diffusing each neighbor, but it is the easiest
  // way to deal with the boundary conditions.  Besides it is fast.
  // (Are you sure every one is correct?!!!)
  inPtr1 = (float *)(inRegion->GetScalarPointer(min0, min1));
  outPtr1 = (float *)(outRegion->GetScalarPointer(min0, min1));
  for (idx1 = min1; idx1 <= max1; ++idx1, inPtr1+=inInc1, outPtr1+=outInc1)
    {
    inPtr0 = inPtr1;
    outPtr0 = outPtr1;    
    for (idx0 = min0; idx0 <= max0; ++idx0, inPtr0+=inInc0, outPtr0+=outInc0)
      {
      // Copy center
      *outPtr0 = *inPtr0;
      
      // Special case for gradient magnitude threhsold 
      if (this->GradientMagnitudeThreshold)
	{
	float d0, d1;
	// compute the gradient magnitude (central differences).
	d0  = (idx0 != inMax0) ? inPtr0[inInc0] : *inPtr0;
	d0 -= (idx0 != inMin0) ? inPtr0[-inInc0] : *inPtr0;
	d0 /= ar0;
	d1  = (idx1 != inMax1) ? inPtr0[inInc1] : *inPtr0;
	d1 -= (idx1 != inMin1) ? inPtr0[-inInc1] : *inPtr0;
	d1 /= ar1;
	// If magnitude is big, don't diffuse.
	d0 = sqrt(d0*d0 + d1*d1);
	if (d0 > this->DiffusionThreshold)
	  {
	  // hack to not diffuse
	  th0 = th1 = th01 = 0.0;
	  }
	else
	  {
	  // hack to diffuse
	  th0 = th1 = th01 = VTK_LARGE_FLOAT;
	  }
	}
      
      // Start diffusing
      if (this->Edges)
	{
	// left
	if (idx0 != inMin0)
	  {
	  temp = inPtr0[-inInc0] - *inPtr0;
	  if (fabs(temp) < th0)
	    {
	    *outPtr0 += temp * df0;
	    }
	  }
	// right
	if (idx0 != inMax0)
	  {
	  temp = inPtr0[inInc0] - *inPtr0;
	  if (fabs(temp) < th0)
	    {
	    *outPtr0 += temp * df0;
	    }
	  }
	// up
	if (idx1 != inMin1)
	  {
	  temp = inPtr0[-inInc1] - *inPtr0;
	  if (fabs(temp) < th1)
	    {
	    *outPtr0 += temp * df1;
	    }
	  }
	// down
	if (idx1 != inMax1)
	  {
	  temp = inPtr0[inInc1] - *inPtr0;
	  if (fabs(temp) < th1)
	    {
	    *outPtr0 += temp * df1;
	    }
	  }
	}
      
      if (this->Corners)
	{
	// left up
	if (idx0 != inMin0 && idx1 != inMin1)
	  {
	  temp = inPtr0[-inInc0-inInc1] - *inPtr0;
	  if (fabs(temp) < th01)
	    {
	    *outPtr0 += temp * df01;
	    }
	  }
	// right up
	if (idx0 != inMax0 && idx1 != inMin1)
	  {
	  temp = inPtr0[inInc0-inInc1] - *inPtr0;
	  if (fabs(temp) < th01)
	    {
	    *outPtr0 += temp * df01;
	    }
	  }
	// left down
	if (idx0 != inMin0 && idx1 != inMax1)
	  {
	  temp = inPtr0[-inInc0+inInc1] - *inPtr0;
	  if (fabs(temp) < th01)
	      {
	      *outPtr0 += temp * df01;
	      }
	  }
	// right down
	if (idx0 != inMax0 && idx1 != inMax1)
	  {
	  temp = inPtr0[inInc0+inInc1] - *inPtr0;
	  if (fabs(temp) < th01)
	    {
	    *outPtr0 += temp * df01;
	    }
	  }
	}
      }
    }
}


  






