/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCheckerboard.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported by PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources

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
#include "vtkImageCheckerboard.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkImageCheckerboard* vtkImageCheckerboard::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCheckerboard");
  if(ret)
    {
    return (vtkImageCheckerboard*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCheckerboard;
}

//----------------------------------------------------------------------------
vtkImageCheckerboard::vtkImageCheckerboard()
{
  this->NumberOfDivisions[0] = 2;
  this->NumberOfDivisions[1] = 2;
  this->NumberOfDivisions[2] = 2;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageCheckerboardExecute2(vtkImageCheckerboard *self,
				  vtkImageData *in1Data, T *in1Ptr,
				  vtkImageData *in2Data, T *in2Ptr,
				  vtkImageData *outData, 
				  T *outPtr,
				  int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int dimWholeX, dimWholeY, dimWholeZ;
  int divX, divY, divZ;
  int nComp;
  int selectX, selectY, selectZ;
  int which;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  int wholeExt[6];
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  
  // find the region to loop over
  nComp = in1Data->GetNumberOfScalarComponents();
  rowLength = (outExt[1] - outExt[0]+1)*nComp;
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];

  
  outData->GetWholeExtent(wholeExt);
  
  dimWholeX = wholeExt[1] - wholeExt[0] + 1;
  dimWholeY = wholeExt[3] - wholeExt[2] + 1;
  dimWholeZ = wholeExt[5] - wholeExt[4] + 1;
  
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  divX = dimWholeX / self->GetNumberOfDivisions()[0] * nComp;
  if (divX == 0)
    {
    divX = 1;
    }
  divY = dimWholeY / self->GetNumberOfDivisions()[1];
  if (divY == 0)
    {
    divY = 1;
    }
  divZ = dimWholeZ /self->GetNumberOfDivisions()[2];
  if (divZ == 0)
    {
    divZ = 1;
    }
  
  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    selectZ = (((idxZ + outExt[4]) / divZ) % 2) << 2;
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
      selectY = (((idxY + outExt[2]) / divY) % 2) << 1;
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	
	selectX = ((idxR + outExt[0]) / divX) % 2;
	which = selectZ + selectY + selectX;
	switch (which)
	  {
	  case 0:
	    *outPtr = *in1Ptr;
	    break;
	  case 1:
	    *outPtr = *in2Ptr;
	    break;
	  case 2:
	    *outPtr = *in2Ptr;
	    break;
	  case 3:
	    *outPtr = *in1Ptr;
	    break;
	  case 4:
	    *outPtr = *in2Ptr;
	    break;
	  case 5:
	    *outPtr = *in1Ptr;
	    break;
	  case 6:
	    *outPtr = *in1Ptr;
	    break;
	  case 7:
	    *outPtr = *in2Ptr;
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
void vtkImageCheckerboard::ThreadedExecute(vtkImageData **inData, 
				    vtkImageData *outData,
				    int outExt[6], int id)
{
  void *in1Ptr, *in2Ptr;
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
    vtkTemplateMacro9(vtkImageCheckerboardExecute2, this, inData[0], 
		      (VTK_TT *)(in1Ptr), inData[1], (VTK_TT *)(in2Ptr), 
		      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageCheckerboard::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);
  os << indent << "NumberOfDivisions: (" << this->NumberOfDivisions[0] << ", "
     << this->NumberOfDivisions[1] << ", "
     << this->NumberOfDivisions[2] << ")\n";
}

