/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.cxx
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
#include "vtkImageAppendComponents.h"



//----------------------------------------------------------------------------
// Description:
// This method tells the ouput it will have more components
void vtkImageAppendComponents::ExecuteImageInformation()
{
  this->Output->SetNumberOfScalarComponents
    (this->Inputs[0]->GetNumberOfScalarComponents() 
     + this->Inputs[1]->GetNumberOfScalarComponents());
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAppendComponentsExecute(vtkImageAppendComponents *self,
					    vtkImageData *in1Data, 
					    T *in1PtrP,
					    vtkImageData *in2Data, 
					    T *in2PtrP,
					    vtkImageData *outData, 
					    T *outPtrP,
					    int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int outMaxC, maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  T *outPtr, *inPtr;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)(outData->GetNumberOfScalarComponents()*
			   (maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  
  outMaxC = outData->GetNumberOfScalarComponents();
  // Loop through image 1
  maxC = in1Data->GetNumberOfScalarComponents();
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtr = outPtrP + idxC;
    inPtr = in1PtrP + idxC;
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
	  // Pixel operation
	  *outPtr = *inPtr;
	  outPtr += outMaxC;
	  inPtr += maxC;
	  }
	outPtr += outIncY;
	inPtr += inIncY;
	}
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  outPtrP += maxC;
  
  // do image 2
  in2Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  maxC = in2Data->GetNumberOfScalarComponents();
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtr = outPtrP + idxC;
    inPtr = in2PtrP + idxC;
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
	  // Pixel operation
	  *outPtr = *inPtr;
	  outPtr += outMaxC;
	  inPtr += maxC;
	  }
	outPtr += outIncY;
	inPtr += inIncY;
	}
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppendComponents::ThreadedExecute(vtkImageData **inData, 
					       vtkImageData *outData,
					       int outExt[6], int id)
{
  void *in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  void *in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType() ||
      inData[1]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input1 ScalarType (" << 
    inData[0]->GetScalarType() << ") and input2 ScalarType (" <<
    inData[1]->GetScalarType()
    << "), must both match output ScalarType (" << outData->GetScalarType() << ")");
    return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageAppendComponentsExecute(this, inData[0], 
				      (float *)(in1Ptr), 
				      inData[1], (float *)(in2Ptr), 
				      outData, (float *)(outPtr), 
				      outExt, id);
      break;
    case VTK_INT:
      vtkImageAppendComponentsExecute(this, inData[0], (int *)(in1Ptr), 
				      inData[1], (int *)(in2Ptr), 
				      outData, (int *)(outPtr), 
				      outExt, id);
      break;
    case VTK_SHORT:
      vtkImageAppendComponentsExecute(this, inData[0], 
				      (short *)(in1Ptr), 
				      inData[1], (short *)(in2Ptr), 
				      outData, (short *)(outPtr), 
				      outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageAppendComponentsExecute(this, inData[0], 
				      (unsigned short *)(in1Ptr), inData[1], 
				      (unsigned short *)(in2Ptr), outData, 
				      (unsigned short *)(outPtr), 
				      outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageAppendComponentsExecute(this, inData[0], 
				      (unsigned char *)(in1Ptr), inData[1], 
				      (unsigned char *)(in2Ptr), outData, 
				      (unsigned char *)(outPtr), 
				      outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}















