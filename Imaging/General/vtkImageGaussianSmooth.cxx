/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageGaussianSmooth.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageGaussianSmooth);

//----------------------------------------------------------------------------
vtkImageGaussianSmooth::vtkImageGaussianSmooth()
{
  this->Dimensionality = 3; // note: this overrides Standard deviation.
  this->StandardDeviations[0] = 2.0;
  this->StandardDeviations[1] = 2.0;
  this->StandardDeviations[2] = 2.0;
  this->RadiusFactors[0] = 1.5;
  this->RadiusFactors[1] = 1.5;
  this->RadiusFactors[2] = 1.5;
}

//----------------------------------------------------------------------------
vtkImageGaussianSmooth::~vtkImageGaussianSmooth()
{
}

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // int idx;

  //os << indent << "BoundaryRescale: " << this->BoundaryRescale << "\n";

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";

  os << indent << "RadiusFactors: ( "
     << this->RadiusFactors[0] << ", "
     << this->RadiusFactors[1] << ", "
     << this->RadiusFactors[2] << " )\n";

  os << indent << "StandardDeviations: ( "
     << this->StandardDeviations[0] << ", "
     << this->StandardDeviations[1] << ", "
     << this->StandardDeviations[2] << " )\n";
}

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::ComputeKernel(double *kernel, int min, int max,
                                           double std)
{
  int x;
  double sum;

  // handle special case
  if (std == 0.0)
    {
    kernel[0] = 1.0;
    return;
    }

  // fill in kernel
  sum = 0.0;
  for (x = min; x <= max; ++x)
    {
    sum += kernel[x-min] =
      exp(- (static_cast<double>(x*x)) / (std * std * 2.0));
    }

  // normalize
  for (x = min; x <= max; ++x)
    {
    kernel[x-min] /= sum;
    }
}

//----------------------------------------------------------------------------
int vtkImageGaussianSmooth::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int wholeExtent[6], inExt[6];

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);

  // Expand filtered axes
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  this->InternalRequestUpdateExtent(inExt, wholeExtent);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::InternalRequestUpdateExtent(int *inExt,
                                                         int *wholeExtent)
{
  int idx, radius;

  // Expand filtered axes
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    radius = static_cast<int>(this->StandardDeviations[idx]
                              * this->RadiusFactors[idx]);
    inExt[idx*2] -= radius;
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }

    inExt[idx*2+1] += radius;
    if (inExt[idx*2+1] > wholeExtent[idx*2+1])
      {
      inExt[idx*2+1] = wholeExtent[idx*2+1];
      }
    }
}

