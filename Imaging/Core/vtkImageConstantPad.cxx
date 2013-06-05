/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConstantPad.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageConstantPad.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageConstantPad);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageConstantPad::vtkImageConstantPad()
{
  this->Constant = 0.0;
}



//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageConstantPadExecute(vtkImageConstantPad *self,
                                vtkImageData *inData, T *inPtr,
                                vtkImageData *outData, T *outPtr,
                                int outExt[6], int inExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  T constant;
  int inMinX, inMaxX, inMaxC;
  constant = static_cast<T>(self->GetConstant());
  int state0, state1, state2, state3;
  unsigned long count = 0;
  unsigned long target;

  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  inMaxC = inData->GetNumberOfScalarComponents();
  inMinX = inExt[0] - outExt[0];
  inMaxX = inExt[1] - outExt[0];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through output pixels
  for (idxZ = outExt[4]; idxZ <= outExt[5]; idxZ++)
    {
    state3 = (idxZ < inExt[4] || idxZ > inExt[5]);
    for (idxY = outExt[2]; !self->AbortExecute && idxY <= outExt[3]; idxY++)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      state2 = (state3 || idxY < inExt[2] || idxY > inExt[3]);
      if ((maxC == inMaxC) && (maxC == 1))
        {
        for (idxX = 0; idxX <= maxX; idxX++)
          {
          state1 = (state2 || idxX < inMinX || idxX > inMaxX);
          if (state1)
            {
            *outPtr = constant;
            }
          else
            {
            *outPtr = *inPtr;
            inPtr++;
            }
          outPtr++;
          }
        }
      else
        {
        for (idxX = 0; idxX <= maxX; idxX++)
          {
          state1 = (state2 || idxX < inMinX || idxX > inMaxX);
          for (idxC = 0; idxC < maxC; idxC++)
            {
            // Pixel operation
            // Copy Pixel
            state0 = (state1 || idxC >= inMaxC);
            if (state0)
              {
              *outPtr = constant;
              }
            else
              {
              *outPtr = *inPtr;
              inPtr++;
              }
            outPtr++;
            }
          }
        }
      outPtr += outIncY;
      if (!state2)
        {
        inPtr += inIncY;
        }
      }
    outPtr += outIncZ;
    if (!state3)
      {
      inPtr += inIncZ;
      }
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageConstantPad::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

  // get the whole extent
  int wExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wExt);

  // need to get the correct pointer for the input data
  int inExt[6];
  this->ComputeInputUpdateExtent(inExt,outExt,wExt);
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageConstantPadExecute(this,
                                 inData[0][0], static_cast<VTK_TT *>(inPtr),
                                 outData[0], static_cast<VTK_TT *>(outPtr),
                                 outExt, inExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageConstantPad::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Constant: " << this->Constant << "\n";

}

