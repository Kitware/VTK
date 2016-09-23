/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageNonMaximumSuppression.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

#include <cmath>

vtkStandardNewMacro(vtkImageNonMaximumSuppression);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageNonMaximumSuppression fitler.
vtkImageNonMaximumSuppression::vtkImageNonMaximumSuppression()
{
  this->Dimensionality= 2;
  this->HandleBoundaries = 1;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
int vtkImageNonMaximumSuppression::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int extent[6];
  int idx;

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);
  if ( ! this->HandleBoundaries)
  {
    // shrink output image extent.
    for (idx = 0; idx < this->Dimensionality; ++idx)
    {
      extent[idx*2] += 1;
      extent[idx*2+1] -= 1;
    }
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent,6);

  return 1;
}

//----------------------------------------------------------------------------
// This method computes the input extent necessary to generate the output.
int vtkImageNonMaximumSuppression::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);

  int *wholeExtent;
  int idx;

  // get the whole image for input 2
  int inExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt);
  wholeExtent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  inInfo2->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);

  // grow input image extent for input 0
  for (idx = 0; idx < this->Dimensionality; ++idx)
  {
    inExt[idx*2] -= 1;
    inExt[idx*2+1] += 1;
    if (this->HandleBoundaries)
    {
      // we must clip extent with whole extent if we hanlde boundaries.
      if (inExt[idx*2] < wholeExtent[idx*2])
      {
        inExt[idx*2] = wholeExtent[idx*2];
      }
      if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
        inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
  }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);

  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
void vtkImageNonMaximumSuppressionExecute(vtkImageNonMaximumSuppression *self,
                                          vtkImageData *in1Data, T *in1Ptr,
                                          vtkImageData *in2Data,
                                          T *in2Ptr,
                                          vtkImageData *outData,
                                          T *outPtr,
                                          int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType in2IncX, in2IncY, in2IncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;
  double d, normalizeFactor, vector[3], *ratio;
  int neighborA, neighborB;
  int *wholeExtent;
  vtkIdType *inIncs;
  int axesNum;

  vector[0] = 0.0;
  vector[1] = 0.0;
  vector[2] = 0.0;

  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();
  // get some other info we need
  inIncs = in1Data->GetIncrements();
  wholeExtent = in1Data->GetExtent();

  // Get increments to march through data
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);


  // Gradient is computed with data spacing (world coordinates)
  ratio = in2Data->GetSpacing();

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    useZMin = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useZMax = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
    {
      useYMin = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useYMax = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      if (!id)
      {
        if (!(count%target))
        {
          self->UpdateProgress(count/(50.0*target));
        }
        count++;
      }
      for (idxX = 0; idxX <= maxX; idxX++)
      {
        useXMin = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
        useXMax = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];

        // calculate the neighbors
        d = vector[0] = static_cast<double>(*in2Ptr) * ratio[0];
        normalizeFactor = (d * d);
        d = vector[1] = static_cast<double>(in2Ptr[1]) * ratio[1];
        normalizeFactor += (d * d);
        if (axesNum == 3)
        {
          d = vector[2] = static_cast<double>(in2Ptr[2]) * ratio[2];
          normalizeFactor += (d * d);
        }
        if (normalizeFactor != 0.0)
        {
          normalizeFactor = 1.0 / sqrt(normalizeFactor);
        }
        // Vector points positive along this idx?
        // (can point along multiple axes)
        d = vector[0] * normalizeFactor;

        if (d > 0.5)
        {
          neighborA = useXMax;
          neighborB = useXMin;
        }
        else if (d < -0.5)
        {
          neighborB = useXMax;
          neighborA = useXMin;
        }
        else
        {
          neighborA = 0;
          neighborB = 0;
        }
        d = vector[1] * normalizeFactor;
        if (d > 0.5)
        {
          neighborA += useYMax;
          neighborB += useYMin;
        }
        else if (d < -0.5)
        {
          neighborB += useYMax;
          neighborA += useYMin;
        }
        if (axesNum == 3)
        {
          d = vector[2] * normalizeFactor;
          if (d > 0.5)
          {
            neighborA += useZMax;
            neighborB += useZMin;
          }
          else if (d < -0.5)
          {
            neighborB += useZMax;
            neighborA += useZMin;
          }
        }

        // now process the components
        for (idxC = 0; idxC < maxC; idxC++)
        {
          // Pixel operation
          // Set Output Magnitude
          if (in1Ptr[neighborA] > *in1Ptr || in1Ptr[neighborB] > *in1Ptr)
          {
            *outPtr = 0;
          }
          else
          {
            *outPtr = *in1Ptr;
            // also check for them being equal is neighbor with larger ptr
            if ((neighborA > neighborB)&&(in1Ptr[neighborA] == *in1Ptr))
            {
              *outPtr = 0;
            }
            else if ((neighborB > neighborA)&&(in1Ptr[neighborB] == *in1Ptr))
            {
              *outPtr = 0;
            }
          }
          outPtr++;
          in1Ptr++;
        }
        in2Ptr += axesNum;
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
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageNonMaximumSuppression::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *in1Ptr;
  void *in2Ptr;
  void *outPtr;

  if (id == 0)
  {
    if (outData[0]->GetPointData()->GetScalars())
    {
      outData[0]->GetPointData()->GetScalars()->SetName("SuppressedMaximum");
    }
  }

  in1Ptr = inData[0][0]->GetScalarPointerForExtent(outExt);
  in2Ptr = inData[1][0]->GetScalarPointerForExtent(outExt);
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType() ||
      inData[1][0]->GetScalarType() != outData[0]->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, " <<
    inData[0][0]->GetScalarType()
    << ", must match out ScalarType " << outData[0]->GetScalarType());
    return;
  }

  switch (inData[0][0]->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageNonMaximumSuppressionExecute(this, inData[0][0],
                                           static_cast<VTK_TT *>(in1Ptr),
                                           inData[1][0],
                                           static_cast<VTK_TT *>(in2Ptr),
                                           outData[0],
                                           static_cast<VTK_TT *>(outPtr),
                                           outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

void vtkImageNonMaximumSuppression::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";

  os << indent << "HandleBoundaries: " << (this->HandleBoundaries ? "On\n" : "Off\n");
}
