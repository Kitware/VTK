/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCorrelation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCorrelation.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageCorrelation);

//----------------------------------------------------------------------------
vtkImageCorrelation::vtkImageCorrelation()
{
  this->Dimensionality = 2;
  this->SetNumberOfInputPorts(2);
}


//----------------------------------------------------------------------------
// Grow the output image
int vtkImageCorrelation::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//----------------------------------------------------------------------------
// Grow
int vtkImageCorrelation::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo1 = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  // get the whole image for input 2
  int inWExt2[6];
  inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),inWExt2);
  inInfo2->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               inWExt2, 6);

  int inWExt1[6];
  inInfo1->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),inWExt1);

  // try to get all the data required to handle the boundaries
  // but limit to the whole extent
  int idx;
  int inUExt1[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inUExt1);

  for (idx = 0; idx < 3; idx++)
  {
    inUExt1[idx*2+1] = inUExt1[idx*2+1] +
      (inWExt2[idx*2+1] - inWExt2[idx*2]);

    // clip to whole extent
    if (inUExt1[idx*2+1] > inWExt1[idx*2+1])
    {
      inUExt1[idx*2+1] = inWExt1[idx*2+1];
    }
  }
  inInfo1->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               inUExt1, 6);

  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void vtkImageCorrelationExecute(vtkImageCorrelation *self,
                                vtkImageData *in1Data, T *in1Ptr,
                                vtkImageData *in2Data, T *in2Ptr,
                                vtkImageData *outData, float *outPtr,
                                int outExt[6], int id,
                                int in2Extent[6])
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  vtkIdType in1IncX, in1IncY, in1IncZ;
  vtkIdType in1CIncX, in1CIncY, in1CIncZ;
  vtkIdType in2IncX, in2IncY, in2IncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  T *in2Ptr2, *in1Ptr2;
  int kIdxX, kIdxY, kIdxZ;
  int xKernMax, yKernMax, zKernMax;
  int maxIX, maxIY, maxIZ;
  int *wExtent;

  // find the region to loop over
  maxC = in1Data->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data
  in1Data->GetContinuousIncrements(outExt, in1CIncX, in1CIncY, in1CIncZ);
  in1Data->GetIncrements(in1IncX, in1IncY, in1IncZ);
  in2Data->GetIncrements(in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // how far can we gon with input data
  // this may be farther that the outExt because of
  // subpieces etc.
  wExtent = in1Data->GetExtent();
  maxIZ = wExtent[5] - outExt[4];
  maxIY = wExtent[3] - outExt[2];
  maxIX = wExtent[1] - outExt[0];

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    // how much of kernel to use
    zKernMax = maxIZ - idxZ;
    if (zKernMax > in2Extent[5])
    {
      zKernMax = in2Extent[5];
    }
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
      yKernMax = maxIY - idxY;
      if (yKernMax > in2Extent[3])
      {
        yKernMax = in2Extent[3];
      }
      for (idxX = 0; idxX <= maxX; idxX++)
      {
        // determine the extent of input 1 that contributes to this pixel
        *outPtr = 0.0;
        xKernMax = maxIX - idxX;
        if (xKernMax > in2Extent[1])
        {
          xKernMax = in2Extent[1];
        }

        // sumation
        for (kIdxZ = 0; kIdxZ <= zKernMax; kIdxZ++)
        {
          for (kIdxY = 0; kIdxY <= yKernMax; kIdxY++)
          {
            in1Ptr2 = in1Ptr + kIdxY*in1IncY + kIdxZ*in1IncZ;
            in2Ptr2 = in2Ptr + kIdxY*in2IncY + kIdxZ*in2IncZ;
            for (kIdxX = 0; kIdxX <= xKernMax; kIdxX++)
            {
              for (idxC = 0; idxC < maxC; idxC++)
              {
                *outPtr = *outPtr +
                  static_cast<float>((*in1Ptr2) * (*in2Ptr2));
                in1Ptr2++;
                in2Ptr2++;
              }
            }
          }
        }
        in1Ptr += maxC;
        outPtr++;
      }
      in1Ptr += in1CIncY;
      outPtr += outIncY;
    }
    in1Ptr += in1CIncZ;
    outPtr += outIncZ;
  }
}


//----------------------------------------------------------------------------
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageCorrelation::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** inputVector ,
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int *in2Extent;
  void *in1Ptr;
  void *in2Ptr;
  float *outPtr;

  in2Extent = inputVector[1]->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  in1Ptr = inData[0][0]->GetScalarPointerForExtent(outExt);
  in2Ptr = inData[1][0]->GetScalarPointerForExtent(in2Extent);
  outPtr = static_cast<float *>(outData[0]->GetScalarPointerForExtent(outExt));

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != inData[1][0]->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, " <<
    inData[0][0]->GetScalarType() << " and input2 ScalarType " <<
    inData[1][0]->GetScalarType() << ", should match");
    return;
  }

  // input depths must match
  if (inData[0][0]->GetNumberOfScalarComponents() !=
      inData[1][0]->GetNumberOfScalarComponents())
  {
    vtkErrorMacro(<< "Execute: input depths must match");
    return;
  }

  switch (inData[0][0]->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageCorrelationExecute(this, inData[0][0],
                                 static_cast<VTK_TT *>(in1Ptr),
                                 inData[1][0],
                                 static_cast<VTK_TT *>(in2Ptr),
                                 outData[0], outPtr, outExt, id,
                                 in2Extent));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageCorrelation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

