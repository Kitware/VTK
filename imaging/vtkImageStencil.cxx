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
  this->SetNumberOfInputs(2);
  this->Inputs[0] = NULL;
  this->Inputs[1] = NULL;

  this->StencilFunction = NULL;
  this->ClippingExtents = NULL;
  
  this->ReverseStencil = 0;

  this->DefaultColor[0] = 1;
  this->DefaultColor[1] = 1;
  this->DefaultColor[2] = 1;
  this->DefaultColor[3] = 1;
}

//----------------------------------------------------------------------------
vtkImageStencil::~vtkImageStencil()
{
  this->SetStencilFunction(NULL);
  this->SetClippingExtents(NULL);
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetInput(vtkImageData *input)
{ 
  this->vtkImageMultipleInputFilter::SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetInput(int n, vtkImageData *input)
{
  this->vtkImageMultipleInputFilter::SetInput(n, input);
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetInput2(vtkImageData *input)
{ 
  this->vtkImageMultipleInputFilter::SetInput(1, input);
}

//----------------------------------------------------------------------------
void vtkImageStencil::ComputeInputUpdateExtent(int inExt[6],
                                               int outExt[6],
                                               int whichInput)
{
  int i;

  for (i = 0; i < 6; i++)
    {
    inExt[i] = outExt[i];
    }

  if (whichInput != 0)
    {
    // Clip, just to make sure we hit _some_ of the input extent,
    // otherwise an error will occur even though we don't need
    // any part of that input's data
    int *wholeExtent = this->GetInput(whichInput)->GetWholeExtent();
    for (i = 0; i < 3; i++)
      {
      if (inExt[2*i] < wholeExtent[2*i])
        {
        inExt[2*i] = wholeExtent[2*i];
        }
      if (inExt[2*i+1] > wholeExtent[2*i+1])
        {
        inExt[2*i+1] = wholeExtent[2*i+1];
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageStencil::ExecuteInformation(vtkImageData **inputs, 
                                         vtkImageData *output)
{
  vtkImageData *input = inputs[0];
  vtkImageData *input2 = inputs[1];

  if (input2)
    {
    if (input2->GetScalarType() != input->GetScalarType())
      {
      vtkErrorMacro("ExecuteInformation: inputs must have same scalar type");
      }
    }

  if (this->StencilFunction)
    {
    if (!this->ClippingExtents)
      {
      this->ClippingExtents = vtkImageClippingExtents::New();
      }
    this->ClippingExtents->SetClippingObject(this->StencilFunction);
    }
  if (this->ClippingExtents)
    {
    this->ClippingExtents->BuildExtents(output);
    }
  else
    {
    vtkErrorMacro("ExecuteInformation: no StencilFunction or ClippingExtents");
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
        background[i] = (T)self->GetDefaultColor()[i];
        }
      else
        { // round float to nearest int
        background[i] = (T)floor(self->GetDefaultColor()[i] + 0.5);
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
                                   vtkImageData *in2Data,T *vtkNotUsed(in2Ptr),
                                   vtkImageData *outData, T *outPtr,
                                   int outExt[6], int id)
{
  int numscalars;
  int idX, idY, idZ;
  int r1, r2, cr1, cr2, iter, rval;
  int outIncX, outIncY, outIncZ;
  int inExt[6], inInc[3];
  int in2Ext[6], in2Inc[3];
  unsigned long count = 0;
  unsigned long target;
  T *background, *tempPtr;

  // get the clipping extents
  vtkImageClippingExtents *clippingExtents = self->GetClippingExtents();

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

      if (clippingExtents == NULL)
        {
        tempPtr = inPtr + (inInc[2]*(idZ - inExt[4]) +
                           inInc[1]*(idY - inExt[2]) +
                           numscalars*(outExt[0] - inExt[0]));
        for (idX = outExt[0]; idX <= outExt[1]; idX++)
          {
          vtkCopyPixel(outPtr, tempPtr, numscalars);
          tempPtr += numscalars;
          }          
        }
      else
        {
        iter = 0;
        cr1 = outExt[0];
        for (;;)
          {
          rval = clippingExtents->GetNextExtent(r1, r2, outExt[0], outExt[1],
                                                idY, idZ, iter);

          cr2 = r1 - 1;
          if (self->GetReverseStencil())
            {
            // do stencil portion
            for (idX = cr1; idX <= cr2; idX++)
              {
              vtkCopyPixel(outPtr, background, numscalars);
              }
            }
          else
            {
            // do unchanged portion
            tempPtr = inPtr + (inInc[2]*(idZ - inExt[4]) +
                               inInc[1]*(idY - inExt[2]) +
                               numscalars*(cr1 - inExt[0]));
            for (idX = cr1; idX <= cr2; idX++)
              {
              vtkCopyPixel(outPtr, tempPtr, numscalars);
              tempPtr += numscalars;
              }
            }
          cr1 = r2 + 1;

          // break if no foreground extents left
          if (rval == 0)
            {
            break;
            }

          if (self->GetReverseStencil())
            {
            // do unchanged portion
            tempPtr = inPtr + (inInc[2]*(idZ - inExt[4]) +
                               inInc[1]*(idY - inExt[2]) +
                               numscalars*(r1 - inExt[0]));
            for (idX = r1; idX <= r2; idX++)
              {
              vtkCopyPixel(outPtr, tempPtr, numscalars);
              tempPtr += numscalars;
              }
            }
          else
            {
            // do stencil portion
            for (idX = r1; idX <= r2; idX++)
              {
              vtkCopyPixel(outPtr, background, numscalars);
              }
            }
          }
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  vtkFreeBackground(self, background);
}


//----------------------------------------------------------------------------
void vtkImageStencil::ThreadedExecute(vtkImageData **inData, 
                                      vtkImageData *outData,
                                      int outExt[6], int id)
{
  void *in1Ptr, *in2Ptr;
  void *outPtr;
  
  vtkDebugMacro("Execute: inData = " << inData << ", outData = " << outData);
  
  if (inData[0] == NULL)
    {
    vtkErrorMacro("Input " << 0 << " must be specified.");
    return;
    }
  in1Ptr = inData[0]->GetScalarPointer();
  outPtr = outData->GetScalarPointerForExtent(outExt);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro("Execute: Input ScalarType, " << inData[0]->GetScalarType()
             << ", must match Output ScalarType " << outData->GetScalarType());
    return;
    }
    
  if (inData[1] == NULL)
    {
    in2Ptr = NULL;
    }
  else
    {
    in2Ptr = inData[1]->GetScalarPointer();
    if (inData[1]->GetScalarType() != inData[0]->GetScalarType())
      {
      vtkErrorMacro("Execute: Input2 ScalarType, " <<inData[1]->GetScalarType()
            << ", must match Input ScalarType " << inData[0]->GetScalarType());
      return;
      }
    }
  
  switch (inData[0]->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageStencilExecute, this, inData[0], 
                      (VTK_TT *)(in1Ptr), inData[1], (VTK_TT *)(in2Ptr), 
                      outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro("Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageMultipleInputFilter::PrintSelf(os, indent);

  os << indent << "StencilFunction: " << this->StencilFunction << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ?
		                         "On\n" : "Off\n");

  os << indent << "ClippingExtents: " << this->ClippingExtents << "\n";

  os << indent << "DefaultValue: " << this->DefaultColor[0] << "\n";

  os << indent << "DefaultColor: (" << this->DefaultColor[0] << ", "
                                    << this->DefaultColor[1] << ", "
                                    << this->DefaultColor[2] << ", "
                                    << this->DefaultColor[3] << ")\n";
}

