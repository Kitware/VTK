/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel2D.cxx
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
#include "vtkImageSobel2D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSobel2D fitler.
vtkImageSobel2D::vtkImageSobel2D()
{
  this->SetFilteredAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetOutputScalarType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
void vtkImageSobel2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageFilter::PrintSelf(os, indent);
}



//----------------------------------------------------------------------------
void vtkImageSobel2D::SetFilteredAxes(int axis0, int axis1)
{
  int axes[3];
  
  if (axis0 > 3 || axis1 > 3)
    {
    vtkErrorMacro("SetFilteredAxes: Component axis not allowed");
    return;
    }
  axes[0] = axis0;
  axes[1] = axis1;
  axes[2] = VTK_IMAGE_COMPONENT_AXIS;
  this->vtkImageFilter::SetFilteredAxes(3,axes);
}

//----------------------------------------------------------------------------
void vtkImageSobel2D::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(this->NumberOfFilteredAxes);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageSobel2D::ComputeRequiredInputUpdateExtent()
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
static void vtkImageSobel2DExecute(vtkImageSobel2D *self,
			    vtkImageRegion *inRegion, T *inPtr, 
			    vtkImageRegion *outRegion, float *outPtr)
{
  float r0, r1;
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1;
  int outIdx0, outIdx1;
  int outInc0, outInc1, outIncV;
  float *outPtr0, *outPtr1, *outPtrV;
  int inInc0, inInc1;
  T *inPtr0, *inPtr1;
  // For sobel function convolution (Left Right incs for each axis)
  int inInc0L, inInc0R, inInc1L, inInc1R;
  T *inPtrL, *inPtrR;
  float sum;
  // Boundary of input image
  int inWholeMin0, inWholeMax0, inWholeMin1, inWholeMax1;

  // avoid warings. (unused parameter)
  self = self;
  
  // Get boundary information 
  inRegion->GetWholeExtent(inWholeMin0,inWholeMax0, 
			   inWholeMin1,inWholeMax1);
  
  // Get information to march through data (skip component)
  inRegion->GetIncrements(inInc0, inInc1); 
  outRegion->GetIncrements(outInc0, outInc1); 
  outRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, outIncV);
  outRegion->GetExtent(min0,max0, min1,max1);
  
  // We want the input pixel to correspond to output
  inPtr = (T *)(inRegion->GetScalarPointer(min0,min1));

  // The data spacing is important for computing the gradient.
  // Scale so it has the same range as gradient.
  inRegion->GetSpacing(r0, r1);
  r0 = 0.125 / r0;
  r1 = 0.125 / r1;
  
  // loop through pixels of output
  outPtr1 = outPtr;
  inPtr1 = inPtr;
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
      // 0 direction
      inPtrL = inPtr0 + inInc0L;
      inPtrR = inPtr0 + inInc0R;
      sum = 2.0 * (*inPtrR - *inPtrL);
      sum += (inPtrR[inInc1L] + inPtrR[inInc1R]);
      sum -= (inPtrL[inInc1L] + inPtrL[inInc1R]);

      *outPtrV = sum * r0;
      outPtrV += outIncV;
      // 1 direction
      inPtrL = inPtr0 + inInc1L;
      inPtrR = inPtr0 + inInc1R;
      sum = 2.0 * (*inPtrR - *inPtrL);
      sum += (inPtrR[inInc0L] + inPtrR[inInc0R]);
      sum -= (inPtrL[inInc0L] + inPtrL[inInc0R]);
      *outPtrV = sum * r1;
      
      outPtr0 += outInc0;
      inPtr0 += inInc0;
      }
    outPtr1 += outInc1;
    inPtr1 += inInc1;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageSobel2D::Execute(vtkImageRegion *inRegion, 
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
      vtkImageSobel2DExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageSobel2DExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageSobel2DExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageSobel2DExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageSobel2DExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}






