/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNormalize.cxx
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

#include "vtkImageNormalize.h"


//----------------------------------------------------------------------------
// This method tells the superclass that the first axis will collapse.
void vtkImageNormalize::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
					   vtkImageData *outData)
{
  outData->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageNormalizeExecute(vtkImageNormalize *self,
				     vtkImageData *inData, T *inPtr,
				     vtkImageData *outData, float *outPtr,
				     int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float sum;
  T *inVect;
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      for (idxX = 0; idxX <= maxX; idxX++)
	{

	// save the start of the vector
	inVect = inPtr;

	// compute the magnitude.
	sum = 0.0;
	for (idxC = 0; idxC < maxC; idxC++)
	  {
	  sum += (float)(*inPtr) * (float)(*inPtr);
	  inPtr++;
	  }
	if (sum > 0.0)
	  {
	  sum = 1.0 / sqrt(sum);
	  }
	
	// now divide to normalize.
	for (idxC = 0; idxC < maxC; idxC++)
	  {
	  *outPtr = (float)(*inVect) * sum;
	  inVect++;
	  outPtr++;
	  }

	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageNormalize::ThreadedExecute(vtkImageData *inData, 
					vtkImageData *outData,
					int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, " << outData->GetScalarType()
    << ", must be float");
    return;
    }
  
  switch (inData->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageNormalizeExecute(this, 
			       inData, (double *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageNormalizeExecute(this, 
			       inData, (float *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_LONG:
      vtkImageNormalizeExecute(this, 
			       inData, (long *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageNormalizeExecute(this, 
			       inData, (unsigned long *)(inPtr), 
			       outData, (float *)(outPtr), 
			       outExt, id);
      break;
    case VTK_INT:
      vtkImageNormalizeExecute(this, 
			       inData, (int *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageNormalizeExecute(this, 
			       inData, (unsigned int *)(inPtr), 
			       outData, (float *)(outPtr), 
			       outExt, id);
      break;
    case VTK_SHORT:
      vtkImageNormalizeExecute(this, 
			       inData, (short *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageNormalizeExecute(this, 
			       inData, (unsigned short *)(inPtr), 
			       outData, (float *)(outPtr), 
			       outExt, id);
      break;
    case VTK_CHAR:
      vtkImageNormalizeExecute(this, 
			       inData, (char *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageNormalizeExecute(this, 
			       inData, (unsigned char *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}












