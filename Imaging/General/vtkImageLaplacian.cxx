/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLaplacian.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageLaplacian.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkImageLaplacian);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageLaplacian fitler.
vtkImageLaplacian::vtkImageLaplacian()
{
  this->Dimensionality = 2;
}

//----------------------------------------------------------------------------
void vtkImageLaplacian::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensionality: " << this->Dimensionality;
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
int vtkImageLaplacian::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int idx;
  int wholeExtent[6], inUExt[6];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inUExt);

  // update and Clip
  for (idx = 0; idx < 3; ++idx)
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
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inUExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void vtkImageLaplacianExecute(vtkImageLaplacian *self,
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
  int axesNum;
  int *wholeExtent;
  vtkIdType *inIncs;
  double r[3], d, sum;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = static_cast<unsigned long>((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // The data spacing is important for computing the Laplacian.
  // Divided by dx twice (second derivative).
  inData->GetSpacing(r);
  r[0] = 1.0 / (r[0] * r[0]);
  r[1] = 1.0 / (r[1] * r[1]);
  r[2] = 1.0 / (r[2] * r[2]);

  // get some other info we need
  inIncs = inData->GetIncrements();
  wholeExtent = inData->GetExtent();

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    useZMin = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useZMax = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
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
      useYMin = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useYMax = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
      for (idxX = 0; idxX <= maxX; idxX++)
      {
        useXMin = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
        useXMax = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];
        for (idxC = 0; idxC < maxC; idxC++)
        {
          // do X axis
          d = -2.0*(*inPtr);
          d += static_cast<double>(inPtr[useXMin]);
          d += static_cast<double>(inPtr[useXMax]);
          sum = d * r[0];

          // do y axis
          d = -2.0*(*inPtr);
          d += static_cast<double>(inPtr[useYMin]);
          d += static_cast<double>(inPtr[useYMax]);
          sum = sum + d * r[1];
          if (axesNum == 3)
          {
            // do z axis
            d = -2.0*(*inPtr);
            d += static_cast<double>(inPtr[useZMin]);
            d += static_cast<double>(inPtr[useZMax]);
            sum = sum + d * r[2];
          }
          *outPtr = static_cast<T>(sum);
          inPtr++;
          outPtr++;
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
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageLaplacian::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() !=
      outData[0]->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, "
      << inData[0][0]->GetScalarType() << ", must match out ScalarType "
      << outData[0]->GetScalarType());
    return;
  }

  switch (inData[0][0]->GetScalarType())
  {
    vtkTemplateMacro(
      vtkImageLaplacianExecute( this, inData[0][0],
                                static_cast<VTK_TT *>(inPtr), outData[0],
                                static_cast<VTK_TT *>(outPtr),
                                outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}

