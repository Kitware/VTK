/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageInPlaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageInPlaceFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLargeInteger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"


//----------------------------------------------------------------------------
vtkImageInPlaceFilter::vtkImageInPlaceFilter()
{
}

//----------------------------------------------------------------------------
vtkImageInPlaceFilter::~vtkImageInPlaceFilter()
{
}

//----------------------------------------------------------------------------

int vtkImageInPlaceFilter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output =
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *input =
    vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  int *inExt, *outExt;
  inExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  outExt = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // if the total size of the data is the same then can be in place
  vtkLargeInteger inSize;
  vtkLargeInteger outSize;
  inSize = (inExt[1] - inExt[0] + 1);
  inSize = inSize * (inExt[3] - inExt[2] + 1);
  inSize = inSize * (inExt[5] - inExt[4] + 1);
  outSize = (outExt[1] - outExt[0] + 1);
  outSize = outSize * (outExt[3] - outExt[2] + 1);
  outSize = outSize * (outExt[5] - outExt[4] + 1);
  if (inSize == outSize &&
      (vtkDataObject::GetGlobalReleaseDataFlag() ||
       inInfo->Get(vtkDemandDrivenPipeline::RELEASE_DATA())))
    {
    // pass the data
    output->GetPointData()->PassData(input->GetPointData());
    output->SetExtent(outExt);
    }
  else
    {
    output->SetExtent(outExt);
    output->AllocateScalars(outInfo);
    this->CopyData(input,output,outExt);
    }

  return 1;
}

void vtkImageInPlaceFilter::CopyData(vtkImageData *inData,
                                     vtkImageData *outData,
                                     int* outExt)
{
  char *inPtr = static_cast<char *>(inData->GetScalarPointerForExtent(outExt));
  char *outPtr =
    static_cast<char *>(outData->GetScalarPointerForExtent(outExt));
  int rowLength, size;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;

  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // adjust increments for this loop
  inIncY = inIncY*size + rowLength;
  outIncY = outIncY*size + rowLength;
  inIncZ *= size;
  outIncZ *= size;

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr,rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

//----------------------------------------------------------------------------
void vtkImageInPlaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
