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
#include "vtkImageCache.h"
#include "vtkImageFFT1D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageFFT1D fitler.
vtkImageFFT1D::vtkImageFFT1D()
{
  // mimic a call to SetFilteredAxis.
  this->FilteredAxis = 0;
}


//----------------------------------------------------------------------------
void vtkImageFFT1D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFourierFilter::PrintSelf(os,indent);

  os << indent << "FilteredAxis: " << this->FilteredAxis << "\n";
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
  
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("SetFilteredAxis: Bad axis: " << axis);
    return;
    }
  
  this->FilteredAxis = axis;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This extent of the components changes to real and imaginary values.
void vtkImageFFT1D::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(2);
  this->Output->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageFFT1D::ComputeRequiredInputUpdateExtent(int inExt[6], 
						     int outExt[6])
{
  int *extent;
  
  // Assumes that the input update extent has been initialized to output ...
  extent = this->Input->GetWholeExtent();
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->FilteredAxis*2] = extent[this->FilteredAxis*2];
  inExt[this->FilteredAxis*2 + 1] = extent[this->FilteredAxis*2 + 1];
}

//----------------------------------------------------------------------------
// Description:
// This templated execute method handles any type input, but the output
// is always floats.
template <class T>
static void vtkImageFFT1DExecute(vtkImageFFT1D *self,
			 vtkImageData *inData, int inExt[6], T *inPtr,
			 vtkImageData *outData, int outExt[6], float *outPtr)
{
  vtkImageComplex *inComplex;
  vtkImageComplex *outComplex;
  vtkImageComplex *pComplex;
  //
  int inMin0, inMax0;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  //
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outInc0, outInc1, outInc2;
  float *outPtr0, *outPtr1, *outPtr2;
  //
  int idx0, idx1, idx2, inSize0, numberOfComponents;
  
  // Reorder axes (brute force)
  outMin2 = 0;
  switch (self->GetFilteredAxis())
    {
    case 0:
      inMin0 = inExt[0];      inMax0 = inExt[1];
      inData->GetIncrements(inInc0, inInc1, inInc2);
      outData->GetIncrements(outInc0, outInc1, outInc2);
      outMin0 = outExt[0];    outMax0 = outExt[1];
      outMin1 = outExt[2];    outMax1 = outExt[3];
      outMin2 = outExt[4];    outMax2 = outExt[5];
      break;
    case 1:
      inMin0 = inExt[2];      inMax0 = inExt[3];
      inData->GetIncrements(inInc1, inInc0, inInc2);
      outData->GetIncrements(outInc1, outInc0, outInc2);
      outMin0 = outExt[2];    outMax0 = outExt[3];
      outMin1 = outExt[0];    outMax1 = outExt[1];
      outMin2 = outExt[4];    outMax2 = outExt[5];
      break;
    case 2:
      inMin0 = inExt[4];      inMax0 = inExt[5];
      inData->GetIncrements(inInc2, inInc0, inInc1);
      outData->GetIncrements(outInc2, outInc0, outInc1);
      outMin0 = outExt[4];    outMax0 = outExt[5];
      outMin1 = outExt[0];    outMax1 = outExt[1];
      outMin2 = outExt[2];    outMax2 = outExt[3];
      break;
    default:
      vtkGenericWarningMacro("bad axis ");
      return;
    }
  
  inSize0 = inMax0 - inMin0 + 1;
  
  // Input has to have real components at least.
  numberOfComponents = inData->GetNumberOfScalarComponents();
  if (numberOfComponents < 1)
    {
    vtkGenericWarningMacro("No real components");
    return;
    }

  // Allocate the arrays of complex numbers
  inComplex = new vtkImageComplex[inSize0];
  outComplex = new vtkImageComplex[inSize0];
  
  // loop over other axes
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
      {
      // copy into complex numbers
      inPtr0 = inPtr1;
      pComplex = inComplex;
      for (idx0 = inMin0; idx0 <= inMax0; ++idx0)
	{
	pComplex->Real = (double)(*inPtr0);
	pComplex->Imag = 0.0;
	if (numberOfComponents > 1)
	  { // yes we have an imaginary input
	  pComplex->Imag = (double)(inPtr0[1]);;
	  }
	inPtr0 += inInc0;
	++pComplex;
	}
      
      // Call the method that performs the fft
      self->ExecuteFft(inComplex, outComplex, inSize0);

      // copy into output
      outPtr0 = outPtr1;
      pComplex = outComplex + (inMin0 - outMin0);
      for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
	{
	*outPtr0 = (float)pComplex->Real;
	outPtr0[1] = (float)pComplex->Imag;
	outPtr0 += outInc0;
	++pComplex;
	}
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
    
  delete inComplex;
  delete outComplex;
}




//----------------------------------------------------------------------------
// Description:
// This method is passed input and output Datas, and executes the fft
// algorithm to fill the output from the input.
// Not threaded yet.
void vtkImageFFT1D::Execute(vtkImageData *inData, 
			    vtkImageData *outData)
{
  void *inPtr, *outPtr;
  int inExt[6], *outExt;

  inPtr = inData->GetScalarPointer();
  outPtr = outData->GetScalarPointer();
  outExt = this->GetOutput()->GetUpdateExtent();
  this->ComputeRequiredInputUpdateExtent(inExt, outExt);  
  
  // this filter expects that the output be floats.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: Output must be be type float.");
    return;
    }

  // this filter expects input to have 1 or two components
  if (outData->GetNumberOfScalarComponents() != 1 && 
      outData->GetNumberOfScalarComponents() != 2)
    {
    vtkErrorMacro(<< "Execute: Cannot handle more than 2 components");
    return;
    }

  // choose which templated function to call.
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageFFT1DExecute(this, inData, inExt, (float *)(inPtr), 
			   outData, outExt, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageFFT1DExecute(this, inData, inExt, (int *)(inPtr),
			   outData, outExt, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageFFT1DExecute(this, inData, inExt, (short *)(inPtr),
			   outData, outExt, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageFFT1DExecute(this, inData, inExt, (unsigned short *)(inPtr), 
			   outData, outExt, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageFFT1DExecute(this, inData, inExt, (unsigned char *)(inPtr),
			   outData, outExt, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















