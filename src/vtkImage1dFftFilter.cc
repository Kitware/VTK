/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage1dFftFilter.cc
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
#include "vtkImage1dFftFilter.hh"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImage1dFftFilter fitler.
vtkImage1dFftFilter::vtkImage1dFftFilter()
{
  this->SetAxes2d(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS);
  // Output is always floats.
  this->SetOutputDataType(VTK_IMAGE_FLOAT);
  this->InputRealComponent = 0;
  this->InputImaginaryComponent = 1;
}

//----------------------------------------------------------------------------
// Description:
// This 1d filter is actually a 2d filter with the component axis as the first
// axis (axis0).
void vtkImage1dFftFilter::SetAxis1d(int axis)
{
  this->SetAxes2d(VTK_IMAGE_COMPONENT_AXIS, axis);
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image bounds of this filters
// input, and changes the region to hold the image bounds of this filters
// output.  The image changes to multispectral.
void 
vtkImage1dFftFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion,
						   vtkImageRegion *outRegion)
{
  int min, max;

  // shrink output image bounds.
  inRegion->GetImageBounds1d(min, max);
  // We could check to see if the input actually contains specified real and
  // imaginary components.
  // Output components are always 0, and 1.
  min = 0;
  max = 1;
  outRegion->SetImageBounds1d(min, max);
}

//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// We might as well create both real and imaginary components.
void vtkImage1dFftFilter::InterceptCacheUpdate(vtkImageRegion *region)
{
  int min, max;
  
  region->GetBounds1d(min, max);
  if (min < 0 || max > 1)
    {
    vtkErrorMacro(<< "Only two channels to request 0 and 1");
    }
  
  region->SetBounds1d(0, 1);
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImage1dFftFilter::ComputeRequiredInputRegionBounds(
		   vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int bounds[4];
  int min, max;

  // Avoid a warning message.
  outRegion = outRegion;
  // compute the smallest bounds that contains both real and imaginary 
  // components.
  if (this->InputRealComponent < this->InputImaginaryComponent)
    {
    min = this->InputRealComponent;
    max = this->InputImaginaryComponent;
    }
  else
    {
    min = this->InputImaginaryComponent;
    max = this->InputRealComponent;
    }

  // Eliminate a component if it is not contained in the image bounds.
  inRegion->GetImageBounds2d(bounds);
  if (min < bounds[0])
    {
    min = max;
    }
  if (max > bounds[1])
    {
    max = min;
    }
  if (max > bounds[1])
    {
    vtkErrorMacro(<< "Both real and imaginary components are out of bounds.");
    return;
    }

  bounds[0] = min;
  bounds[1] = max;
  inRegion->SetBounds2d(bounds);
}

//----------------------------------------------------------------------------
// Description:
// This templated execute method handles any type input, but the output
// is always floats. Axis 0 should be components. FFT is performed on axis 1.
template <class T>
void vtkImage1dFftFilterExecute2d(vtkImage1dFftFilter *self,
				  vtkImageRegion *inRegion, T *inPtr,
				  vtkImageRegion *outRegion, float *outPtr)
{
  vtkImageComplex *inComplex;
  vtkImageComplex *outComplex;
  vtkImageComplex *pComplex;
  int realComp, imagComp;
  T *inPtrReal, *inPtrImag;
  int inMin0, inMax0, inMin1, inMax1, inSize1;
  int inInc0, inInc1;
  float *outPtrReal, *outPtrImag;
  int outMin0, outMax0, outMin1, outMax1;
  int outInc0, outInc1;
  int idx;
  
  // avoid warnings
  inPtr = inPtr;
  // Get information to march through data 
  inRegion->GetIncrements2d(inInc0, inInc1);
  inRegion->GetBounds2d(inMin0, inMax0, inMin1, inMax1);
  inSize1 = inMax1 - inMin1 + 1;
  
  // Allocate the arrays of complex numbers
  inComplex = new vtkImageComplex[inSize1];
  outComplex = new vtkImageComplex[inSize1];
  
  // Convert the input to complex numbers.
  // The complexity is because real or imaginary may not exist.
  // Set up the pointers (NULL => does not exist).
  realComp = self->GetInputRealComponent();
  imagComp = self->GetInputImaginaryComponent();
  if (realComp > inMax0 || realComp < inMin0)
    {
    inPtrReal = NULL;
    }
  else
    {
    inPtrReal = (T *)(inRegion->GetVoidPointer2d(realComp,inMin1));
    }
  if (imagComp > inMax0 || imagComp < inMin0)
    {
    inPtrImag = NULL;
    }
  else
    {
    inPtrImag = (T *)(inRegion->GetVoidPointer2d(imagComp,inMin1));
    }
  pComplex = inComplex;
  // Loop and copy
  for (idx = inMin1; idx <= inMax1; ++idx)
    {
    if (inPtrReal)
      {
      pComplex->Real = (double)(*inPtrReal);
      inPtrReal += inInc1;
      }
    else
      {
      pComplex->Real = 0.0;
      }
    if (inPtrImag)
      {
      pComplex->Imag = (double)(*inPtrImag);
      inPtrImag += inInc1;
      }
    else
      {
      pComplex->Imag = 0.0;
      }
    ++pComplex;
    }

  // Call the method that performs the fft
  self->ExecuteFft(inComplex, outComplex, inSize1);
  
  // Get information to loop through output region.
  outRegion->GetIncrements2d(outInc0, outInc1);
  outRegion->GetBounds2d(outMin0, outMax0, outMin1, outMax1);
  
  // Copy the complex numbers into the output
  pComplex = outComplex + (outMin1 - inMin1);
  outPtrReal = outPtr;
  outPtrImag = outPtr + outInc0;
  for (idx = outMin1; idx <= outMax1; ++idx)
    {
    (*outPtrReal) = (float)(pComplex->Real);
    (*outPtrImag) = (float)(pComplex->Imag);
    outPtrReal += outInc1;
    outPtrImag += outInc1;
    ++pComplex;
    }

  delete inComplex;
  delete outComplex;
}




//----------------------------------------------------------------------------
// Description:
// This method is passed input and output regions, and executes the fft
// algorithm to fill the output from the input.
void vtkImage1dFftFilter::Execute2d(vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  inPtr = inRegion->GetVoidPointer1d();
  outPtr = outRegion->GetVoidPointer1d();

  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that the output be floats.
  if (outRegion->GetDataType() != VTK_IMAGE_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Output must be be type float.");
    return;
    }

  // choose which templated function to call.
  switch (inRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImage1dFftFilterExecute2d(this, inRegion, (float *)(inPtr), 
				   outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImage1dFftFilterExecute2d(this, inRegion, (int *)(inPtr),
				   outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage1dFftFilterExecute2d(this, inRegion, (short *)(inPtr),
				   outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage1dFftFilterExecute2d(this, inRegion, (unsigned short *)(inPtr), 
				   outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage1dFftFilterExecute2d(this, inRegion, (unsigned char *)(inPtr),
				   outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown DataType");
      return;
    }
}



















