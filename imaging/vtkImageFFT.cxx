/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFFT.cxx
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
#include "vtkImageFFT.h"


//----------------------------------------------------------------------------
// This extent of the components changes to real and imaginary values.
void vtkImageFFT::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(2);
  this->Output->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageFFT::ComputeRequiredInputUpdateExtent(int inExt[6], 
						   int outExt[6])
{
  int *extent;
  
  // Assumes that the input update extent has been initialized to output ...
  extent = this->Input->GetWholeExtent();
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->Iteration*2] = extent[this->Iteration*2];
  inExt[this->Iteration*2 + 1] = extent[this->Iteration*2 + 1];
}

//----------------------------------------------------------------------------
// This templated execute method handles any type input, but the output
// is always floats.
template <class T>
static void vtkImageFFTExecute(vtkImageFFT *self,
			 vtkImageData *inData, int inExt[6], T *inPtr,
			 vtkImageData *outData, int outExt[6], float *outPtr,
			 int id)
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
  unsigned long count = 0;
  unsigned long target;
  float startProgress;

  startProgress = self->GetIteration()/(float)(self->GetNumberOfIterations());

  // Reorder axes (The outs here are just placeholdes
  self->PermuteExtent(inExt, inMin0, inMax0, outMin1,outMax1,outMin2,outMax2);
  self->PermuteExtent(outExt, outMin0,outMax0,outMin1,outMax1,outMin2,outMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);
  
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

  target = (unsigned long)((outMax2-outMin2+1)*(outMax1-outMin1+1)
			   * self->GetNumberOfIterations() / 50.0);
  target++;

  // loop over other axes
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = outMin1; !self->AbortExecute && idx1 <= outMax1; ++idx1)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target) + startProgress);
	  }
	count++;
	}
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
// This method is passed input and output Datas, and executes the fft
// algorithm to fill the output from the input.
// Not threaded yet.
void vtkImageFFT::ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
				  int outExt[6], int threadId)
{
  void *inPtr, *outPtr;
  int inExt[6];

  this->ComputeRequiredInputUpdateExtent(inExt, outExt);  
  inPtr = inData->GetScalarPointerForExtent(inExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
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
      vtkImageFFTExecute(this, inData, inExt, (float *)(inPtr), 
			 outData, outExt, (float *)(outPtr), threadId);
      break;
    case VTK_INT:
      vtkImageFFTExecute(this, inData, inExt, (int *)(inPtr),
			 outData, outExt, (float *)(outPtr), threadId);
      break;
    case VTK_SHORT:
      vtkImageFFTExecute(this, inData, inExt, (short *)(inPtr),
			 outData, outExt, (float *)(outPtr), threadId);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageFFTExecute(this, inData, inExt, (unsigned short *)(inPtr), 
			 outData, outExt, (float *)(outPtr), threadId);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageFFTExecute(this, inData, inExt, (unsigned char *)(inPtr),
			 outData, outExt, (float *)(outPtr), threadId);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".  
// If 1 is returned, the extent cannot be split.
int vtkImageFFT::SplitExtent(int splitExt[6], int startExt[6], 
			     int num, int total)
{
  int splitAxis;
  int min, max;

  vtkDebugMacro("SplitExtent: ( " << startExt[0] << ", " << startExt[1] << ", "
		<< startExt[2] << ", " << startExt[3] << ", "
		<< startExt[4] << ", " << startExt[5] << "), " 
		<< num << " of " << total);

  // start with same extent
  memcpy(splitExt, startExt, 6 * sizeof(int));

  splitAxis = 2;
  min = startExt[4];
  max = startExt[5];
  while ((splitAxis == this->Iteration) || (min == max))
    {
    splitAxis--;
    if (splitAxis < 0)
      { // cannot split
      vtkDebugMacro("  Cannot Split");
      return 1;
      }
    min = startExt[splitAxis*2];
    max = startExt[splitAxis*2+1];
    }

  // determine the actual number of pieces that will be generated
  if ((max - min + 1) < total)
    {
    total = max - min + 1;
    }
  
  if (num >= total)
    {
    vtkDebugMacro("  SplitRequest (" << num 
		  << ") larger than total: " << total);
    return total;
    }
  
  // determine the extent of the piece
  splitExt[splitAxis*2] = min + (max - min + 1)*num/total;
  if (num == total - 1)
    {
    splitExt[splitAxis*2+1] = max;
    }
  else
    {
    splitExt[splitAxis*2+1] = (min-1) + (max - min + 1)*(num+1)/total;
    }
  
  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
		<< splitExt[2] << ", " << splitExt[3] << ", "
		<< splitExt[4] << ", " << splitExt[5] << ")");
  fflush(stderr);

  return total;
}

  
















