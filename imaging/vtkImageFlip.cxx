/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.cxx
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
#include "vtkImageFlip.h"



//----------------------------------------------------------------------------
vtkImageFlip::vtkImageFlip()
{
  this->PreserveImageExtent = 1;
  this->FilteredAxis = 0;
}

//----------------------------------------------------------------------------
// Description:
// Image extent is modified by this filter.
void vtkImageFlip::ExecuteImageInformation()
{
  int extent[6];
  int axis, temp;

  if ( ! this->PreserveImageExtent)
    {
    this->Input->GetWholeExtent(extent);
    axis = this->FilteredAxis;
    temp = extent[axis*2+1];
    extent[axis*2+1] = -temp;
    this->Output->SetWholeExtent(extent);
    }
}

//----------------------------------------------------------------------------
// Description:
// What input should be requested.
void vtkImageFlip::ComputeRequiredInputUpdateExtent(int inExt[6], 
						    int outExt[6])
{
  int axis, sum;
  int *wholeExtent;
  
  // copy out to in
  memcpy((void *)inExt, (void *)outExt, 6 * sizeof(int));
  
  wholeExtent = this->Output->GetWholeExtent();
  axis = this->FilteredAxis;
  if (this->PreserveImageExtent)
    {
    sum = wholeExtent[axis*2] + wholeExtent[axis*2+1];
    inExt[axis*2] = -outExt[axis*2+1]+sum;
    inExt[axis*2+1] = -outExt[axis*2]+sum;
    }
  else
    {
    inExt[axis*2] = -outExt[axis*2+1];
    inExt[axis*2+1] = -outExt[axis*2];
    }
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageFlipExecute(vtkImageFlip *self, int id,
				vtkImageData *inData, int *inExt,
				vtkImageData *outData, int *outExt, T *outPtr)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outSkipY, outSkipZ;
  T *inPtrX, *inPtrY, *inPtrZ;
  int scalarSize;
  unsigned long count = 0;
  unsigned long target;

  // find the region to loop over
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  // target is for progress ...
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  // outIncX is a place holder
  outData->GetContinuousIncrements(outExt, outIncX, outSkipY, outSkipZ);
  // for x, increment is used because all components are copied at once
  outIncX = inData->GetNumberOfScalarComponents();
  // for memcpy
  scalarSize = sizeof(T)*outIncX;
  
  // Get the starting in pointer
  inPtrZ = (T *)inData->GetScalarPointerForExtent(inExt);
  // adjust the increments (and pointer) for the flip
  switch (self->GetFilteredAxis())
    {
    case 0:
      inPtrZ += maxX * inIncX;
      inIncX = -inIncX;
      break;
    case 1:
      inPtrZ += maxY * inIncY;
      inIncY = -inIncY;
      break;
    case 2:
      inPtrZ += maxZ * inIncZ;
      inIncZ = -inIncZ;
      break;
    default:
      vtkGenericWarningMacro("Bad axis " << self->GetFilteredAxis());
      return;
    }
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtrY = inPtrZ;
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      // handle updating progress method
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      inPtrX = inPtrY;
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// Pixel operation
	memcpy((void *)outPtr, (void *)inPtrX, scalarSize);
	outPtr += outIncX;
	inPtrX += inIncX;
	}
      outPtr += outSkipY;
      inPtrY += inIncY;
      }
    outPtr += outSkipZ;
    inPtrZ += inIncZ;
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageFlip::ThreadedExecute(vtkImageData *inData, 
				   vtkImageData *outData,
				   int outExt[6], int id) 
{
  int inExt[6];
  void *outPtr;
  
  outPtr = outData->GetScalarPointerForExtent(outExt);
  this->ComputeRequiredInputUpdateExtent(inExt, outExt);

  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
               << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (outData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageFlipExecute(this, id, inData, inExt, 
			  outData, outExt, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageFlipExecute(this, id, inData, inExt, 
			  outData, outExt, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageFlipExecute(this, id, inData, inExt, 
			  outData, outExt, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageFlipExecute(this, id, inData, inExt, 
			  outData, outExt, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageFlipExecute(this, id, inData, inExt, 
			  outData, outExt, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}
















