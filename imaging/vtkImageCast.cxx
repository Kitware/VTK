/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCast.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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

#include "vtkImageCast.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageCast* vtkImageCast::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCast");
  if(ret)
    {
    return (vtkImageCast*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCast;
}






//----------------------------------------------------------------------------
vtkImageCast::vtkImageCast()
{
  this->OutputScalarType = VTK_FLOAT;
  this->ClampOverflow = 0;
}


//----------------------------------------------------------------------------
// Just change the Image type.
void vtkImageCast::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
				      vtkImageData *outData)
{
  outData->SetScalarType(this->OutputScalarType);
}

//----------------------------------------------------------------------------
// The update method first checks to see is a cast is necessary.
void vtkImageCast::UpdateData(vtkDataObject *data)
{
  
  if (! this->GetInput() || ! this->GetOutput())
    {
    vtkErrorMacro("Update: Input or output is not set.");
    return;
    }
  
  // call the superclass update which will cause an execute.
  this->vtkImageToImageFilter::UpdateData(data);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageCastExecute(vtkImageCast *self,
				vtkImageData *inData, IT *inPtr,
				vtkImageData *outData, OT *outPtr,
				int outExt[6], int id)
{
  float typeMin, typeMax, val;
  int clamp;
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;

  // for preventing overflow
  typeMin = outData->GetScalarTypeMin();
  typeMax = outData->GetScalarTypeMax();
  clamp = self->GetClampOverflow();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
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
      // put the test for clamp to avoid the innermost loop
      if (clamp)
	{
	for (idxR = 0; idxR < rowLength; idxR++)
	  {
	  // Pixel operation
	  val = (float)(*inPtr);
	  if (val > typeMax)
	    {
	    val = typeMax;
	    }
	  if (val < typeMin)
	    {
	    val = typeMin;
	    }
	  *outPtr = (OT)(val);
	  outPtr++;
	  inPtr++;
	  }
	}
      else
	{
	for (idxR = 0; idxR < rowLength; idxR++)
	  {
	  // Pixel operation
	  *outPtr = (OT)(*inPtr);
	  outPtr++;
	  inPtr++;
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
template <class T>
static void vtkImageCastExecute(vtkImageCast *self,
				vtkImageData *inData, T *inPtr,
				vtkImageData *outData, int outExt[6], int id)
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  switch (outData->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (double *)(outPtr),outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (float *)(outPtr),outExt, id);
      break;
    case VTK_LONG:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (long *)(outPtr),outExt, id); 
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (unsigned long *)(outPtr),outExt, id); 
      break;
    case VTK_INT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (int *)(outPtr),outExt, id); 
      break;
    case VTK_UNSIGNED_INT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (unsigned int *)(outPtr),outExt, id); 
      break;
    case VTK_SHORT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (unsigned short *)(outPtr),outExt, id); 
      break;
    case VTK_CHAR:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (char *)(outPtr),outExt, id); 
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (unsigned char *)(outPtr),outExt, id); 
      break;
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageCast::ThreadedExecute(vtkImageData *inData, 
				   vtkImageData *outData,
				   int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  switch (inData->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageCastExecute(this, inData, (double *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageCastExecute(this, inData, (float *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_LONG:
      vtkImageCastExecute(this, inData, (long *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageCastExecute(this, inData, (unsigned long *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_INT:
      vtkImageCastExecute(this, inData, (int *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageCastExecute(this, inData, (unsigned int *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_SHORT:
      vtkImageCastExecute(this, inData, (short *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCastExecute(this, inData, (unsigned short *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_CHAR:
      vtkImageCastExecute(this, inData, (char *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCastExecute(this, inData, (unsigned char *)(inPtr), 
			  outData, outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageCast::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "ClampOverflow: ";
  if (this->ClampOverflow)
    {
    os << "On\n";
    }
  else 
    {
    os << "Off\n";
    }
}

