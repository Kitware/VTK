/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFFT1D.cxx
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
#include "vtkImageFFT1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageFFT1D fitler.
vtkImageFFT1D::vtkImageFFT1D()
{
  // mimic a call to SetFilteredAxis.
  this->FilteredAxis = VTK_IMAGE_X_AXIS;
  this->SetExecutionAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_COMPONENT_AXIS);
  
  // Output is always floats.
  this->SetOutputScalarType(VTK_FLOAT);
}


//----------------------------------------------------------------------------
void vtkImageFFT1D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFourierFilter::PrintSelf(os,indent);

  os << indent << "FilteredAxis: " << vtkImageAxisNameMacro(this->FilteredAxis)
     << "\n";
}

//----------------------------------------------------------------------------
// Description:
// Which axis will be operated on.
void vtkImageFFT1D::SetFilteredAxis(int axis)
{  
  if (this->FilteredAxis == axis)
    {
    return;
    }
  
  if (axis < 0 || axis > 3)
    {
    vtkErrorMacro("SetFilteredAxis: Bad axis: " << axis);
    return;
    }
  
  // Tell the supper class which axes to loop over
  this->SetExecutionAxes(axis, VTK_IMAGE_COMPONENT_AXIS);
  
  this->FilteredAxis = axis;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This extent of the components changes to real and imaginary values.
void vtkImageFFT1D::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(2);
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageFFT1D::ComputeRequiredInputUpdateExtent()
{
  int min, max;
  
  this->Input->GetAxisWholeExtent(this->FilteredAxis, min, max);
  this->Input->SetAxisUpdateExtent(this->FilteredAxis, min, max);
}

//----------------------------------------------------------------------------
// Description:
// This templated execute method handles any type input, but the output
// is always floats.
template <class T>
static void vtkImageFFT1DExecute(vtkImageFFT1D *self,
				 vtkImageRegion *inRegion, T *inPtr,
				 vtkImageRegion *outRegion, float *outPtr)
{
  vtkImageComplex *inComplex;
  vtkImageComplex *outComplex;
  vtkImageComplex *pComplex;
  T *inPtrReal, *inPtrImag = NULL;
  int inMin0, inMax0, inMinC, inMaxC, inSize0;
  int inInc0, inIncC;
  float *outPtrReal, *outPtrImag;
  int outMin0, outMax0, outMinC, outMaxC;
  int outInc0, outIncC;
  int idx, axis;
  
  // Get information to march through data 
  axis = self->GetFilteredAxis();
  inRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, inIncC);
  inRegion->GetAxisIncrements(axis, inInc0);
  inRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, inMinC, inMaxC);
  inRegion->GetAxisExtent(axis, inMin0, inMax0);
  inSize0 = inMax0 - inMin0 + 1;
  
  // We have to have real components at least.
  if (inMinC > 0 || inMaxC < 0)
    {
    vtkGenericWarningMacro("No real components");
    return;
    }

  // Allocate the arrays of complex numbers
  inComplex = new vtkImageComplex[inSize0];
  outComplex = new vtkImageComplex[inSize0];
  
  // Convert the input to complex numbers.
  // The complexity is because the imput may be real or complex.
  inPtrReal = inPtr;
  if (inMaxC >= 1)
    {
    inPtrImag = inPtrReal + inIncC;
    }
  pComplex = inComplex;
  // Loop and copy
  for (idx = inMin0; idx <= inMax0; ++idx)
    {
    pComplex->Real = (double)(*inPtrReal);
    inPtrReal += inInc0;
    if (inPtrImag)
      {
      pComplex->Imag = (double)(*inPtrImag);
      inPtrImag += inInc0;
      }
    else
      {
      pComplex->Imag = 0.0;
      }
    ++pComplex;
    }

  // Call the method that performs the fft
  self->ExecuteFft(inComplex, outComplex, inSize0);
  
  // Get information to loop through output region.
  outRegion->GetAxisIncrements(axis, outInc0);
  outRegion->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, outIncC);
  outRegion->GetAxisExtent(axis, outMin0, outMax0);
  outRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, outMinC, outMaxC);
  
  // Copy the complex numbers into the output
  pComplex = outComplex + (outMin0 - inMin0); // may request only a piece
  outPtrReal = outPtr;
  outPtrImag = outPtr + outIncC;
  for (idx = outMin0; idx <= outMax0; ++idx)
    {
    (*outPtrReal) = (float)(pComplex->Real);
    (*outPtrImag) = (float)(pComplex->Imag);
    outPtrReal += outInc0;
    outPtrImag += outInc0;
    ++pComplex;
    }

  delete inComplex;
  delete outComplex;
}




//----------------------------------------------------------------------------
// Description:
// This method is passed input and output regions, and executes the fft
// algorithm to fill the output from the input.
void vtkImageFFT1D::Execute(vtkImageRegion *inRegion, 
			    vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;

  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarPointer();

  // this filter expects that the output be floats.
  if (outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Output must be be type float.");
    return;
    }

  // choose which templated function to call.
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageFFT1DExecute(this, inRegion, (float *)(inPtr), 
			   outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageFFT1DExecute(this, inRegion, (int *)(inPtr),
			   outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageFFT1DExecute(this, inRegion, (short *)(inPtr),
			   outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageFFT1DExecute(this, inRegion, (unsigned short *)(inPtr), 
			   outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageFFT1DExecute(this, inRegion, (unsigned char *)(inPtr),
			   outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















