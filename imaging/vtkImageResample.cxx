/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.cxx
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
#include "vtkImageResample.h"



//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageResample::vtkImageResample()
{
  this->MagnificationFactors[0] = 1.0;
  this->MagnificationFactors[1] = 1.0;
  this->MagnificationFactors[2] = 1.0;
  this->OutputSpacing[0] = 0.0; // not specified
  this->OutputSpacing[1] = 0.0; // not specified
  this->OutputSpacing[2] = 0.0; // not specified
}


//----------------------------------------------------------------------------
void vtkImageResample::SetAxisOutputSpacing(int axis, float spacing)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return;
    }
  
  if (this->OutputSpacing[axis] != spacing)
    {
    this->OutputSpacing[axis] = spacing;
    this->Modified();
    if (spacing != 0.0)
      {
      // Delay computing the magnification factor.
      // Input might not be set yet.
      this->MagnificationFactors[axis] = 0.0; // Not computed yet.
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageResample::SetAxisMagnificationFactor(int axis, float factor)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return;
    }
  
  if (this->MagnificationFactors[axis] == factor)
    {
    return;
    }
  this->Modified();
  this->MagnificationFactors[axis] = factor;
  // Spacing is no longer valid.
  this->OutputSpacing[axis] = 0.0; // Not computed yet.
}

//----------------------------------------------------------------------------
float vtkImageResample::GetAxisMagnificationFactor(int axis)
{
  if (axis < 0 || axis > 2)
    {
    vtkErrorMacro("Bad axis: " << axis);
    return 0.0;
    }
  
  if (this->MagnificationFactors[axis] == 0.0)
    {
    float *inputSpacing;
    if ( ! this->Input)
      {
      vtkErrorMacro("GetMagnificationFactor: Input not set.");
      return 0.0;
      }
    this->Input->UpdateImageInformation();
    inputSpacing = this->Input->GetSpacing();
    this->MagnificationFactors[axis] = 
      inputSpacing[axis] / this->OutputSpacing[axis];
    
    }

  vtkDebugMacro("Returning magnification factor " 
		<<  this->MagnificationFactors[axis] << " for axis "
		<< axis);
  
  return this->MagnificationFactors[axis];
}




//----------------------------------------------------------------------------
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImageResample::ComputeRequiredInputUpdateExtent(int inExt[6], 
							int outExt[6])
{
  int min, max, axis;
  float factor;
 
  axis = this->Iteration;
  factor = this->GetAxisMagnificationFactor(axis);

  vtkDebugMacro("ComputeRequiredInputUpdateExtent (axis " << axis 
		<< ") factor " << factor);
  
  memcpy(inExt, outExt, 6 * sizeof(int));
  
  min = outExt[axis*2];
  max = outExt[axis*2+1];

  min = (int)(floor((float)(min) / factor));
  max = (int)(ceil((float)(max) / factor));

  inExt[axis*2] = min;
  inExt[axis*2+1] = max;
}


