/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAccumulate.cxx
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

#include "vtkImageAccumulate.h"
#include <math.h>
#include <stdlib.h>
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageAccumulate* vtkImageAccumulate::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageAccumulate");
  if(ret)
    {
    return (vtkImageAccumulate*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageAccumulate;
}





//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageAccumulate::vtkImageAccumulate()
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->ComponentSpacing[idx] = 1.0;
    this->ComponentOrigin[idx] = 0.0;
    this->ComponentExtent[idx*2] = 0;
    this->ComponentExtent[idx*2+1] = 0;
    }
  this->ComponentExtent[1] = 255;
}

//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->ComponentExtent[idx] != extent[idx])
      {
      this->ComponentExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int minX, int maxX, 
                                            int minY, int maxY,
                                            int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetComponentExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::GetComponentExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->ComponentExtent[idx];
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAccumulateExecute(vtkImageAccumulate *self,
                               vtkImageData *inData, T *inPtr,
                               vtkImageData *outData, int *outPtr)
{
  int min0, max0, min1, max1, min2, max2;
  int idx0, idx1, idx2, idxC;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  int *outPtrC;
  int numC, outIdx, *outExtent, *outIncs;
  float *origin, *spacing;
  unsigned long count = 0;
  unsigned long target;

  self = self;

  // Zero count in every bin
  outData->GetExtent(min0, max0, min1, max1, min2, max2);
  memset((void *)outPtr, 0, 
         (max0-min0+1)*(max1-min1+1)*(max2-min2+1)*sizeof(int));
    
  // Get information to march through data 
  numC = inData->GetNumberOfScalarComponents();
  inData->GetUpdateExtent(min0, max0, min1, max1, min2, max2);
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outExtent = outData->GetExtent();
  outIncs = outData->GetIncrements();
  origin = outData->GetOrigin();
  spacing = outData->GetSpacing();

  vtkGenericWarningMacro(<<"Executing with: " << min0 << "," << max0 << " " 
  << min1 << "," << max1 << " " << min2 << "," << max2);
  
  target = (unsigned long)((max2 - min2 + 1)*(max1 - min1 +1)/50.0);
  target++;

  inPtr2 = inPtr;
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    inPtr1 = inPtr2;
    for (idx1 = min1; !self->AbortExecute && idx1 <= max1; ++idx1)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;
      inPtr0  = inPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        inPtrC = inPtr0;
        // find the bin for this pixel.
        outPtrC = outPtr;
        for (idxC = 0; idxC < numC; ++idxC)
          {
          // compute the index
          outIdx = (int)(((float)*inPtrC - origin[idxC]) / spacing[idxC]);
          if (outIdx < outExtent[idxC*2] || outIdx > outExtent[idxC*2+1])
            {
            // Out of bin range
            outPtrC = NULL;
            break;
            }
          outPtrC += (outIdx - outExtent[idxC*2]) * outIncs[idxC];
          ++inPtrC;
          }
        if (outPtrC)
          {
          ++(*outPtrC);
          }
        
        inPtr0 += inInc0;
        }
      inPtr1 += inInc1;
      }
    inPtr2 += inInc2;
    }
}

        

//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
void vtkImageAccumulate::ExecuteData(vtkDataObject *vtkNotUsed(out))
{
  void *inPtr;
  void *outPtr;
  vtkImageData *inData = this->GetInput();
  vtkImageData *outData = this->GetOutput();
  
  vtkDebugMacro(<<"Executing image accumulate");
  
  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  outData->SetExtent(outData->GetWholeExtent());
  outData->AllocateScalars();
  
  inPtr = inData->GetScalarPointerForExtent(inData->GetUpdateExtent());
  outPtr = outData->GetScalarPointer();
  
  // Components turned into x, y and z
  if (this->GetInput()->GetNumberOfScalarComponents() > 3)
    {
    vtkErrorMacro("This filter can handle upto 3 components");
    return;
    }
  
  // this filter expects that output is type int.
  if (outData->GetScalarType() != VTK_INT)
    {
    vtkErrorMacro(<< "Execute: out ScalarType " << outData->GetScalarType()
                  << " must be int\n");
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro5(vtkImageAccumulateExecute, this, 
                      inData, (VTK_TT *)(inPtr), 
                      outData, (int *)(outPtr));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::ExecuteInformation(vtkImageData *vtkNotUsed(input), 
                                            vtkImageData *output)
{
  output->SetWholeExtent(this->ComponentExtent);
  output->SetOrigin(this->ComponentOrigin);
  output->SetSpacing(this->ComponentSpacing);
  output->SetNumberOfScalarComponents(1);
  output->SetScalarType(VTK_INT);
}

//----------------------------------------------------------------------------
// Get ALL of the input.
void vtkImageAccumulate::ComputeInputUpdateExtent(int inExt[6], 
                                                  int outExt[6])
{
  int *wholeExtent;

  outExt = outExt;
  wholeExtent = this->GetInput()->GetWholeExtent();
  memcpy(inExt, wholeExtent, 6*sizeof(int));
}


void vtkImageAccumulate::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "ComponentOrigin: ( "
     << this->ComponentOrigin[0] << ", "
     << this->ComponentOrigin[1] << ", "
     << this->ComponentOrigin[2] << " )\n";

  os << indent << "ComponentSpacing: ( "
     << this->ComponentSpacing[0] << ", "
     << this->ComponentSpacing[1] << ", "
     << this->ComponentSpacing[2] << " )\n";

  os << indent << "ComponentExtent: ( "
     << this->ComponentExtent[0] << "," << this->ComponentExtent[1] << " "
     << this->ComponentExtent[2] << "," << this->ComponentExtent[3] << " "
     << this->ComponentExtent[4] << "," << this->ComponentExtent[5] << " }\n";
}

