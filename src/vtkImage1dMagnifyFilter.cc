/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage1dMagnifyFilter.cc
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
#include "vtkImage1dMagnifyFilter.hh"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImage1dMagnifyFilter::vtkImage1dMagnifyFilter()
{
  this->SetAxis1d(VTK_IMAGE_X_AXIS);
  this->SetMagnificationFactor(1);
  this->InterpolateOff();
}


//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImage1dMagnifyFilter::ComputeRequiredInputRegionBounds(
					       vtkImageRegion *outRegion,
					       vtkImageRegion *inRegion)
{
  int bounds[2];
  
  outRegion->GetBounds1d(bounds);
  
  // For Min. Round Down
  if (bounds[0] >= 0)
    {
    bounds[0] /= this->MagnificationFactor;
    }
  else
    {
    bounds[0]= -1-(-bounds[0]-1)/this->MagnificationFactor;
    }
  // For Max. Round Down
  if (bounds[1] >= 0)
    {
    bounds[1] /= this->MagnificationFactor;
    }
  else
    {
    bounds[1]= -1-(-bounds[1]-1)/this->MagnificationFactor;
    }

  // We need the last pixel if we are interpolating
  if (this->Interpolate)
    {
    ++bounds[1];
    }
    
  inRegion->SetBounds1d(bounds);
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
void vtkImage1dMagnifyFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int imageBounds[2];
  float aspectRatio;

  inRegion->GetImageBounds1d(imageBounds);
  inRegion->GetAspectRatio1d(aspectRatio);

  // Scale the output bounds
  imageBounds[0] *= this->MagnificationFactor;
  imageBounds[1] = (imageBounds[1]+1) * this->MagnificationFactor - 1;
  // Change the aspect ratio.
  aspectRatio *= (float)(this->MagnificationFactor);

  outRegion->SetImageBounds1d(imageBounds);
  outRegion->SetAspectRatio1d(aspectRatio);
}



//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// Updating the extra pixels is a small price to pay for the reduced
// complexity of boundry checking of the output.
void vtkImage1dMagnifyFilter::InterceptCacheUpdate(vtkImageRegion *region)
{
  int bounds[2];
  
  this->ComputeRequiredInputRegionBounds(region, region);
  
  region->GetBounds1d(bounds);
  bounds[0] *= this->MagnificationFactor;
  if (this->Interpolate)
    {
    bounds[1]= (bounds[1]) * this->MagnificationFactor - 1;
    }
  else
    {
    bounds[1]= (bounds[1]+1) * this->MagnificationFactor - 1;
    }
    
  region->SetBounds1d(bounds);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is 1d.
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
void vtkImage1dMagnifyFilterExecute(vtkImage1dMagnifyFilter *self,
				    vtkImageRegion *inRegion, T *inPtr,
				    vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1;
  int idx0, idx1;
  int magIdx;
  int inInc0, inInc1;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1;
  T *outPtr0, *outPtr1;
  int mag, interpolate;
  float step, val;

  mag = self->GetMagnificationFactor();
  interpolate = self->GetInterpolate();
  
  // Get information to march through data 
  inRegion->GetIncrements2d(inInc0, inInc1);
  outRegion->GetIncrements2d(outInc0, outInc1);
  inRegion->GetBounds2d(min0, max0, min1, max1);

  // Special cases
  if (interpolate)
    {
    --max0;
    }

  // Loop through input pixels
  inPtr1 = inPtr;
  outPtr1 = outPtr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    inPtr0 = inPtr1;
    outPtr0 = outPtr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      // Treat first pixel as special case
      *outPtr0 = *inPtr0;
      outPtr0 += outInc0;
      val = (float)(*inPtr0);
      if (interpolate)
	{
	step = ((float)(inPtr0[inInc0]) - (float)(*inPtr0)) / (float)(mag);
	}
      // loop over magnification factor
      for (magIdx = 1; magIdx < mag; ++magIdx)
	{
	if (interpolate)
	  {
	  val += step;
	  }
	// Replicate the pixel
	*outPtr0 = (T)(val);
	
	outPtr0 += outInc0;
	}
      
      inPtr0 += inInc0;
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
void vtkImage1dMagnifyFilter::Execute2d(vtkImageRegion *inRegion, 
					vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetVoidPointer2d();
  void *outPtr = outRegion->GetVoidPointer2d();
  
  vtkDebugMacro(<< "Execute2d: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetDataType() != outRegion->GetDataType())
    {
    vtkErrorMacro(<< "Execute2d: input DataType, " << inRegion->GetDataType()
                  << ", must match out DataType " << outRegion->GetDataType());
    return;
    }
  
  switch (inRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImage1dMagnifyFilterExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImage1dMagnifyFilterExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage1dMagnifyFilterExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage1dMagnifyFilterExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage1dMagnifyFilterExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute2d: Unknown DataType");
      return;
    }
}
















