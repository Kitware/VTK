/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel3D.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageSobel3D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSobel3D fitler.
vtkImageSobel3D::vtkImageSobel3D()
{
  this->SetFilteredAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
void vtkImageSobel3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageFilter::PrintSelf(os, indent);
}



//----------------------------------------------------------------------------
void vtkImageSobel3D::SetFilteredAxes(int axis0, int axis1, int axis2)
{
  int axes[4];
  
  if (axis0 > 3 || axis1 > 3 || axis2 > 3)
    {
    vtkErrorMacro("SetFilteredAxes: Component axis not allowed");
    return;
    }
  axes[0] = axis0;
  axes[1] = axis1;
  axes[2] = axis2;
  axes[3] = VTK_IMAGE_COMPONENT_AXIS;
  this->vtkImageFilter::SetFilteredAxes(4,axes);
}

//----------------------------------------------------------------------------
void vtkImageSobel3D::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(this->NumberOfFilteredAxes);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageSobel3D::ComputeRequiredInputUpdateExtent()
{
  int extent[8];
  int *wholeExtent;
  int idx, axis;

  wholeExtent = this->Input->GetWholeExtent();
  this->Output->GetUpdateExtent(extent);
  
  // grow input extent.
  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    extent[axis*2] -= 1;
    extent[axis*2+1] += 1;
    // we must clip extent with whole extent to handle boundaries.
    if (extent[axis*2] < wholeExtent[axis*2])
      {
      extent[axis*2] = wholeExtent[axis*2];
      }
    if (extent[axis*2 + 1] > wholeExtent[axis*2 + 1])
      {
      extent[axis*2 + 1] = wholeExtent[axis*2 + 1];
      }
    }
  
  this->Input->SetUpdateExtent(extent);
}





//----------------------------------------------------------------------------
// Description:
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageSobel3DExecute(vtkImageSobel3D *self,
			    vtkImageRegion *inRegion, T *inPtr, 
			    vtkImageRegion *outRegion, float *outPtr)
{
  float r0, r1, r2;
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2;
  int outIdx0, outIdx1, outIdx2;
  int outInc0, outInc1, outInc2, outIncV;
  float *outPtr0, *outPtr1, *outPtr2, *outPtrV;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  // For sobel function convolution (Left Right incs for each axis)
  int inInc0L, inInc0R, inInc1L, inInc1R, inInc2L, inInc2R;
  T *inPtrL, *inPtrR;
  float sum;
  // Boundary of input image
  int inWholeMin0, inWholeMax0, inWholeMin1, inWholeMax1;
  int inWholeMin2, inWholeMax2;

  // avoid warings. (unused parameter)
  self = self;
  
  // Get boundary information 
  inRegion->GetWholeExtent(inWholeMin0,inWholeMax0, 
			   inWholeMin1,inWholeMax1,
			   inWholeMin2,inWholeMax2);
  
  // Get information to march through data (skip component)
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, outIncV);
  outRegion->GetExtent(min0,max0, min1,max1, min2,max2);
  
  // We want the input pixel to correspond to output
  inPtr = (T *)(inRegion->GetScalarPointer(min0,min1,min2));

  // The data spacing is important for computing the gradient.
  // Scale so it has the same range as gradient.
  inRegion->GetSpacing(r0, r1, r2);
  r0 = 0.060445 / r0;
  r1 = 0.060445 / r1;
  r2 = 0.060445 / r2;
  
  // loop through pixels of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
    {
    inInc2L = (outIdx2 == inWholeMin2) ? 0 : -inInc2;
    inInc2R = (outIdx2 == inWholeMax2) ? 0 : inInc2;

    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
      {
      inInc1L = (outIdx1 == inWholeMin1) ? 0 : -inInc1;
      inInc1R = (outIdx1 == inWholeMax1) ? 0 : inInc1;

      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	{
	inInc0L = (outIdx0 == inWholeMin0) ? 0 : -inInc0;
	inInc0R = (outIdx0 == inWholeMax0) ? 0 : inInc0;

	// compute vector.
	outPtrV = outPtr0;
	// 12 Plane
	inPtrL = inPtr0 + inInc0L;
	inPtrR = inPtr0 + inInc0R;
	sum = 2.0 * (*inPtrR - *inPtrL);
	sum += (inPtrR[inInc1L] + inPtrR[inInc1R] 
		+ inPtrR[inInc2L] + inPtrR[inInc2R]);
	sum += (0.586 * (inPtrR[inInc1L+inInc2L] + inPtrR[inInc1L+inInc2R]
			 + inPtrR[inInc1R+inInc2L] + inPtrR[inInc1R+inInc2R]));
	sum -= (inPtrL[inInc1L] + inPtrL[inInc1R] 
		+ inPtrL[inInc2L] + inPtrL[inInc2R]);
	sum -= (0.586 * (inPtrL[inInc1L+inInc2L] + inPtrL[inInc1L+inInc2R]
			 + inPtrL[inInc1R+inInc2L] + inPtrL[inInc1R+inInc2R]));
	*outPtrV = sum * r0;
	outPtrV += outIncV;
	// 02 Plane
	inPtrL = inPtr0 + inInc1L;
	inPtrR = inPtr0 + inInc1R;
	sum = 2.0 * (*inPtrR - *inPtrL);
	sum += (inPtrR[inInc0L] + inPtrR[inInc0R] 
		+ inPtrR[inInc2L] + inPtrR[inInc2R]);
	sum += (0.586 * (inPtrR[inInc0L+inInc2L] + inPtrR[inInc0L+inInc2R]
			 + inPtrR[inInc0R+inInc2L] + inPtrR[inInc0R+inInc2R]));
	sum -= (inPtrL[inInc0L] + inPtrL[inInc0R] 
		+ inPtrL[inInc2L] + inPtrL[inInc2R]);
	sum -= (0.586 * (inPtrL[inInc0L+inInc2L] + inPtrL[inInc0L+inInc2R]
			 + inPtrL[inInc0R+inInc2L] + inPtrL[inInc0R+inInc2R]));
	*outPtrV = sum * r1;
	outPtrV += outIncV;
	// 01 Plane
	inPtrL = inPtr0 + inInc2L;
	inPtrR = inPtr0 + inInc2R;
	sum = 2.0 * (*inPtrR - *inPtrL);
	sum += (inPtrR[inInc0L] + inPtrR[inInc0R] 
		+ inPtrR[inInc1L] + inPtrR[inInc1R]);
	sum += (0.586 * (inPtrR[inInc0L+inInc1L] + inPtrR[inInc0L+inInc1R]
			 + inPtrR[inInc0R+inInc1L] + inPtrR[inInc0R+inInc1R]));
	sum -= (inPtrL[inInc0L] + inPtrL[inInc0R] 
		+ inPtrL[inInc1L] + inPtrL[inInc1R]);
	sum -= (0.586 * (inPtrL[inInc0L+inInc1L] + inPtrL[inInc0L+inInc1R]
			 + inPtrL[inInc0R+inInc1L] + inPtrL[inInc0R+inInc1R]));
	*outPtrV = sum * r2;
	outPtrV += outIncV;

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


//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageSobel3D::Execute(vtkImageRegion *inRegion, 
			      vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that output is type float.
  if (outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outRegion->GetScalarType())
                  << ", must be float");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageSobel3DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageSobel3DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageSobel3DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageSobel3DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageSobel3DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}






