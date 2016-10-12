/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCheckerboard.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCheckerboard.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageCheckerboard);

//----------------------------------------------------------------------------
vtkImageCheckerboard::vtkImageCheckerboard()
{
  this->NumberOfDivisions[0] = 2;
  this->NumberOfDivisions[1] = 2;
  this->NumberOfDivisions[2] = 2;

  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void vtkImageCheckerboardExecute2(vtkImageCheckerboard *self,
                                  vtkImageData *in1Data, T *in1Ptr,
                                  vtkImageData *in2Data, T *in2Ptr,
                                  vtkImageData *outData,
                                  T *outPtr,
                                  int outExt[6], int id,
                                  int wholeExt[6])
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int dimWholeX, dimWholeY, dimWholeZ;
  int divX, divY, divZ;
  int nComp;
  int selectX, selectY, selectZ;
  int which;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType in2IncX, in2IncY, in2IncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int threadOffsetX, threadOffsetY, threadOffsetZ;
  int numDivX, numDivY, numDivZ;

  // find the region to loop over
  nComp = in1Data->GetNumberOfScalarComponents();
  rowLength = (outExt[1] - outExt[0]+1)*nComp;
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  dimWholeX = wholeExt[1] - wholeExt[0] + 1;
  dimWholeY = wholeExt[3] - wholeExt[2] + 1;
  dimWholeZ = wholeExt[5] - wholeExt[4] + 1;

  threadOffsetX = (outExt[0] - wholeExt[0]) * nComp;
  threadOffsetY = outExt[2] - wholeExt[2];
  threadOffsetZ = outExt[4] - wholeExt[4];

  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // make sure number of divisions is not 0
  numDivX = (self->GetNumberOfDivisions()[0] == 0) ? 1 : self->GetNumberOfDivisions()[0];
  numDivY = (self->GetNumberOfDivisions()[1] == 0) ? 1 : self->GetNumberOfDivisions()[1];
  numDivZ = (self->GetNumberOfDivisions()[2] == 0) ? 1 : self->GetNumberOfDivisions()[2];

  divX = dimWholeX / numDivX * nComp;
  divY = dimWholeY / numDivY;
  divZ = dimWholeZ / numDivZ;

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    selectZ = (((idxZ + threadOffsetZ) / divZ) % 2) << 2;
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
      selectY = (((idxY + threadOffsetY) / divY) % 2) << 1;
      for (idxR = 0; idxR < rowLength; idxR++)
      {

        selectX = ((idxR + threadOffsetX) / divX) % 2;
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
void vtkImageCheckerboard::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector * outputVector,
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *in1Ptr, *in2Ptr;
  void *outPtr;

  if (inData[0][0] == NULL)
  {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
  }
  in1Ptr = inData[0][0]->GetScalarPointerForExtent(outExt);
  if (!in1Ptr)
  {
    vtkErrorMacro(<< "Input " << 0 << " cannot be empty.");
    return;
  }

  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  if (inData[1][0] == NULL)
  {
    vtkErrorMacro(<< "Input " << 1 << " must be specified.");
    return;
  }
  in2Ptr = inData[1][0]->GetScalarPointerForExtent(outExt);
  if (!in2Ptr)
  {
    vtkErrorMacro(<< "Input " << 1 << " cannot be empty.");
    return;
  }

  // this filter expects that inputs that have the same number of components
  if (inData[0][0]->GetNumberOfScalarComponents() !=
      inData[1][0]->GetNumberOfScalarComponents())
  {
    vtkErrorMacro(<< "Execute: input1 NumberOfScalarComponents, "
                  << inData[0][0]->GetNumberOfScalarComponents()
                  << ", must match out input2 NumberOfScalarComponents "
                  << inData[1][0]->GetNumberOfScalarComponents());
    return;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int wholeExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  switch (inData[0][0]->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageCheckerboardExecute2(this, inData[0][0],
                                   static_cast<VTK_TT *>(in1Ptr),
                                   inData[1][0],
                                   static_cast<VTK_TT *>(in2Ptr),
                                   outData[0],
                                   static_cast<VTK_TT *>(outPtr),
                                   outExt, id, wholeExtent));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageCheckerboard::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfDivisions: (" << this->NumberOfDivisions[0] << ", "
     << this->NumberOfDivisions[1] << ", "
     << this->NumberOfDivisions[2] << ")\n";
}

