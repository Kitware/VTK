/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRfft1d.cxx
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
#include "vtkImageRfft1d.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageRfft1d fitler.
vtkImageRfft1d::vtkImageRfft1d()
{
  this->SetAxes2d(VTK_IMAGE_COMPONENT_AXIS, VTK_IMAGE_X_AXIS);
  // Output is whatever type you want, but defaults to float.
  this->SetOutputDataType(VTK_IMAGE_FLOAT);
}

//----------------------------------------------------------------------------
// Description:
// This 1d filter is actually a 2d filter with the component axis as the first
// axis (axis0).
void vtkImageRfft1d::SetAxes1d(int axis)
{
  this->SetAxes2d(VTK_IMAGE_COMPONENT_AXIS, axis);
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// Easier than changing logic of execute function to resemble Fft.
void vtkImageRfft1d::InterceptCacheUpdate(vtkImageRegion *region)
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
void vtkImageRfft1d::ComputeRequiredInputRegionBounds(
		   vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int bounds[4];
  
  outRegion = outRegion;
  inRegion->GetImageBounds2d(bounds);
  // make sure input has two component
  if (bounds[0] != 0 || bounds[1] != 1)
    {
    vtkErrorMacro(<< "ComputeRequiredInputRegionBounds: "
                  << "Input has wrong number of component");
    return;
    }
  
  inRegion->SetBounds2d(bounds);
}

//----------------------------------------------------------------------------
// Description:
// This templated execute method handles any type input, but the output
// is always floats. Axis 0 should be components. FFT is performed on axis 1.
template <class T>
void vtkImageRfft1dExecute2d(vtkImageRfft1d *self,
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
  inRegion->GetIncrements2d(inInc0, inInc1);
  inRegion->GetBounds2d(inMin0, inMax0, inMin1, inMax1);
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
  outRegion->GetIncrements2d(outInc0, outInc1);
  outRegion->GetBounds2d(outMin0, outMax0, outMin1, outMax1);
  
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
void vtkImageRfft1d::Execute2d(vtkImageRegion *inRegion, 
				     vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  inPtr = inRegion->GetVoidPointer1d();
  outPtr = outRegion->GetVoidPointer1d();

  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects the input to be floats.
  if (inRegion->GetDataType() != VTK_IMAGE_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Input must be be type float.");
    return;
    }

  // choose which templated function to call.
  switch (outRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageRfft1dExecute2d(this, inRegion, (float *)(inPtr), 
				   outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImageRfft1dExecute2d(this, inRegion, (float *)(inPtr),
				   outRegion, (int *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImageRfft1dExecute2d(this, inRegion, (float *)(inPtr),
				   outRegion, (short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageRfft1dExecute2d(this, inRegion, (float *)(inPtr), 
				   outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageRfft1dExecute2d(this, inRegion, (float *)(inPtr),
				   outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown DataType");
      return;
    }
}



