//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
void vtkImageResample::ExecuteImageInformation() 
{
  int wholeMin, wholeMax, axis, ext[6];
  float spacing[3], factor;

  axis = this->Iteration;
  this->Input->GetWholeExtent(ext);
  wholeMin = ext[axis*2];
  wholeMax = ext[axis*2+1];
  
  this->Input->GetSpacing(spacing);

  // Scale the output extent
  factor = this->GetAxisMagnificationFactor(axis);
  wholeMin = (int)(ceil((float)(wholeMin) * factor));
  wholeMax = (int)(floor((float)(wholeMax) * factor));

  // Change the data spacing
  spacing[axis] /= factor;

  ext[axis*2] = wholeMin;
  ext[axis*2+1] = wholeMax;
  this->Output->SetWholeExtent(ext);
  this->Output->SetSpacing(spacing);
  
  // just in case  the input spacing has changed.
  if (this->OutputSpacing[axis] != 0.0)
    {
    // Cause MagnificationFactor to recompute.
    this->MagnificationFactors[axis] = 0.0;
    }
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is .
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
static void vtkImageResampleExecute(vtkImageResample *self,
			    vtkImageData *inData, T *inPtr, int inExt[6],
			    vtkImageData *outData, T *outPtr, int outExt[6],
			    int id)
{
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int inMin0, inMax0, inMin1, inMax1, inMin2, inMax2;
  int outIdx0, outIdx1, outIdx2, inIdx0, temp; 
  int inInc0, inInc1, inInc2, outInc0, outInc1, outInc2;
  T *inPtr1, *inPtr2, *outPtr1, *outPtr2, *inPtrC, *outPtrC;
  float magFactor, factor;
  int idxC, numC;
  unsigned long count = 0;
  unsigned long target;
  float startProgress;

  startProgress = self->GetIteration()/(float)(self->GetNumberOfIterations());

  temp = 0;

  numC = inData->GetNumberOfScalarComponents();
  
  // permute to make the filtered axis come first.
  self->PermuteExtent(inExt, inMin0, inMax0, 
		      inMin1, inMax1, inMin2, inMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteExtent(outExt, outMin0, outMax0, 
		      outMin1, outMax1, outMin2, outMax2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);

  // interpolation stuff
  magFactor = self->GetAxisMagnificationFactor(self->GetIteration());
  
  target = (unsigned long)((outMax0-outMin0+1)*(numC)
			   * self->GetNumberOfIterations() / 50.0);
  target++;

  // Loop through filteredAxisFirst
  inIdx0 = inMin0;
  for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
    {
    // compute the left input pixel for this sample
    temp = (int)(floor((float)(outIdx0) / magFactor));
    if (temp != inIdx0)
      {
      inPtr += inInc0 * (temp - inIdx0);
      inIdx0 = temp;
      }
    // compute the factor for this interpolation
    factor = ((float)(outIdx0) / magFactor) - (float)(inIdx0);
    
    // loop through the other axes
    outPtrC = outPtr;
    inPtrC = inPtr;
    for (idxC = 0; !self->AbortExecute && idxC < numC; ++idxC)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target) + startProgress);
	  }
	count++;
	}
      inPtr1 = inPtrC;
      outPtr1 = outPtrC;

      for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
	{
	outPtr2 = outPtr1;
	inPtr2 = inPtr1;
	for (outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
	  {
	  // compute the interpolation
	  if (factor < 0.001)
	    { // special case for one slice
	    *outPtr2 = *inPtr2;
	    }
	  else
	    {
	    *outPtr2 = (T)((float)(*inPtr2) 
			   + (factor * (float)(inPtr2[inInc0] - *inPtr2)));
	    }
	  
	  // increment
	  inPtr2 += inInc2;
	  outPtr2 += outInc2;
	  }
	inPtr1 += inInc1;
	outPtr1 += outInc1;
	}
      // increment
      ++inPtrC;
      ++outPtrC;
      }
    // increment (input is handled at front of loop)
    outPtr += outInc0;
    }
  
}

    
//----------------------------------------------------------------------------
// This method uses the input data to fill the output data.
// It can handle any type data, but the two datas must have the same 
// scalar type.
void vtkImageResample::ThreadedExecute(vtkImageData *inData, 
				       vtkImageData *outData, 
				       int outExt[6], int id)
{
  void *inPtr, *outPtr;
  int inExt[6];

  outPtr = outData->GetScalarPointerForExtent(outExt);
  this->ComputeRequiredInputUpdateExtent(inExt,outExt);
  inPtr = inData->GetScalarPointerForExtent(inExt);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
         << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageResampleExecute(this, 
			      inData, (float *)(inPtr), inExt,
			      outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_INT:
      vtkImageResampleExecute(this, 
			  inData, (int *)(inPtr), inExt,
			  outData, (int *)(outPtr), outExt, id);
      break;
    case VTK_SHORT:
      vtkImageResampleExecute(this, 
			  inData, (short *)(inPtr),inExt,
			  outData, (short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageResampleExecute(this, 
			  inData, (unsigned short *)(inPtr), inExt, 
			  outData, (unsigned short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageResampleExecute(this, 
			  inData, (unsigned char *)(inPtr), inExt, 
			  outData, (unsigned char *)(outPtr), outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















