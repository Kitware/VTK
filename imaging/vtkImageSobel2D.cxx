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
#include "vtkImageCache.h"
#include "vtkImageSobel2D.h"


//----------------------------------------------------------------------------
// Construct an instance of vtkImageSobel2D fitler.
vtkImageSobel2D::vtkImageSobel2D()
{
  this->KernelSize[0] = 3;
  this->KernelSize[1] = 3;
  this->KernelSize[2] = 1;
  this->KernelMiddle[0] = 1;
  this->KernelMiddle[1] = 1;
  this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}


//----------------------------------------------------------------------------
void vtkImageSobel2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
}



//----------------------------------------------------------------------------
void vtkImageSobel2D::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(2);
  this->Output->SetScalarType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageSobel2DExecute(vtkImageSobel2D *self,
				   vtkImageData *inData, T *inPtr, 
				   vtkImageData *outData, int *outExt, 
				   float *outPtr, int id)
{
  float r0, r1, *r;
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2;
  int outIdx0, outIdx1, outIdx2;
  int outInc0, outInc1, outInc2;
  float *outPtr0, *outPtr1, *outPtr2, *outPtrV;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  // For sobel function convolution (Left Right incs for each axis)
  int inInc0L, inInc0R, inInc1L, inInc1R;
  T *inPtrL, *inPtrR;
  float sum;
  // Boundary of input image
  int inWholeMin0,inWholeMax0;
  int inWholeMin1,inWholeMax1;
  int inWholeMin2,inWholeMax2;
  unsigned long count = 0;
  unsigned long target;

  // Get boundary information 
  self->GetInput()->GetWholeExtent(inWholeMin0,inWholeMax0,
			   inWholeMin1,inWholeMax1, inWholeMin2,inWholeMax2);
  
  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2); 
  outData->GetIncrements(outInc0, outInc1, outInc2); 
  min0 = outExt[0];   max0 = outExt[1];
  min1 = outExt[2];   max1 = outExt[3];
  min2 = outExt[4];   max2 = outExt[5];
  
  // We want the input pixel to correspond to output
  inPtr = (T *)(inData->GetScalarPointer(min0,min1,min2));

  // The data spacing is important for computing the gradient.
  // Scale so it has the same range as gradient.
  r = inData->GetSpacing();
  r0 = 0.125 / r[0];
  r1 = 0.125 / r[1];
  // ignore r2

  target = (unsigned long)((max2-min2+1)*(max1-min1+1)/50.0);
  target++;
  
  // loop through pixels of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
    {
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = min1; !self->AbortExecute && outIdx1 <= max1; ++outIdx1)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
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
	++outPtrV;
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
    outPtr2 += outInc2;
    inPtr2 += inInc2;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageSobel2D::ThreadedExecute(vtkImageData *inData, 
				      vtkImageData *outData,
				      int outExt[6], int id)
{
  void *inPtr, *outPtr;
  int inExt[6];
  
  this->ComputeRequiredInputUpdateExtent(inExt, outExt);  
  
  inPtr = inData->GetScalarPointerForExtent(inExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that output is type float.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outData->GetScalarType())
                  << ", must be float");
    return;
    }
  
  // this filter cannot handle multi component input.
  if (inData->GetNumberOfScalarComponents() != 1)
    {
    vtkWarningMacro("Expecting input with only one compenent.\n");
    }
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageSobel2DExecute(this,
			  inData, (float *)(inPtr), 
			  outData, outExt, (float *)(outPtr),id);
      break;
    case VTK_INT:
      vtkImageSobel2DExecute(this, 
			  inData, (int *)(inPtr), 
			  outData, outExt, (float *)(outPtr),id);
      break;
    case VTK_SHORT:
      vtkImageSobel2DExecute(this, 
			  inData, (short *)(inPtr), 
			  outData, outExt, (float *)(outPtr),id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageSobel2DExecute(this, 
			  inData, (unsigned short *)(inPtr), 
			  outData, outExt, (float *)(outPtr),id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageSobel2DExecute(this, 
			  inData, (unsigned char *)(inPtr), 
			  outData, outExt, (float *)(outPtr),id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}