//----------------------------------------------------------------------------
// For a given position along the convolution axis, this method loops over
// all other axes, and performs the convolution. Boundary conditions handled
// previously.
template <class T>
void
vtkImageGaussianSmoothExecute(vtkImageGaussianSmooth *self, int axis,
                              double *kernel, int kernelSize,
                              vtkImageData *inData, T *inPtrC,
                              vtkImageData *outData, int outExt[6],
                              T *outPtrC, int *pcycle, int target,
                              int *pcount, int total)
{
  int maxC, max0 = 0, max1 = 0;
  int idxC, idx0, idx1, idxK;
  vtkIdType *inIncs, *outIncs;
  vtkIdType inInc0 = 0, inInc1 = 0, inIncK, outInc0 = 0, outInc1 = 0;
  T *outPtr1, *outPtr0;
  T *inPtr1, *inPtr0, *inPtrK;
  double *ptrK, sum;

  // I am counting on the fact that tight loops (component on outside)
  // is more important than cache misses from shuffled access.

  // Do the correct shuffling of the axes (increments, extents)
  inIncs = inData->GetIncrements();
  outIncs = outData->GetIncrements();
  inIncK = inIncs[axis];
  maxC = outData->GetNumberOfScalarComponents();
  switch (axis)
    {
    case 0:
      inInc0 = inIncs[1];  inInc1 = inIncs[2];
      outInc0 = outIncs[1];  outInc1 = outIncs[2];
      max0 = outExt[3] - outExt[2] + 1;   max1 = outExt[5] - outExt[4] + 1;
      break;
    case 1:
      inInc0 = inIncs[0];  inInc1 = inIncs[2];
      outInc0 = outIncs[0];  outInc1 = outIncs[2];
      max0 = outExt[1] - outExt[0] + 1;   max1 = outExt[5] - outExt[4] + 1;
      break;
    case 2:
      inInc0 = inIncs[0];  inInc1 = inIncs[1];
      outInc0 = outIncs[0];  outInc1 = outIncs[1];
      max0 = outExt[1] - outExt[0] + 1;   max1 = outExt[3] - outExt[2] + 1;
      break;
    }

  for (idxC = 0; idxC < maxC; ++idxC)
    {
    inPtr1 = inPtrC;
    outPtr1 = outPtrC;
    for (idx1 = 0; !self->AbortExecute && idx1 < max1; ++idx1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;
      for (idx0 = 0; idx0 < max0; ++idx0)
        {
        inPtrK = inPtr0;
        ptrK = kernel;
        sum = 0.0;
        // too bad this short loop has to be the inner most loop
        for (idxK = 0; idxK < kernelSize; ++idxK)
          {
          sum += *ptrK * static_cast<double>(*inPtrK);
          ++ptrK;
          inPtrK += inIncK;
          }
        *outPtr0 = static_cast<T>(sum);
        inPtr0 += inInc0;
        outPtr0 += outInc0;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      // we finished a row ... do we update ???
      if (total)
        { // yes this is the main thread
        *pcycle += max0;
        if (*pcycle > target)
          { // yes
          *pcycle -= target;
          *pcount += target;
          self->UpdateProgress(static_cast<double>(*pcount) /
                               static_cast<double>(total));
          //fprintf(stderr, "count: %d, total: %d, progress: %f\n",
          //*pcount, total, (double)(*pcount) / (double)total);
          }
        }
      }

    ++inPtrC;
    ++outPtrC;
    }
}

//----------------------------------------------------------------------------
template <class T>
size_t vtkImageGaussianSmoothGetTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
// This method convolves over one axis. It loops over the convolved axis,
// and handles boundary conditions.
void vtkImageGaussianSmooth::ExecuteAxis(int axis,
                                         vtkImageData *inData, int inExt[6],
                                         vtkImageData *outData, int outExt[6],
                                         int *pcycle, int target,
                                         int *pcount, int total,
                                         vtkInformation *inInfo)
{
  int idxA, max;
  int wholeExtent[6], wholeMax, wholeMin;
  double *kernel;
  // previousClip and currentClip rembers that the previous was not clipped
  // keeps from recomputing kernels for center pixels.
  int kernelSize = 0;
  int kernelLeftClip, kernelRightClip;
  int previousClipped, currentClipped;
  int radius, size;
  void *inPtr;
  void *outPtr;
  int coords[3];
  vtkIdType *outIncs, outIncA;

  // Get the correct starting pointer of the output
  outPtr = outData->GetScalarPointerForExtent(outExt);
  outIncs = outData->GetIncrements();
  outIncA = outIncs[axis];

  // trick to account for the scalar type of the output(used to be only float)
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(
      outIncA *= vtkImageGaussianSmoothGetTypeSize(static_cast<VTK_TT*>(0))
      );
    default:
      vtkErrorMacro("Unknown scalar type");
      return;
    }

  // Determine default starting position of input
  coords[0] = inExt[0];
  coords[1] = inExt[2];
  coords[2] = inExt[4];

  // get whole extent for boundary checking ...
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  wholeMin = wholeExtent[axis*2];
  wholeMax = wholeExtent[axis*2+1];

  // allocate memory for the kernel
  radius = static_cast<int>(this->StandardDeviations[axis]
                            * this->RadiusFactors[axis]);
  size = 2*radius + 1;
  kernel = new double[size];

  // loop over the convolution axis
  previousClipped = currentClipped = 1;
  max = outExt[axis*2+1];
  for (idxA = outExt[axis*2]; idxA <= max; ++idxA)
    {
    // left boundary condition
    coords[axis] = idxA - radius;
    kernelLeftClip = wholeMin - coords[axis];
    if (kernelLeftClip > 0)
      { // front of kernel is cut off ("kernelStart" samples)
      coords[axis] += kernelLeftClip;
      }
    else
      {
      kernelLeftClip = 0;
      }
    // Right boundary condition
    kernelRightClip = (idxA + radius) - wholeMax;
    if (kernelRightClip < 0)
      {
      kernelRightClip = 0;
      }

    // We can only use previous kernel if it is not clipped and new
    // kernel is also not clipped.
    currentClipped = kernelLeftClip + kernelRightClip;
    if (currentClipped || previousClipped)
      {
      this->ComputeKernel(kernel, -radius+kernelLeftClip,
                          radius-kernelRightClip,
                          static_cast<double>(this->StandardDeviations[axis]));
      kernelSize = size - kernelLeftClip - kernelRightClip;
      }
    previousClipped = currentClipped;

    /* now do the convolution on the rest of the axes */
    inPtr = inData->GetScalarPointer(coords);
    switch (inData->GetScalarType())
      {
      vtkTemplateMacro(
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                                      inData, static_cast<VTK_TT*>(inPtr),
                                      outData, outExt,
                                      static_cast<VTK_TT*>(outPtr),
                                      pcycle, target, pcount, total)
        );
      default:
        vtkErrorMacro("Unknown scalar type");
        return;
      }
    outPtr = static_cast<void *>(
      static_cast<unsigned char *>(outPtr) + outIncA);
    }

  // get rid of temporary kernel
  delete [] kernel;
}

