/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify1D.cxx
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
#include "vtkImageMagnify1D.h"



//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageMagnify1D::vtkImageMagnify1D()
{
  this->SetAxes(VTK_IMAGE_X_AXIS);
  this->SetMagnificationFactor(1);
  this->InterpolateOff();

  // For better performance, the execute function was written as a 2d.
  this->ExecuteDimensionality = 2;
  this->Dimensionality = 1;
}


//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImageMagnify1D::ComputeRequiredInputRegionExtent(
					       vtkImageRegion *outRegion,
					       vtkImageRegion *inRegion)
{
  int extent[2];
  
  outRegion->GetExtent(1, extent);
  
  // For Min. Round Down
  extent[0] = 
    (int)(floor((float)(extent[0]) / (float)(this->MagnificationFactor)));
  
  if (this->Interpolate)
    {
    // Round Up
    extent[1] = 
      (int)(ceil((float)(extent[1]) / (float)(this->MagnificationFactor)));
    }
  else
    {
    extent[1] =
      (int)(floor((float)(extent[1]) / (float)(this->MagnificationFactor)));
    }
  
  inRegion->SetExtent(1, extent);
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
void vtkImageMagnify1D::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int imageExtent[2];
  float aspectRatio;

  inRegion->GetImageExtent(1, imageExtent);
  inRegion->GetAspectRatio(aspectRatio);

  // Scale the output extent
  imageExtent[0] *= this->MagnificationFactor;
  if (this->Interpolate)
    {
    imageExtent[1] *= this->MagnificationFactor;
    }
  else
    {
    imageExtent[1] = (imageExtent[1]+1) * this->MagnificationFactor - 1;
    }
  
  // Change the aspect ratio.
  aspectRatio /= (float)(this->MagnificationFactor);

  outRegion->SetImageExtent(1, imageExtent);
  outRegion->SetAspectRatio(aspectRatio);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is 1d.
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
void vtkImageMagnify1DExecute(vtkImageMagnify1D *self,
			      vtkImageRegion *inRegion, T *inPtr,
			      vtkImageRegion *outRegion, T *outPtr)
{
  int outMin0, outMax0, outMin1, outMax1, inMin0, inMax0;
  int outIdx0, outIdx1; 
  int inInc0, inInc1, outInc0, outInc1;
  T *inPtr0, *inPtr1, *outPtr0, *outPtr1;
  int interpolate, magFactor, magStart, magIdx;
  float step, val;

  // Avoid warnings.
  step = 0.0;

  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);
  inRegion->GetExtent(inMin0, inMax0);
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);

  // Compute interpolation stuff
  interpolate = self->GetInterpolate();
  magFactor = self->GetMagnificationFactor();
  magStart = outMin0 - (inMin0 * magFactor);
  
  // Loop through output pixels
  inPtr1 = inPtr;
  outPtr1 = outPtr;
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    inPtr0 = inPtr1;
    outPtr0 = outPtr1;
    magIdx = magStart;
    // compute the state just before this sample.
    val = *inPtr0;
    if (magIdx != 0 && magIdx != 1)
      {
      step = ((float)(inPtr0[inInc0]) - val) / (float)(magFactor);
      val += step * (magStart - 1);
      }
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
      
      // Treat first pixel as special case
      if (magIdx == 0)
	{
	val = (float)(*inPtr0);
	*outPtr0 = *inPtr0;
	}
      else if (magIdx == 1)
	{
	step = ((float)(inPtr0[inInc0]) - val) / (float)(magFactor);
	val += step;
	*outPtr0 = (T)(val);
	}
      else
	{
	val += step;
	*outPtr0 = (T)(val);
	}

      ++magIdx;	
      if (magIdx >= magFactor)
	{
	magIdx = 0;
	inPtr0 += inInc0;
	}
      outPtr0 += outInc0;
      }
    inPtr1 += inInc1;
    outPtr1 += outInc1;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.
void vtkImageMagnify1D::Execute(vtkImageRegion *inRegion, 
				  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
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
      vtkImageMagnify1DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageMagnify1DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageMagnify1DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMagnify1DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMagnify1DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















