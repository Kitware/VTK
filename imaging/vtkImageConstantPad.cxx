/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConstantPad.cxx
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
#include "vtkImageConstantPad.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageConstantPad::vtkImageConstantPad()
{
  this->Constant = 0.0;
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageConstantPadExecute(vtkImageConstantPad *self,
				       vtkImageData *inData, T *inPtr,
				       vtkImageData *outData, T *outPtr,
				       int outExt[6], int inExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  T constant;
  int inMinX, inMaxX, inMaxC;
  constant = (T)(self->GetConstant());
  int state0, state1, state2, state3;
  unsigned long count = 0;
  unsigned long target;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  inMaxC = inData->GetNumberOfScalarComponents();
  inMinX = inExt[0] - outExt[0];
  inMaxX = inExt[1] - outExt[0];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = outExt[4]; idxZ <= outExt[5]; idxZ++)
    {
    state3 = (idxZ < inExt[4] || idxZ > inExt[5]);
    for (idxY = outExt[2]; idxY <= outExt[3]; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      state2 = (state3 || idxY < inExt[2] || idxY > inExt[3]);
      if ((maxC == inMaxC) && (maxC == 1))
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  state1 = (state2 || idxX < inMinX || idxX > inMaxX);
	  if (state1)
	    {
	    *outPtr = constant;
	    }
	  else
	    {
	    *outPtr = *inPtr;
	    inPtr++;
	    }
	  outPtr++;
	  }
	}
      else
	{
	for (idxX = 0; idxX <= maxX; idxX++)
	  {
	  state1 = (state2 || idxX < inMinX || idxX > inMaxX);
	  for (idxC = 0; idxC < maxC; idxC++)
	    {
	    // Pixel operation
	    // Copy Pixel
	    state0 = (state1 || idxC >= inMaxC);
	    if (state0)
	      {
	      *outPtr = constant;
	      }
	    else
	      {
	      *outPtr = *inPtr;
	      inPtr++;
	      }
	    outPtr++;
	    }
	  }
	}
      outPtr += outIncY;
      if (!state2)
	{
	inPtr += inIncY;
	}
      }
    outPtr += outIncZ;
    if (!state3)
      {
      inPtr += inIncZ;
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageConstantPad::ThreadedExecute(vtkImageData *inData, 
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
  
  // need to get the correct pointer for the input data
  int inExt[6];
  this->ComputeRequiredInputUpdateExtent(inExt,outExt);
  void *inPtr = inData->GetScalarPointerForExtent(inExt);

  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageConstantPadExecute(this, inData, (float *)(inPtr), 
			       outData, (float *)(outPtr), outExt, inExt, id);
      break;
    case VTK_INT:
      vtkImageConstantPadExecute(this, inData, (int *)(inPtr), 
			       outData, (int *)(outPtr), outExt, inExt, id);
      break;
    case VTK_SHORT:
      vtkImageConstantPadExecute(this, inData, (short *)(inPtr), 
			       outData, (short *)(outPtr), outExt, inExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageConstantPadExecute(this, inData, (unsigned short *)(inPtr), 
				 outData, (unsigned short *)(outPtr), outExt,
				 inExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageConstantPadExecute(this, inData, (unsigned char *)(inPtr), 
				 outData, (unsigned char *)(outPtr), outExt,
				 inExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageConstantPad::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImagePadFilter::PrintSelf(os,indent);

  os << indent << "Constant: " << this->Constant << "\n";

}