//----------------------------------------------------------------------------
// This method decomposes the gaussian and smooths along each axis.
void vtkImageGaussianSmooth::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector,
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int inExt[6];
  int target, count, total, cycle;

  // for feed back, determine line target to get 50 progress update
  // update is called every target lines. Progress is computed from
  // the number of pixels processed so far.
  count = 0; target = 0; total = 0; cycle = 0;
  if (id == 0)
    {
    // determine the number of pixels.
    total = this->Dimensionality * (outExt[1] - outExt[0] + 1)
      * (outExt[3] - outExt[2] + 1) * (outExt[5] - outExt[4] + 1)
      * inData[0][0]->GetNumberOfScalarComponents();
    // pixels per update (50 updates)
    target = total / 50;
    }

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro("Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

  // Decompose
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int wholeExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);
  this->InternalRequestUpdateExtent(inExt, wholeExt);

  switch (this->Dimensionality)
    {
    case 1:
      this->ExecuteAxis(0, inData[0][0], inExt, outData[0], outExt,
                        &cycle, target, &count, total, inInfo);
      break;
    case 2:
      int tempExt[6];
      vtkImageData *tempData;
      // compute intermediate extent
      tempExt[0] = inExt[0];  tempExt[1] = inExt[1];
      tempExt[2] = outExt[2];  tempExt[3] = outExt[3];
      tempExt[4] = inExt[4];  tempExt[5] = inExt[5];
      // create a temp data for intermediate results
      tempData = vtkImageData::New();
      tempData->SetExtent(tempExt);
      tempData->AllocateScalars(inData[0][0]->GetScalarType(),
                                inData[0][0]->GetNumberOfScalarComponents());
      this->ExecuteAxis(1, inData[0][0], inExt, tempData, tempExt,
                        &cycle, target, &count, total, inInfo);
      this->ExecuteAxis(0, tempData, tempExt, outData[0], outExt,
                        &cycle, target, &count, total, inInfo);
      // release temporary data
      tempData->Delete();
      break;
    case 3:
      // we do z first because it is most likely smallest
      int temp0Ext[6], temp1Ext[6];
      vtkImageData *temp0Data, *temp1Data;
      // compute intermediate extents
      temp0Ext[0] = inExt[0];  temp0Ext[1] = inExt[1];
      temp0Ext[2] = inExt[2];  temp0Ext[3] = inExt[3];
      temp0Ext[4] = outExt[4];  temp0Ext[5] = outExt[5];

      temp1Ext[0] = inExt[0];  temp1Ext[1] = inExt[1];
      temp1Ext[2] = outExt[2];  temp1Ext[3] = outExt[3];
      temp1Ext[4] = outExt[4];  temp1Ext[5] = outExt[5];

      // create a temp data for intermediate results
      temp0Data = vtkImageData::New();
      temp0Data->SetExtent(temp0Ext);
      temp0Data->AllocateScalars(inData[0][0]->GetScalarType(),
                                 inData[0][0]->GetNumberOfScalarComponents());

      temp1Data = vtkImageData::New();
      temp1Data->SetExtent(temp1Ext);
      temp1Data->AllocateScalars(inData[0][0]->GetScalarType(),
                                 inData[0][0]->GetNumberOfScalarComponents());
      this->ExecuteAxis(2, inData[0][0], inExt, temp0Data, temp0Ext,
                        &cycle, target, &count, total, inInfo);
      this->ExecuteAxis(1, temp0Data, temp0Ext, temp1Data, temp1Ext,
                        &cycle, target, &count, total, inInfo);
      temp0Data->Delete();
      this->ExecuteAxis(0, temp1Data, temp1Ext, outData[0], outExt,
                        &cycle, target, &count, total, inInfo);
      temp1Data->Delete();
      break;
    }
}
