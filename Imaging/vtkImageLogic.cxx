/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImageLogic.h"
#include <math.h>
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageLogic* vtkImageLogic::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageLogic");
  if(ret)
    {
    return (vtkImageLogic*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageLogic;
}






//----------------------------------------------------------------------------
vtkImageLogic::vtkImageLogic()
{
  this->Operation = VTK_AND;
  this->OutputTrueValue = 255;
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
static void vtkImageLogicExecute1(vtkImageLogic *self,
				  vtkImageData *in1Data, 
				  T *in1Ptr,
				  vtkImageData *outData, 
				  T *outPtr,
				  int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  T trueValue = (T)(self->GetOutputTrueValue());
  int op = self->GetOperation();

  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*in1Data->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
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
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	switch (op)
	  {
	  case VTK_NOT:
	    if ( ! *in1Ptr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_NOP:
	    if (*in1Ptr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  }
	outPtr++;
	in1Ptr++;
	}
      outPtr += outIncY;
      in1Ptr += inIncY;
      }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageLogicExecute2(vtkImageLogic *self,
				  vtkImageData *in1Data, T *in1Ptr,
				  vtkImageData *in2Data, T *in2Ptr,
				  vtkImageData *outData, 
				  T *outPtr,
				  int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  T trueValue = (T)(self->GetOutputTrueValue());
  int op = self->GetOperation();

  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*in1Data->GetNumberOfScalarComponents();
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
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	switch (op)
	  {
	  case VTK_AND:
	    if (*in1Ptr && *in2Ptr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_OR:
	    if (*in1Ptr || *in2Ptr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_XOR:
	    if (( ! *in1Ptr && *in2Ptr) || (*in1Ptr && ! *in2Ptr))
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_NAND:
	    if ( ! (*in1Ptr && *in2Ptr))
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	    }
	    break;
	  case VTK_NOR:
	    if ( ! (*in1Ptr || *in2Ptr))
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  }
	outPtr++;
	in1Ptr++;
	in2Ptr++;
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
void vtkImageLogic::ThreadedExecute(vtkImageData **inData, 
				    vtkImageData *outData,
				    int outExt[6], int id)
{
  void *in1Ptr;
  void *outPtr;
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  if (inData[0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }
  in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0]->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }

  if (this->Operation == VTK_NOT || this->Operation == VTK_NOP)
    {
    switch (inData[0]->GetScalarType())
      {
      vtkTemplateMacro7(vtkImageLogicExecute1, this, inData[0], 
                        (VTK_TT *)(in1Ptr), outData, (VTK_TT *)(outPtr), 
                        outExt, id);
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
  else
    {
    void *in2Ptr;
    
    if (inData[1] == NULL)
      {
      vtkErrorMacro(<< "Input " << 1 << " must be specified.");
      return;
      }
    in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);

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
      vtkTemplateMacro9(vtkImageLogicExecute2, this, inData[0], 
                        (VTK_TT *)(in1Ptr), inData[1], (VTK_TT *)(in2Ptr), 
                        outData, (VTK_TT *)(outPtr), outExt, id);
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
}

void vtkImageLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);

  os << indent << "Operation: " << this->Operation << "\n";

  os << indent << "OutputTrueValue: " << this->OutputTrueValue << "\n";
}

