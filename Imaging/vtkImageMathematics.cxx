/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMathematics.cxx
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
#include "vtkImageData.h"
#include "vtkImageMathematics.h"
#include <math.h>
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageMathematics* vtkImageMathematics::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMathematics");
  if(ret)
    {
    return (vtkImageMathematics*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMathematics;
}






//----------------------------------------------------------------------------
vtkImageMathematics::vtkImageMathematics()
{
  this->Operation = VTK_ADD;
  this->ConstantK = 1.0;
  this->ConstantC = 0.0;
}



//----------------------------------------------------------------------------
// The output extent is the intersection.
void vtkImageMathematics::ExecuteInformation(vtkImageData **inDatas, 
					     vtkImageData *outData)
{
  int ext[6], *ext2, idx;

  inDatas[0]->GetWholeExtent(ext);

  // two input take intersection
  if (this->Operation == VTK_ADD || this->Operation == VTK_SUBTRACT || 
      this->Operation == VTK_MULTIPLY || this->Operation == VTK_DIVIDE ||
      this->Operation == VTK_MIN || this->Operation == VTK_MAX || 
      this->Operation == VTK_ATAN2) 
    {
    ext2 = this->GetInput(1)->GetWholeExtent();
    for (idx = 0; idx < 3; ++idx)
      {
      if (ext2[idx*2] > ext[idx*2])
	{
	ext[idx*2] = ext2[idx*2];
	}
      if (ext2[idx*2+1] < ext[idx*2+1])
	{
	ext[idx*2+1] = ext2[idx*2+1];
	}
      }
    }
  
  outData->SetWholeExtent(ext);
}






//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
static void vtkImageMathematicsExecute1(vtkImageMathematics *self,
					vtkImageData *in1Data, T *in1Ptr,
					vtkImageData *outData, T *outPtr,
					int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();

  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*in1Data->GetNumberOfScalarComponents();
  // What a pain.  Maybe I should just make another filter.
  if (op == VTK_CONJUGATE)
    {
    rowLength = (outExt[1] - outExt[0]+1);
    }
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  double constantk = self->GetConstantK();
  double constantc = self->GetConstantC();
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
	// Pixel operaton
	switch (op)
	  {
	  case VTK_INVERT:
	    *outPtr = (T)(1.0 / *in1Ptr);
	    break;
	  case VTK_SIN:
	    *outPtr = (T)(sin((double)*in1Ptr));
	    break;
	  case VTK_COS:
	    *outPtr = (T)(cos((double)*in1Ptr));
	    break;
	  case VTK_EXP:
	    *outPtr = (T)(exp((double)*in1Ptr));
	    break;
	  case VTK_LOG:
	    *outPtr = (T)(log((double)*in1Ptr));
	    break;
	  case VTK_ABS:
	    *outPtr = (T)(fabs((double)*in1Ptr));
	    break;
	  case VTK_SQR:
	    *outPtr = (T)(*in1Ptr * *in1Ptr);
	    break;
	  case VTK_SQRT:
	    *outPtr = (T)(sqrt((double)*in1Ptr));
	    break;
	  case VTK_ATAN:
	    *outPtr = (T)(atan((double)*in1Ptr));
	    break;
	  case VTK_MULTIPLYBYK:
	    *outPtr = (T)(constantk*(double)*in1Ptr);
	    break;
	  case VTK_ADDC:
	    *outPtr = (T)((T)constantc + *in1Ptr);
	    break;
	  case VTK_REPLACECBYK:
	    *outPtr = (*in1Ptr == (T)constantc)?((T)constantk):(*in1Ptr);
	    break;
	  case VTK_CONJUGATE:
	    outPtr[0] = in1Ptr[0];
	    outPtr[1] = (T)(-1.0*(double)(in1Ptr[1]));
	    // Why bother trtying to figure out the continuous increments.
	    outPtr++;
	    in1Ptr++;
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
static void vtkImageMathematicsExecute2(vtkImageMathematics *self,
					vtkImageData *in1Data, T *in1Ptr,
					vtkImageData *in2Data, T *in2Ptr,
					vtkImageData *outData, T *outPtr,
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
  int op = self->GetOperation();

  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*in1Data->GetNumberOfScalarComponents();
  // What a pain.  Maybe I should just make another filter.
  if (op == VTK_COMPLEX_MULTIPLY)
    {
    rowLength = (outExt[1] - outExt[0]+1);
    }

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
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	switch (op)
	  {
	  case VTK_ADD:
	    *outPtr = *in1Ptr + *in2Ptr;
	    break;
	  case VTK_SUBTRACT:
	    *outPtr = *in1Ptr - *in2Ptr;
	    break;
	  case VTK_MULTIPLY:
	    *outPtr = *in1Ptr * *in2Ptr;
	    break;
	  case VTK_DIVIDE:
	    if (*in2Ptr)
	      {
	      *outPtr = *in1Ptr / *in2Ptr;
	      }
	    else
	      {
	      *outPtr = (T)(*in1Ptr / 0.00001);
	      }
	    break;
	  case VTK_MIN:
	    if (*in1Ptr < *in2Ptr)
	      {
	      *outPtr = *in1Ptr;
	      }
	    else
	      {
	      *outPtr = *in2Ptr;
	      }
	    break;
	  case VTK_MAX:
	    if (*in1Ptr > *in2Ptr)
	      {
	      *outPtr = *in1Ptr;
	      }
	    else
	      {
	      *outPtr = *in2Ptr;
	      }
	    break;
	  case VTK_ATAN2:
	      if (*in1Ptr == 0.0 && *in2Ptr == 0.0)
		{
		*outPtr = 0;
		}
	      else
		{
	        *outPtr =  (T)atan2((double)*in1Ptr,(double)*in2Ptr);
		}
	    break;
	  case VTK_COMPLEX_MULTIPLY:
	    outPtr[0] = in1Ptr[0] * in2Ptr[0] - in1Ptr[1] * in2Ptr[1];
	    outPtr[1] = in1Ptr[1] * in2Ptr[0] + in1Ptr[0] * in2Ptr[1];
	    // Why bother trtying to figure out the continuous increments.
	    outPtr++;
	    in1Ptr++;
	    in2Ptr++;
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
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageMathematics::ThreadedExecute(vtkImageData **inData, 
					  vtkImageData *outData,
					  int outExt[6], int id)
{
  void *inPtr1;
  void *outPtr;
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  

  if (inData[0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }

  inPtr1 = inData[0]->GetScalarPointerForExtent(outExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);
  

  if (this->Operation == VTK_ADD || this->Operation == VTK_SUBTRACT || 
      this->Operation == VTK_MULTIPLY || this->Operation == VTK_DIVIDE ||
      this->Operation == VTK_MIN || this->Operation == VTK_MAX || 
      this->Operation == VTK_ATAN2 || this->Operation == VTK_COMPLEX_MULTIPLY) 
    {
    void *inPtr2;
    
    if (inData[1] == NULL)
      {
      vtkErrorMacro(<< "Input " << 1 << " must be specified.");
      return;
      }

    if ( this->Operation == VTK_COMPLEX_MULTIPLY )
      {
      if (inData[0]->GetNumberOfScalarComponents() != 2 ||
	  inData[1]->GetNumberOfScalarComponents() != 2)
	{
        vtkErrorMacro("Complex inputs must have two components.");
	return;
	}
      }

    inPtr2 = inData[1]->GetScalarPointerForExtent(outExt);

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
      vtkTemplateMacro9(vtkImageMathematicsExecute2,
                        this,inData[0], (VTK_TT *)(inPtr1), 
                        inData[1], (VTK_TT *)(inPtr2), 
                        outData, (VTK_TT *)(outPtr), outExt, id);
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
  else
    {
    // this filter expects that input is the same type as output.
    if (inData[0]->GetScalarType() != outData->GetScalarType())
      {
      vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0]->GetScalarType()
      << ", must match out ScalarType " << outData->GetScalarType());
      return;
      }

    if ( this->Operation == VTK_CONJUGATE )
      {
      if (inData[0]->GetNumberOfScalarComponents() != 2)
	{
        vtkErrorMacro("Complex inputs must have two components.");
	return;
	}
      }

    switch (inData[0]->GetScalarType())
      {
      vtkTemplateMacro7(vtkImageMathematicsExecute1,
                        this, inData[0], (VTK_TT *)(inPtr1), 
                        outData, (VTK_TT *)(outPtr), outExt, id);
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
}

void vtkImageMathematics::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "ConstantK: " << this->ConstantK << "\n";
  os << indent << "ConstantC: " << this->ConstantC << "\n";

}

