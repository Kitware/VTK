/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.cxx
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
#include "vtkImageDotProduct.h"


//----------------------------------------------------------------------------
// Description:
// Colapse the first axis
void vtkImageDotProduct::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents(1);
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageDotProductExecute(vtkImageDotProduct *self,
				      vtkImageData *in1Data, 
				      T *in1Ptr,
				      vtkImageData *in2Data, 
				      T *in2Ptr,
				      vtkImageData *outData, 
				      T *outPtr,
				      int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float dot;
  
  // find the region to loop over
  maxC = in1Data->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// now process the components
	dot = 0.0;
	for (idxC = 0; idxC < maxC; idxC++)
	  {
	  // Pixel operation
	  dot += (float)(*in1Ptr * *in2Ptr);
	  in1Ptr++;
	  in2Ptr++;
	  }
	*outPtr = (T)dot;
	outPtr++;
	}
      outPtr += outIncY;
      in1Ptr += inIncY;
      in2Ptr += in2IncY;
      }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    in2Ptr += in2IncZ;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageDotProduct::ThreadedExecute(vtkImageData **inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  void *in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  void *in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType() ||
      inData[1]->GetScalarType() != outData->GetScalarType())
    {
      vtkErrorMacro(<< "Execute: input ScalarType, " << 
      inData[0]->GetScalarType()
      << ", must match out ScalarType " << outData->GetScalarType());
      return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageDotProductExecute(this, inData[0], 
				(float *)(in1Ptr), 
				inData[1], (float *)(in2Ptr), 
				outData, (float *)(outPtr), 
				outExt, id);
      break;
    case VTK_INT:
      vtkImageDotProductExecute(this, inData[0], (int *)(in1Ptr), 
				inData[1], (int *)(in2Ptr), 
				outData, (int *)(outPtr), 
				outExt, id);
      break;
    case VTK_SHORT:
      vtkImageDotProductExecute(this, inData[0], 
				(short *)(in1Ptr), 
				inData[1], (short *)(in2Ptr), 
				outData, (short *)(outPtr), 
				outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDotProductExecute(this, inData[0], 
				(unsigned short *)(in1Ptr), 
				inData[1], 
				(unsigned short *)(in2Ptr), 
				outData, 
				(unsigned short *)(outPtr), 
				outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDotProductExecute(this, inData[0], 
				(unsigned char *)(in1Ptr), 
				inData[1], 
				(unsigned char *)(in2Ptr), 
				outData, 
				(unsigned char *)(outPtr), 
				outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}














