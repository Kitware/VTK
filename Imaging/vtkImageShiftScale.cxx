/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShiftScale.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageShiftScale.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageShiftScale, "1.41");
vtkStandardNewMacro(vtkImageShiftScale);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageShiftScale::vtkImageShiftScale()
{
  this->Shift = 0.0;
  this->Scale = 1.0;
  this->OutputScalarType = -1;
  this->ClampOverflow = 0;
}



//----------------------------------------------------------------------------
void vtkImageShiftScale::ExecuteInformation(vtkImageData *inData, 
                                            vtkImageData *outData)
{
  this->vtkImageToImageFilter::ExecuteInformation( inData, outData );

  if (this->OutputScalarType != -1)
    {
    outData->SetScalarType(this->OutputScalarType);
    }
}




//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageShiftScaleExecute(vtkImageShiftScale *self,
                                      vtkImageData *inData, IT *inPtr,
                                      vtkImageData *outData, OT *outPtr,
                                      int outExt[6], int id)
{
  double typeMin, typeMax, val;
  int clamp;
  float shift = self->GetShift();
  float scale = self->GetScale();
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
          val = ((float)(*inPtr) + shift) * scale;
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
          *outPtr = (OT)(((float)(*inPtr) + shift) * scale);
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
static void vtkImageShiftScaleExecute1(vtkImageShiftScale *self,
                                      vtkImageData *inData, T *inPtr,
                                      vtkImageData *outData,
                                      int outExt[6], int id)
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageShiftScaleExecute, self, inData, inPtr,
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
void vtkImageShiftScale::ThreadedExecute(vtkImageData *inData, 
                                         vtkImageData *outData,
                                         int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageShiftScaleExecute1, this, 
                      inData, (VTK_TT *)(inPtr), outData, outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



void vtkImageShiftScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Shift: " << this->Shift << "\n";
  os << indent << "Scale: " << this->Scale << "\n";
  os << indent << "Output Scalar Type: " << this->OutputScalarType << "\n";
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

