/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkImageDotProduct.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageDotProduct* vtkImageDotProduct::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDotProduct");
  if(ret)
    {
    return (vtkImageDotProduct*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDotProduct;
}





//----------------------------------------------------------------------------
// Colapse the first axis
void vtkImageDotProduct::ExecuteInformation(vtkImageData **vtkNotUsed(inDatas),
					    vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
}


//----------------------------------------------------------------------------
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
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageDotProduct::ThreadedExecute(vtkImageData **inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  void *in1Ptr;
  void *in2Ptr;
  void *outPtr;
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  if (inData[0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }
  if (inData[1] == NULL)
    {
    vtkErrorMacro(<< "Input " << 1 << " must be specified.");
    return;
    }
  in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input1 ScalarType, "
                  <<  inData[0]->GetScalarType()
                  << ", must match output ScalarType "
                  << outData->GetScalarType());
    return;
    }
  
  if (inData[1]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input2 ScalarType, "
                  << inData[1]->GetScalarType()
                  << ", must match output ScalarType "
                  << outData->GetScalarType());
    return;
    }
  
  // this filter expects that inputs that have the same number of components
  if (inData[0]->GetNumberOfScalarComponents() != 
      inData[1]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input1 NumberOfScalarComponents, "
                  << inData[0]->GetNumberOfScalarComponents()
                  << ", must match out input2 NumberOfScalarComponents "
                  << inData[1]->GetNumberOfScalarComponents());
    return;
    }

  switch (inData[0]->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageDotProductExecute(this, inData[0], 
				(double *)(in1Ptr), 
				inData[1], (double *)(in2Ptr), 
				outData, (double *)(outPtr), 
				outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageDotProductExecute(this, inData[0], 
				(float *)(in1Ptr), 
				inData[1], (float *)(in2Ptr), 
				outData, (float *)(outPtr), 
				outExt, id);
      break;
    case VTK_LONG:
      vtkImageDotProductExecute(this, inData[0], (long *)(in1Ptr), 
				inData[1], (long *)(in2Ptr), 
				outData, (long *)(outPtr), 
				outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageDotProductExecute(this, inData[0], 
				(unsigned long *)(in1Ptr), 
				inData[1], 
				(unsigned long *)(in2Ptr), 
				outData, 
				(unsigned long *)(outPtr), 
				outExt, id);
      break;
    case VTK_INT:
      vtkImageDotProductExecute(this, inData[0], (int *)(in1Ptr), 
				inData[1], (int *)(in2Ptr), 
				outData, (int *)(outPtr), 
				outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageDotProductExecute(this, inData[0], 
				(unsigned int *)(in1Ptr), 
				inData[1], 
				(unsigned int *)(in2Ptr), 
				outData, 
				(unsigned int *)(outPtr), 
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
    case VTK_CHAR:
      vtkImageDotProductExecute(this, inData[0], 
				(char *)(in1Ptr), 
				inData[1], (char *)(in2Ptr), 
				outData, (char *)(outPtr), 
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














