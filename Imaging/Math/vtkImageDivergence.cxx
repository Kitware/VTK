/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDivergence.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDivergence.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageDivergence);

vtkImageDivergence::vtkImageDivergence()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
// This method tells the superclass that the first axis will collapse.
int vtkImageDivergence::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkDataObject::SetPointDataActiveScalarInfo(
    outputVector->GetInformationObject(0), -1, 1);
  return 1;
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
int vtkImageDivergence::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int idx;
  int wholeExtent[6];

  vtkInformation *inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!inScalarInfo)
    {
    vtkErrorMacro("Missing scalar field on input information!");
    return 0;
    }

  int dimensionality =
    inScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());

  if (dimensionality > 3)
    {
    vtkErrorMacro("Divergence has to have dimensionality <= 3");
    dimensionality = 3;
    }

  // handle XYZ
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent);
  int inUExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inUExt);

  // update and Clip
  for (idx = 0; idx < dimensionality; ++idx)
    {
    --inUExt[idx*2];
    ++inUExt[idx*2+1];
    if (inUExt[idx*2] < wholeExtent[idx*2])
      {
      inUExt[idx*2] = wholeExtent[idx*2];
      }
    if (inUExt[idx*2] > wholeExtent[idx*2 + 1])
      {
      inUExt[idx*2] = wholeExtent[idx*2 + 1];
      }
    if (inUExt[idx*2+1] < wholeExtent[idx*2])
      {
      inUExt[idx*2+1] = wholeExtent[idx*2];
      }
    if (inUExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
      {
      inUExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
      }
    }
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inUExt,6);

  return 1;
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void vtkImageDivergenceExecute(vtkImageDivergence *self,
                               vtkImageData *inData, T *inPtr,
                               vtkImageData *outData, T *outPtr,
                               int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int *wholeExtent;
  vtkIdType *inIncs;
  double r[3], d, sum;
  int useMin[3], useMax[3];

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  if (maxC > 3)
    {
    vtkGenericWarningMacro("Dimensionality must be less than or equal to 3");
    maxC = 3;
    }
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // The spacing is important for computing the gradient.
  // central differences (2 * ratio).
  // Negative because below we have (min - max) for dx ...
  inData->GetSpacing(r);
  r[0] = -0.5 / r[0];
  r[1] = -0.5 / r[1];
  r[2] = -0.5 / r[2];

  // get some other info we need
  inIncs = inData->GetIncrements();
  wholeExtent = inData->GetExtent();

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useMin[2] = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useMax[2] = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
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
      useMin[1] = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useMax[1] = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        useMin[0] = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
        useMax[0] = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];
        sum = 0.0;
        for (idxC = 0; idxC < maxC; idxC++)
          {
          // do X axis
          d = static_cast<double>(inPtr[useMin[idxC]]);
          d -= static_cast<double>(inPtr[useMax[idxC]]);
          sum += d * r[idxC];
          inPtr++;
          }
        *outPtr = static_cast<T>(sum);
        outPtr++;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageDivergence::ThreadedExecute (vtkImageData *inData,
                                           vtkImageData *outData,
                                           int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData->GetScalarType()
                  << ", must match out ScalarType "
                  << outData->GetScalarType());
    return;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageDivergenceExecute(this, inData,
                                static_cast<VTK_TT *>(inPtr), outData,
                                static_cast<VTK_TT *>(outPtr),
                                outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

