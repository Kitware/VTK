/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageThreshold.cxx
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
#include "vtkImageThreshold.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageThreshold* vtkImageThreshold::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageThreshold");
  if(ret)
    {
    return (vtkImageThreshold*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageThreshold;
}






//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageThreshold::vtkImageThreshold()
{
  this->UpperThreshold = VTK_LARGE_FLOAT;
  this->LowerThreshold = -VTK_LARGE_FLOAT;
  this->ReplaceIn = 0;
  this->InValue = 0.0;
  this->ReplaceOut = 0;
  this->OutValue = 0.0;

  this->OutputScalarType = -1; // invalid; output same as input
}


//----------------------------------------------------------------------------
void vtkImageThreshold::SetInValue(float val)
{
  if (val != this->InValue || this->ReplaceIn != 1)
    {
    this->InValue = val;
    this->ReplaceIn = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageThreshold::SetOutValue(float val)
{
  if (val != this->OutValue || this->ReplaceOut != 1)
    {
    this->OutValue = val;
    this->ReplaceOut = 1;
    this->Modified();
    }
}



//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void vtkImageThreshold::ThresholdByUpper(float thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < VTK_LARGE_FLOAT)
    {
    this->LowerThreshold = thresh;
    this->UpperThreshold = VTK_LARGE_FLOAT;
    this->Modified();
    }
}


//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void vtkImageThreshold::ThresholdByLower(float thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -VTK_LARGE_FLOAT)
    {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -VTK_LARGE_FLOAT;
    this->Modified();
    }
}


//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void vtkImageThreshold::ThresholdBetween(float lower, float upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageThreshold::ExecuteInformation(vtkImageData *inData, 
                                           vtkImageData *outData)
{
  if (this->OutputScalarType != -1)
    {
    outData->SetScalarType(this->OutputScalarType);
    }
  else
    {
    outData->SetScalarType(inData->GetScalarType());
    }
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageThresholdExecute(vtkImageThreshold *self,
                                     vtkImageData *inData, IT *inPtr,
                                     vtkImageData *outData, OT *outPtr, 
                                     int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  float  lowerThreshold = self->GetLowerThreshold();
  float  upperThreshold = self->GetUpperThreshold();
  int replaceIn = self->GetReplaceIn();
  OT  inValue = (OT)(self->GetInValue());
  int replaceOut = self->GetReplaceOut();
  OT  outValue = (OT)(self->GetOutValue());
  float temp;
  unsigned long count = 0;
  unsigned long target;
  
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
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
        temp = (float)(*inPtr);
        if (lowerThreshold <= temp && temp <= upperThreshold)
          {
          // match
          if (replaceIn)
            {
            *outPtr = inValue;
            }
          else
            {
            *outPtr = (OT)(temp);
            }
          }
        else
          {
          // not match
          if (replaceOut)
            {
            *outPtr = outValue;
            }
          else
            {
            *outPtr = (OT)(temp);
            }
          }
        outPtr++;
        inPtr++;
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
static void vtkImageThresholdExecute1(vtkImageThreshold *self,
                                      vtkImageData *inData, T *inPtr,
                                      vtkImageData *outData,
                                      int outExt[6], int id)
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageThresholdExecute, self, inData, inPtr,
                      outData, (VTK_TT *)(outPtr),outExt, id);
    default:
      vtkGenericWarningMacro("Execute: Unknown input ScalarType");
      return;
    }
}



//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageThreshold::ThreadedExecute(vtkImageData *inData, 
                                        vtkImageData *outData,
                                        int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageThresholdExecute1, this, inData, 
                      (VTK_TT *)(inPtr), outData, outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "ReplaceIn: " << this->ReplaceIn << "\n";
  os << indent << "ReplaceOut: " << this->ReplaceOut << "\n";
}

