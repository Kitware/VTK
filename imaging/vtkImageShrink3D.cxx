/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.cxx
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
#include "vtkImageShrink3D.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageShrink3D::vtkImageShrink3D()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
  this->SetShrinkFactors(1, 1, 1);
  this->SetShift(0, 0, 0);
  this->AveragingOn();
}

//----------------------------------------------------------------------------
void vtkImageShrink3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);
  os << indent << "ShrinkFactors: (" << this->ShrinkFactors[0] << ", "
     << this->ShrinkFactors[1] << ", " << this->ShrinkFactors[2] << ")\n";
  os << indent << "Shift: (" << this->Shift[0] << ", "
     << this->Shift[1] << ", " << this->Shift[2] << ")\n";
}

//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
void vtkImageShrink3D::ComputeRequiredInputRegionExtent(
					       vtkImageRegion *outRegion,
					       vtkImageRegion *inRegion)
{
  int extent[6];
  int idx;
  
  outRegion->GetExtent(3, extent);
  
  for (idx = 0; idx < 3; ++idx)
    {
    // For Min.
    extent[idx*2] = extent[idx*2] * this->ShrinkFactors[idx] 
      + this->Shift[idx];
    // For Max.
    extent[idx*2+1] = extent[idx*2+1] * this->ShrinkFactors[idx]
      + this->Shift[idx];
    // If we are averaging, we need a little more
    if (this->Averaging)
      {
      extent[idx*2+1] += this->ShrinkFactors[idx] - 1;
      }
    }
  
  inRegion->SetExtent(3, extent);
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
// Any problems with roundoff or negative numbers ???
void vtkImageShrink3D::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int idx;
  int imageExtent[6];
  float aspectRatio[3];

  inRegion->GetImageExtent(3, imageExtent);
  inRegion->GetAspectRatio(3, aspectRatio);

  for (idx = 0; idx < 3; ++idx)
    {
    // Scale the output extent
    imageExtent[2*idx] = (imageExtent[2*idx] - this->Shift[idx]) 
      / this->ShrinkFactors[idx];
    imageExtent[2*idx+1] = (imageExtent[2*idx+1] - this->Shift[idx])
      / this->ShrinkFactors[idx];
    // Change the aspect ratio.
    aspectRatio[idx] *= (float)(this->ShrinkFactors[idx]);
    }

  outRegion->SetImageExtent(3, imageExtent);
  outRegion->SetAspectRatio(3, aspectRatio);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
void vtkImageShrink3DExecute(vtkImageShrink3D *self,
				   vtkImageRegion *inRegion, T *inPtr,
				   vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx0, outIdx1, outIdx2, inIdx0, inIdx1, inIdx2;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  int outInc0, outInc1, outInc2;
  T *outPtr0, *outPtr1, *outPtr2;
  int tmpInc0, tmpInc1, tmpInc2;
  T *tmpPtr0, *tmpPtr1, *tmpPtr2;
  int min0, max0, min1, max1, min2, max2;
  int factor0, factor1, factor2;
  int averaging;
  float sum, norm;

  averaging = self->GetAveraging();
  self->GetShrinkFactors(factor0, factor1, factor2);
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1, inInc2);
  tmpInc0 = inInc0 * factor0;
  tmpInc1 = inInc1 * factor1;
  tmpInc2 = inInc2 * factor2;
  outRegion->GetIncrements(outInc0, outInc1, outInc2);
  outRegion->GetExtent(min0, max0, min1, max1, min2, max2);
  norm = 1.0 / (float)(factor0 * factor1 * factor2);

  // Loop through ouput pixels
  tmpPtr2 = inPtr;
  outPtr2 = outPtr;
  for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
    {
    tmpPtr1 = tmpPtr2;
    outPtr1 = outPtr2;
    for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
      {
      tmpPtr0 = tmpPtr1;
      outPtr0 = outPtr1;
      for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	{
      
	// Copy pixel from this location
	if ( ! averaging)
	  {
	  *outPtr0 = *tmpPtr0;
	  }
	else
	  {
	  sum = 0.0;
	  // Loop through neighborhood pixels
	  inPtr2 = tmpPtr0;
	  for (inIdx2 = 0; inIdx2 < factor2; ++inIdx2)
	    {
	    inPtr1 = inPtr2;
	    for (inIdx1 = 0; inIdx1 < factor1; ++inIdx1)
	      {
	      inPtr0 = inPtr1;
	      for (inIdx0 = 0; inIdx0 < factor0; ++inIdx0)
		{
		sum += (float)(*inPtr0);
		inPtr0 += inInc0;
		}
	      inPtr1 += inInc1;
	      }
	    inPtr2 += inInc2;
	    }
	  *outPtr0 = (T)(sum * norm);
	  }
	
	tmpPtr0 += tmpInc0;
	outPtr0 += outInc0;
	}
      tmpPtr1 += tmpInc1;
      outPtr1 += outInc1;
      }
    tmpPtr2 += tmpInc2;
    outPtr2 += outInc2;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.
void vtkImageShrink3D::Execute(vtkImageRegion *inRegion, 
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
      vtkImageShrink3DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageShrink3DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageShrink3DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageShrink3DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageShrink3DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















