/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMirrorPad.cxx
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
#include "vtkImageCache.h"
#include "vtkImageMirrorPad.h"



//----------------------------------------------------------------------------
// Just clip the request.
void vtkImageMirrorPad::ComputeRequiredInputUpdateExtent(int inExt[6], 
							 int outExt[6])
{
  int *wExtent = this->Input->GetWholeExtent();
  int pad[6];
  int idx;
  
  // iitialize inExt
  memcpy(inExt,outExt,6*sizeof(int));
  
  // how much padding is required ?
  // if padding is required on a boundary then we know we need the whole extent
  // for that boundary. The following code handles the cases where the outputextent
  // is all inside or outside of the whole extent
  for (idx = 0; idx < 3; idx++)
    {
    pad[idx*2] = 0;
    pad[idx*2+1] = 0;
    if (outExt[idx*2] < wExtent[idx*2])
      {
      pad[idx*2] = wExtent[idx*2] - outExt[idx*2];
      inExt[idx*2] = wExtent[idx*2];
      }
    if (outExt[idx*2+1] > wExtent[idx*2+1])
      {
      pad[idx*2+1] = outExt[idx*2+1] - wExtent[idx*2+1];
      inExt[idx*2+1] = wExtent[idx*2+1];
      }
    }
  
  // now handle mixed case where negative pad mirrored may 
  // require a larger positive extent
  for (idx = 0; idx < 3; idx++)
    {
    if (pad[idx*2] > inExt[idx*2+1])
      {
      inExt[idx*2+1] = pad[idx*2];
      if (inExt[idx*2+1] > wExtent[idx*2+1])
	{
	inExt[idx*2+1] = wExtent[idx*2+1];
	}
      }
    if (wExtent[idx*2+1] - pad[idx*2+1] < inExt[idx*2])
      {
      inExt[idx*2] = wExtent[idx*2+1] - pad[idx*2+1];
      if (inExt[idx*2] < wExtent[idx*2])
	{
	inExt[idx*2] = wExtent[idx*2];
	}
      }
    }
}




//----------------------------------------------------------------------------
// Description:
template <class T>
static void vtkImageMirrorPadExecute(vtkImageMirrorPad *self,
				     vtkImageData *inData,
				     vtkImageData *outData, T *outPtr,
				     int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inInc[3];
  int inIncStart[3];
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int *wExtent = self->GetInput()->GetWholeExtent();
  int pad[6], idx;
  int inIdxStart[3];
  int inIdx[3];
  T *inPtr, *inPtrX, *inPtrY, *inPtrZ;
  int maxC, inMaxC;
  
  // find the region to loop over
  inMaxC = inData->GetNumberOfScalarComponents();
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // how much padding is required ?
  // if padding is required on a boundary then we know we need the whole extent
  // for that boundary. The following code handles the cases where the outputextent
  // is all inside or outside of the whole extent
  for (idx = 0; idx < 3; idx++)
    {
    pad[idx*2] = 0;
    pad[idx*2+1] = 0;
    if (outExt[idx*2] < wExtent[idx*2])
      {
      pad[idx*2] = wExtent[idx*2] - outExt[idx*2];
      }
    if (outExt[idx*2+1] > wExtent[idx*2+1])
      {
      pad[idx*2+1] = outExt[idx*2+1] - wExtent[idx*2+1];
      }
    }
  
  // find the starting point
  for (idx = 0; idx < 3; idx++)
    {
    inIdxStart[idx] = outExt[idx*2];
    inIncStart[idx] = 1;
    while (inIdxStart[idx] < wExtent[idx*2])
      {
      inIncStart[idx] = -inIncStart[idx];
      inIdxStart[idx] = inIdxStart[idx] + (wExtent[idx*2+1] - wExtent[idx*2] + 1);
      }
    // if we are heading negative then we need to mirror the offset
    if (inIncStart[idx] < 0)
      {
      inIdxStart[idx] = wExtent[idx*2+1] - inIdxStart[idx] + wExtent[idx*2];
      }
    }
  inPtr = (T *)inData->GetScalarPointer(inIdxStart[0], inIdxStart[1], inIdxStart[2]);
  
  // Loop through ouput pixels
  inPtrZ = inPtr;
  inIdx[2] = inIdxStart[2];
  inInc[2] = inIncStart[2];
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtrY = inPtrZ;
    inIdx[1] = inIdxStart[1];
    inInc[1] = inIncStart[1];
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      inPtrX = inPtrY;
      inIdx[0] = inIdxStart[0];
      inInc[0] = inIncStart[0];
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}

      // if components are same much faster
      if ((maxC == inMaxC) && (maxC == 1))
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  // Pixel operation
	  *outPtr = *inPtrX;
	  outPtr++;
	  inIdx[0] += inInc[0];
	  inPtrX = inPtrX + inInc[0]*inIncX;
	  if (inIdx[0] < wExtent[0] || inIdx[0] > wExtent[1])
	    {
	    inInc[0] *= -1;
	    inIdx[0] += inInc[0];
	    inPtrX = inPtrX + inInc[0]*inIncX;
	    }
	  }
	}
      else // components are not the same
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  for (idxC = 0; idxC < maxC; idxC++)
	    {
	    // Pixel operation
	    if (idxC < inMaxC) *outPtr = *(inPtrX + idxC);
	    else *outPtr = *(inPtrX + idxC%inMaxC);
	    outPtr++;
	    }
	  inIdx[0] += inInc[0];
	  inPtrX = inPtrX + inInc[0]*inIncX;
	  if (inIdx[0] < wExtent[0] || inIdx[0] > wExtent[1])
	    {
	    inInc[0] *= -1;
	    inIdx[0] += inInc[0];
	    inPtrX = inPtrX + inInc[0]*inIncX;
	    }
	  }
	}
      
      outPtr += outIncY;
      inIdx[1] += inInc[1];
      inPtrY = inPtrY + inInc[1]*inIncY;
      if (inIdx[1] < wExtent[2] || inIdx[1] > wExtent[3])
	{
	inInc[1] *= -1;
	inIdx[1] += inInc[1];
	inPtrY = inPtrY + inInc[1]*inIncY;
	}
      }
    outPtr += outIncZ;
    inIdx[2] += inInc[2];
    inPtrZ = inPtrZ + inInc[2]*inIncZ;
    if (inIdx[2] < wExtent[4] || inIdx[2] > wExtent[5])
      {
      inInc[2] *= -1;
      inIdx[2] += inInc[2];
      inPtrZ = inPtrZ + inInc[2]*inIncZ;
      }
    }
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageMirrorPad::ThreadedExecute(vtkImageData *inData, 
					vtkImageData *outData,
					int outExt[6], int id)
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
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
      vtkImageMirrorPadExecute(this, inData, outData, 
			       (float *)(outPtr), outExt, id);
      break;
    case VTK_INT:
      vtkImageMirrorPadExecute(this, inData, outData, 
			       (int *)(outPtr), outExt, id);
      break;
    case VTK_SHORT:
      vtkImageMirrorPadExecute(this, inData, outData, 
			       (short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMirrorPadExecute(this, inData, outData, 
			       (unsigned short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMirrorPadExecute(this, inData, outData, 
			       (unsigned char *)(outPtr), outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}














