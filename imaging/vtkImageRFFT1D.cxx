/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRFFT1D.cxx
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
#include "vtkImageRFFT1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageRFFT1D fitler.
vtkImageRFFT1D::vtkImageRFFT1D()
{
  this->SetAxes(VTK_IMAGE_X_AXIS);
  // Output is whatever type you want, but defaults to float.
  this->SetOutputScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// Description:
// This 1d filter is actually a 2d filter with the component axis as the first
// axis (axis0).
void vtkImageRFFT1D::SetAxes(int axis)
{
  this->vtkImageFourierFilter::SetAxes(VTK_IMAGE_COMPONENT_AXIS, axis);
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// Easier than changing logic of execute function to resemble Fft.
void vtkImageRFFT1D::InterceptCacheUpdate(vtkImageRegion *region)
{
  int min, max;
  
  region->GetExtent(min, max);
  if (min < 0 || max > 1)
    {
    vtkErrorMacro(<< "Only two channels to request 0 and 1");
    }
  
  region->SetExtent(1, 0);
}


//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageRFFT1D::ComputeRequiredInputRegionExtent(
		   vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int extent[4];
  
  outRegion = outRegion;
  inRegion->GetImageExtent(2, extent);
  // make sure input has two component
  if (extent[0] != 0 || extent[1] != 1)
    {
    vtkErrorMacro(<< "ComputeRequiredInputRegionExtent: "
                  << "Input has wrong number of component");
    return;
    }
  
  inRegion->SetExtent(2, extent);
}

//----------------------------------------------------------------------------
// Description:
// This templated execute method handles any type input, but the output
// is always floats. Axis 0 should be components. FFT is performed on axis 1.
template <class T>
void vtkImageRFFT1DExecute(vtkImageRFFT1D *self,
				   vtkImageRegion *inRegion, float *inPtr,
				   vtkImageRegion *outRegion, T *outPtr)
{
  vtkImageComplex *inComplex;
  vtkImageComplex *outComplex;
  vtkImageComplex *pComplex;
  float *inPtrReal, *inPtrImag;
  int inMin0, inMax0, inMin1, inMax1, inSize1;
  int inInc0, inInc1;
  T *outPtrReal, *outPtrImag;
  int outMin0, outMax0, outMin1, outMax1;
  int outInc0, outInc1;
  int idx;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  inRegion->GetExtent(inMin0, inMax0, inMin1, inMax1);
  inSize1 = inMax1 - inMin1 + 1;
  
  // Input should have two component
  if (inMin0 != 0 || inMax0 != 1)
    {
    cerr << "Input has wrong components.";
    return;
    }

  // Allocate the arrays of complex numbers
  inComplex = new vtkImageComplex[inSize1];
  outComplex = new vtkImageComplex[inSize1];
  
  // Convert the input to complex format.
  pComplex = inComplex;
  inPtrReal = inPtr;
  inPtrImag = inPtr + inInc0;
  for (idx = inMin1; idx <= inMax1; ++idx)
    {
    pComplex->Real = (double)(*inPtrReal);
    pComplex->Imag = (double)(*inPtrImag);
    inPtrReal += inInc1;
    inPtrImag += inInc1;
    ++pComplex;
    }

  // Call the method that performs the fft
  self->ExecuteRfft(inComplex, outComplex, inSize1);
  
  // Get information to loop through output region.
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  
  // Copy the complex numbers into the output
  pComplex = outComplex + (outMin1 - inMin1);
  outPtrReal = outPtr;
  outPtrImag = outPtr + outInc0;
  for (idx = outMin1; idx <= outMax1; ++idx)
    {
    (*outPtrReal) = (T)(pComplex->Real);
    (*outPtrImag) = (T)(pComplex->Imag);
    // Ignore imaginary part
    outPtrReal += outInc1;
    outPtrImag += outInc1;
    ++pComplex;
    }
}




//----------------------------------------------------------------------------
// Description:
// This method is passed input and output regions, and executes the fft
// algorithm to fill the output from the input.
void vtkImageRFFT1D::Execute(vtkImageRegion *inRegion, 
				     vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarPointer();

  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects the input to be floats.
  if (inRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Input must be be type float.");
    return;
    }

  // choose which templated function to call.
  switch (outRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageRFFT1DExecute(this, inRegion, (float *)(inPtr), 
				   outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageRFFT1DExecute(this, inRegion, (float *)(inPtr),
				   outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageRFFT1DExecute(this, inRegion, (float *)(inPtr),
				   outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageRFFT1DExecute(this, inRegion, (float *)(inPtr), 
				   outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageRFFT1DExecute(this, inRegion, (float *)(inPtr),
				   outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















