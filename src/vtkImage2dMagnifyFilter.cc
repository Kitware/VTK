/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2dMagnifyFilter.cc
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
#include "vtkImage2dMagnifyFilter.hh"
#include "vtkImageCache.hh"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImage2dMagnifyFilter::vtkImage2dMagnifyFilter()
{
  this->SetAxes2d(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetMagnificationFactors(1, 1);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImage2dMagnifyFilter::ComputeRequiredInputRegionBounds(
					       vtkImageRegion *outRegion,
					       vtkImageRegion *inRegion)
{
  int bounds[4];
  int idx;
  
  outRegion->GetBounds2d(bounds);
  
  // ignoring boundaries for now
  for (idx = 0; idx < 2; ++idx)
    {
    // For Min. Round Down
    if (bounds[idx*2] >= 0)
      {
      bounds[idx*2] /= this->MagnificationFactors[idx];
      }
    else
      {
      bounds[idx*2]= -1-(-bounds[idx*2]-1)/this->MagnificationFactors[idx];
      }
    // For Max. Round Down
    if (bounds[idx*2+1] >= 0)
      {
      bounds[idx*2+1] /= this->MagnificationFactors[idx];
      }
    else
      {
      bounds[idx*2+1]= -1-(-bounds[idx*2+1]-1)/this->MagnificationFactors[idx];
      }
    }
  
  inRegion->SetBounds2d(bounds);
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
void vtkImage2dMagnifyFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int idx;
  int imageBounds[4];

  inRegion->GetImageBounds2d(imageBounds);
  // Scale the output bounds
  for (idx = 0; idx < 2; ++idx)
    {
    imageBounds[2*idx] *= this->MagnificationFactors[idx];
    imageBounds[2*idx+1]
      = (imageBounds[2*idx+1]+1) * this->MagnificationFactors[idx] - 1;
    }

  outRegion->SetImageBounds2d(imageBounds);
}



//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// Updating the extra pixels is a small price to pay for the reduced
// complexity of boundry checking of the output.
void vtkImage2dMagnifyFilter::InterceptCacheUpdate(vtkImageRegion *region)
{
  int bounds[4];
  int idx;
  
  this->ComputeRequiredInputRegionBounds(region, region);
  
  region->GetBounds2d(bounds);
  for (idx = 0; idx < 2; ++idx)
    {
    bounds[idx*2] *= this->MagnificationFactors[idx];
    bounds[idx*2+1]= (bounds[idx*2+1]+1) * this->MagnificationFactors[idx] - 1;
    }
    
  region->SetBounds2d(bounds);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
void vtkImage2dMagnifyFilterExecute(vtkImage2dMagnifyFilter *self,
				    vtkImageRegion *inRegion, T *inPtr,
				    vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1;
  int idx0, idx1;
  int magIdx0, magIdx1;
  int inInc0, inInc1;
  int tmpInc0, tmpInc1;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1;
  T *tmpPtr0, *tmpPtr1;
  T *outPtr0, *outPtr1;
  int mag0, mag1;

  self->GetMagnificationFactors(mag0, mag1);
  
  // Get information to march through data 
  inRegion->GetIncrements2d(inInc0, inInc1);
  outRegion->GetIncrements2d(outInc0, outInc1);
  tmpInc0 = outInc0 * mag0;
  tmpInc1 = outInc1 * mag1;
  inRegion->GetBounds2d(min0, max0, min1, max1);

  // Loop through ouput pixels
  inPtr1 = inPtr;
  tmpPtr1 = outPtr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    inPtr0 = inPtr1;
    tmpPtr0 = tmpPtr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      
      // loop over magnified pixel
      outPtr1 = tmpPtr0;
      for (magIdx1 = 0; magIdx1 < mag1; ++magIdx1)
	{
	outPtr0 = outPtr1;
	for (magIdx0 = 0; magIdx0 < mag0; ++magIdx0)
	  {
	  // Replicate the pixel
	  *outPtr0 = *inPtr0;
	  
	  outPtr0 += outInc0;
	  }
	outPtr1 += outInc1;
	}
      
      inPtr0 += inInc0;
      tmpPtr0 += tmpInc0;
      }
    inPtr1 += inInc1;
    tmpPtr1 += tmpInc1;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.
void vtkImage2dMagnifyFilter::Execute2d(vtkImageRegion *inRegion, 
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
      vtkImage2dMagnifyFilterExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImage2dMagnifyFilterExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage2dMagnifyFilterExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage2dMagnifyFilterExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage2dMagnifyFilterExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute2d: Unknown DataType");
      return;
    }
}
















