/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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
#include "vtkImageStencil.h"
#include "vtkObjectFactory.h"
#include <math.h>

//----------------------------------------------------------------------------
vtkImageStencil* vtkImageStencil::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageStencil");
  if(ret)
    {
    return (vtkImageStencil*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageStencil;
}

//----------------------------------------------------------------------------
vtkImageStencil::vtkImageStencil()
{
  this->ReverseStencil = 0;

  this->BackgroundColor[0] = 1;
  this->BackgroundColor[1] = 1;
  this->BackgroundColor[2] = 1;
  this->BackgroundColor[3] = 1;
}

//----------------------------------------------------------------------------
vtkImageStencil::~vtkImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetStencil(vtkImageStencilData *stencil)
{
  this->vtkProcessObject::SetNthInput(2, stencil); 
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageStencil::GetStencil()
{
  if (this->NumberOfInputs < 3) 
    { 
    return NULL;
    }
  else
    {
    return (vtkImageStencilData *)(this->Inputs[2]); 
    }
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetBackgroundInput(vtkImageData *data)
{
  this->vtkProcessObject::SetNthInput(1, data); 
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageStencil::GetBackgroundInput()
{
  if (this->NumberOfInputs < 2) 
    { 
    return NULL;
    }
  else
    {
    return (vtkImageData *)(this->Inputs[1]); 
    }
}

//----------------------------------------------------------------------------
void vtkImageStencil::ExecuteInformation(vtkImageData *input, 
                                         vtkImageData *vtkNotUsed(output))
{
  // need to set the spacing and origin of the stencil to match the output
  vtkImageStencilData *stencil = this->GetStencil();
  if (stencil)
    {
    stencil->SetSpacing(input->GetSpacing());
    stencil->SetOrigin(input->GetOrigin());
    }
}

//----------------------------------------------------------------------------
// Some helper functions for 'Execute'
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// copy a pixel, advance the output pointer but not the input pointer

template<class T>
static inline void vtkCopyPixel(T *&out, const T *in, int numscalars)
{
  do
    {
    *out++ = *in++;
    }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Convert background color from float to appropriate type

template <class T>
static void vtkAllocBackground(vtkImageStencil *self,
                               T *&background)
{
  int numComponents = self->GetOutput()->GetNumberOfScalarComponents();
  int scalarType = self->GetOutput()->GetScalarType();

  background = new T[numComponents];

  for (int i = 0; i < numComponents; i++)
    {
    if (i < 4)
      {
      if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
        {
        background[i] = (T)self->GetBackgroundColor()[i];
        }
      else
        { // round float to nearest int
        background[i] = (T)floor(self->GetBackgroundColor()[i] + 0.5);
        }
      }
    else
      { // all values past 4 are set to zero
      background[i] = 0;
      }
    }
}

//----------------------------------------------------------------------------
template <class T>
static void vtkFreeBackground(vtkImageStencil *vtkNotUsed(self),
                              T *&background)
{
  delete [] background;
  background = NULL;
}

//----------------------------------------------------------------------------
template <class T>
static void vtkImageStencilExecute(vtkImageStencil *self,
                                   vtkImageData *inData, T *inPtr,
                                   vtkImageData *in2Data,T *in2Ptr,
                                   vtkImageData *outData, T *outPtr,
                                   int outExt[6], int id)
{
  int numscalars, inIncX;
  int idX, idY, idZ;
  int r1, r2, cr1, cr2, iter, rval;
  int outIncX, outIncY, outIncZ;
  int inExt[6], inInc[3];
  int in2Ext[6], in2Inc[3];
  unsigned long count = 0;
  unsigned long target;
  T *background, *tempPtr;

  // get the clipping extents
  vtkImageStencilData *stencil = self->GetStencil();

  // find maximum input range
  inData->GetExtent(inExt);
  inData->GetIncrements(inInc);
  if (in2Data)
    {
    in2Data->GetExtent(in2Ext);
    in2Data->GetIncrements(in2Inc);
    }

  // Get Increments to march through data 
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();

  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;  
  
  // set color for area outside of input volume extent
  vtkAllocBackground(self, background);

  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (!id) 
        {
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      iter = 0;
      cr1 = outExt[0];
      for (;;)
	{
	rval = 0;
	r1 = outExt[1] + 1;
	r2 = outExt[1];
	if (stencil)
	  {
	  rval = stencil->GetNextExtent(r1, r2, outExt[0], outExt[1],
					idY, idZ, iter);
	  }

	tempPtr = background;
	inIncX = 0; 
	if (self->GetReverseStencil())
	  {
	  tempPtr = inPtr + (inInc[2]*(idZ - inExt[4]) +
			     inInc[1]*(idY - inExt[2]) +
			     numscalars*(cr1 - inExt[0]));
	  inIncX = numscalars;
	  }
	else if (in2Ptr)
	  {
	  tempPtr = in2Ptr + (in2Inc[2]*(idZ - in2Ext[4]) +
			      in2Inc[1]*(idY - in2Ext[2]) +
			      numscalars*(cr1 - in2Ext[0]));
	  inIncX = numscalars;
	  }	    

	cr2 = r1 - 1;
	for (idX = cr1; idX <= cr2; idX++)
	  {
          vtkCopyPixel(outPtr, tempPtr, numscalars);
	  tempPtr += inIncX;
	  }
	cr1 = r2 + 1; // for next time 'round

	// break if no foreground extents left
	if (rval == 0)
	  {
          break;
	  }

	tempPtr = background;
	inIncX = 0;
	if (!self->GetReverseStencil())
	  {
	  tempPtr = inPtr + (inInc[2]*(idZ - inExt[4]) +
			     inInc[1]*(idY - inExt[2]) +
			     numscalars*(r1 - inExt[0]));
	  inIncX = numscalars;
	  }
	else if (in2Ptr)
	  {
	  tempPtr = in2Ptr + (in2Inc[2]*(idZ - in2Ext[4]) +
			      in2Inc[1]*(idY - in2Ext[2]) +
			      numscalars*(r1 - in2Ext[0]));
	  inIncX = numscalars;
	  }	    

	for (idX = r1; idX <= r2; idX++)
	  {
          vtkCopyPixel(outPtr, tempPtr, numscalars);
	  tempPtr += inIncX;
	  }
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  vtkFreeBackground(self, background);
}


//----------------------------------------------------------------------------
void vtkImageStencil::ThreadedExecute(vtkImageData *inData, 
                                      vtkImageData *outData,
                                      int outExt[6], int id)
{
  void *inPtr, *inPtr2;
  void *outPtr;
  vtkImageData *inData2 = this->GetBackgroundInput();
  
  vtkDebugMacro("Execute: inData = " << inData << ", outData = " << outData);
  
  inPtr = inData->GetScalarPointer();
  outPtr = outData->GetScalarPointerForExtent(outExt);

  inPtr2 = NULL;
  if (inData2)
    {
    inPtr2 = inData2->GetScalarPointer();
    if (inData2->GetScalarType() != inData->GetScalarType())
      {
      if (id == 0)
	{
	vtkErrorMacro("Execute: BackgroundInput ScalarType " 
		      << inData2->GetScalarType()
		      << ", must match Input ScalarType "
		      << inData->GetScalarType());
	}
      return;
      }
    else if (inData2->GetNumberOfScalarComponents() 
	     != inData->GetNumberOfScalarComponents())
      {
      if (id == 0)
	{
        vtkErrorMacro("Execute: BackgroundInput NumberOfScalarComponents " 
		      << inData2->GetNumberOfScalarComponents()
		      << ", must match Input NumberOfScalarComponents "
		      << inData->GetNumberOfScalarComponents());
	}
      return;
      }
    int *wholeExt1 = inData->GetWholeExtent();
    int *wholeExt2 = inData2->GetWholeExtent();
    for (int i = 0; i < 6; i++)
      {
      if (wholeExt1[i] != wholeExt2[i])
	{
	if (id == 0)
	  {
	  vtkErrorMacro("Execute: BackgroundInput must have the same "
			"WholeExtent as the Input");
	  }
	return;
	}
      }
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageStencilExecute, this, inData, 
                      (VTK_TT *)(inPtr), inData2, (VTK_TT *)(inPtr2), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro("Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageToImageFilter::PrintSelf(os, indent);

  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ?
		                         "On\n" : "Off\n");

  os << indent << "BackgroundInput: " << this->GetBackgroundInput() << "\n";
  os << indent << "BackgroundValue: " << this->BackgroundColor[0] << "\n";

  os << indent << "BackgroundColor: (" << this->BackgroundColor[0] << ", "
                                    << this->BackgroundColor[1] << ", "
                                    << this->BackgroundColor[2] << ", "
                                    << this->BackgroundColor[3] << ")\n";
}

