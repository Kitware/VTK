/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExtractComponents.cxx
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
#include "vtkImageExtractComponents.h"


//----------------------------------------------------------------------------
vtkImageExtractComponents::vtkImageExtractComponents()
{
  this->Components[0] = 0;
  this->Components[1] = 1;
  this->Components[2] = 2;
  this->Components[3] = 3;
  this->NumberOfComponents = 1;
}

//----------------------------------------------------------------------------
void vtkImageExtractComponents::SetComponents(int c1, int c2, int c3)
{
  int modified = 0;
  
  if (this->Components[0] != c1)
    {
    this->Components[0] = c1;
    modified = 1;
    }
  if (this->Components[1] != c2)
    {
    this->Components[1] = c2;
    modified = 1;
    }
  if (this->Components[2] != c3)
    {
    this->Components[2] = c3;
    modified = 1;
    }
  
  if (modified || this->NumberOfComponents != 3)
    {
    this->NumberOfComponents = 3;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageExtractComponents::SetComponents(int c1, int c2)
{
  int modified = 0;
  
  if (this->Components[0] != c1)
    {
    this->Components[0] = c1;
    modified = 1;
    }
  if (this->Components[1] != c2)
    {
    this->Components[1] = c2;
    modified = 1;
    }
  
  if (modified || this->NumberOfComponents != 2)
    {
    this->NumberOfComponents = 2;
    this->Modified();
    }
}
							
//----------------------------------------------------------------------------
void vtkImageExtractComponents::SetComponents(int c1)
{
  int modified = 0;
  
  if (this->Components[0] != c1)
    {
    this->Components[0] = c1;
    modified = 1;
    }
  
  if (modified || this->NumberOfComponents != 1)
    {
    this->NumberOfComponents = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that only one component will remain.
void vtkImageExtractComponents::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(this->NumberOfComponents);
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageExtractComponentsExecute(vtkImageExtractComponents *self,
					     vtkImageData *inData, T *inPtr,
					     vtkImageData *outData, T *outPtr,
					     int outExt[6])
{
  int idxR, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int cnt, inCnt;
  int offset1, offset2, offset3;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  cnt = outData->GetNumberOfScalarComponents();
  inCnt = inData->GetNumberOfScalarComponents();
  
  // Loop through ouput pixels
  offset1 = self->Components[0];
  offset2 = self->Components[1];
  offset3 = self->Components[2];
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      // handle inner loop based on number of components extracted
      switch (cnt)
	{
	case 1:
	  for (idxR = 0; idxR <= maxX; idxR++)
	    {
	    // Pixel operation
	    *outPtr = *(inPtr + offset1);
	    outPtr++;
	    inPtr += inCnt;
	    }
	  break;
	case 2:
	  for (idxR = 0; idxR <= maxX; idxR++)
	    {
	    // Pixel operation
	    *outPtr = *(inPtr + offset1);
	    outPtr++;
	    *outPtr = *(inPtr + offset2);
	    outPtr++;
	    inPtr += inCnt;
	    }
	  break;
	case 3:
	  for (idxR = 0; idxR <= maxX; idxR++)
	    {
	    // Pixel operation
	    *outPtr = *(inPtr + offset1);
	    outPtr++;
	    *outPtr = *(inPtr + offset2);
	    outPtr++;
	    *outPtr = *(inPtr + offset3);
	    outPtr++;
	    inPtr += inCnt;
	    }
	  break;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed input and output datas, and executes the
// ExtractComponents function on each line.  
void vtkImageExtractComponents::ThreadedExecute(vtkImageData *inData, 
						vtkImageData *outData,
						int outExt[6])
{
  int max, idx;
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
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
  
  // make sure we can get all of the components.
  max = inData->GetNumberOfScalarComponents();
  for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
    if (this->Components[idx] > max)
      {
      vtkErrorMacro("Execute: Component " << this->Components[idx]
		    << " is not in input.");
      return;
      }
    }
  
  // choose which templated function to call.
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageExtractComponentsExecute(this, inData, (float *)(inPtr),
				       outData, (float *)(outPtr),
				       outExt);
      break;
    case VTK_INT:
      vtkImageExtractComponentsExecute(this, inData, (int *)(inPtr),
				       outData, (int *)(outPtr),
				       outExt);
      break;
    case VTK_SHORT:
      vtkImageExtractComponentsExecute(this, inData, (short *)(inPtr),
				       outData, (short *)(outPtr),
				       outExt);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageExtractComponentsExecute(this,inData,(unsigned short *)(inPtr),
				       outData, (unsigned short *)(outPtr),
				       outExt);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageExtractComponentsExecute(this, inData, (unsigned char *)(inPtr),
				       outData, (unsigned char *)(outPtr),
				       outExt);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



















