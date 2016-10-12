/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageBlend.h"

#include "vtkAlgorithmOutput.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkImageBlend);

//----------------------------------------------------------------------------
vtkImageBlend::vtkImageBlend()
{
  this->Opacity = 0;
  this->OpacityArrayLength = 0;
  this->BlendMode = VTK_IMAGE_BLEND_MODE_NORMAL;
  this->CompoundThreshold = 0.0;
  this->DataWasPassed = 0;

  // we have the image inputs and the optional stencil input
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkImageBlend::~vtkImageBlend()
{
  delete [] this->Opacity;
  this->OpacityArrayLength = 0;
}

//----------------------------------------------------------------------------
void vtkImageBlend::ReplaceNthInputConnection(int idx,
                                              vtkAlgorithmOutput *input)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(0))
  {
    vtkErrorMacro("Attempt to replace connection idx " << idx
                  << " of input port " << 0 << ", which has only "
                  << this->GetNumberOfInputConnections(0)
                  << " connections.");
    return;
  }

  if (!input || !input->GetProducer())
  {
    vtkErrorMacro("Attempt to replace connection index " << idx
                  << " for input port " << 0 << " with " <<
                  (!input ? "a null input." : "an input with no producer."));
    return;
  }

  this->SetNthInputConnection(0, idx, input);
}

//----------------------------------------------------------------------------
// The default vtkImageAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the image inputs to
// go on the first port.
void vtkImageBlend::SetInputData(int idx, vtkDataObject *input)
{
  this->SetInputDataInternal(idx, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkImageBlend::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
  {
    return 0;
  }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
void vtkImageBlend::SetStencilConnection(vtkAlgorithmOutput *algOutput)
{
  this->SetInputConnection(1, algOutput);
}
//----------------------------------------------------------------------------
void vtkImageBlend::SetStencilData(vtkImageStencilData *stencil)
{
  this->SetInputDataInternal(1, stencil);
}


//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageBlend::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return 0;
  }
  return vtkImageStencilData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
void vtkImageBlend::SetOpacity(int idx, double opacity)
{
  int i;
  int newLength;
  double *newArray;

  if (opacity < 0.0)
  {
    opacity = 0.0;
  }
  if (opacity > 1.0)
  {
    opacity = 1.0;
  }

  if (idx >= this->OpacityArrayLength)
  {
    newLength = idx + 1;
    newArray = new double[newLength];
    for (i = 0; i < this->OpacityArrayLength; i++)
    {
      newArray[i] = this->Opacity[i];
    }
    for (; i < newLength; i++)
    {
      newArray[i] = 1.0;
    }
    delete [] this->Opacity;
    this->Opacity = newArray;
    this->OpacityArrayLength = newLength;
  }

  if (this->Opacity[idx] != opacity)
  {
    this->Opacity[idx] = opacity;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkImageBlend::GetOpacity(int idx)
{
  if (idx >= this->OpacityArrayLength)
  {
    return 1.0;
  }
  return this->Opacity[idx];
}

//----------------------------------------------------------------------------
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the
// extent of the output region.  After this method finishes, "region" should
// have the extent of the required input region.  The default method assumes
// the required input extent are the same as the output extent.
// Note: The splitting methods call this method with outRegion = inRegion.
void vtkImageBlend::InternalComputeInputUpdateExtent(int inExt[6],
                                                     int outExt[6],
                                                     int wholeExtent[6])
{
  memcpy(inExt,outExt,sizeof(int)*6);

  int i;

  // clip with the whole extent
  for (i = 0; i < 3; i++)
  {
    if (inExt[2*i] < wholeExtent[2*i])
    {
      inExt[2*i] = wholeExtent[2*i];
    }
    if (inExt[2*i+1] > wholeExtent[2*i+1])
    {
      inExt[2*i+1] = wholeExtent[2*i+1];
    }
  }
}

//----------------------------------------------------------------------------
int vtkImageBlend::RequestUpdateExtent(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // default input extent will be that of output extent
  int inExt[6];
  int *outExt =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  int whichInput;
  for (whichInput = 0; whichInput < this->GetNumberOfInputConnections(0);
       whichInput++)
  {
    int *inWextent;
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(whichInput);
    inWextent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    this->InternalComputeInputUpdateExtent(inExt, outExt, inWextent);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageBlend::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // check to see if we have more than one input
  if (this->GetNumberOfInputConnections(0) == 1)
  {
    vtkDebugMacro("RequestData: single input, passing data");

    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkImageData *outData = static_cast<vtkImageData *>(
      info->Get(vtkDataObject::DATA_OBJECT()));
    info = inputVector[0]->GetInformationObject(0);
    vtkImageData *inData =
      static_cast<vtkImageData*>(info->Get(vtkDataObject::DATA_OBJECT()));

    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    this->DataWasPassed = 1;
  }
  else // multiple inputs
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkImageData *outData = static_cast<vtkImageData *>(
      info->Get(vtkDataObject::DATA_OBJECT()));
    if (this->DataWasPassed)
    {
      outData->GetPointData()->SetScalars(NULL);
      this->DataWasPassed = 0;
    }
    return this->Superclass::RequestData(request,inputVector,outputVector);
  }

  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageBlendExecute(vtkImageBlend *self, int extent[6],
                          vtkImageData *inData, T *,
                          vtkImageData *outData, T *,
                          double opacity, int id)
{
  double minA, maxA;

  if (inData->GetScalarType() == VTK_DOUBLE ||
      inData->GetScalarType() == VTK_FLOAT)
  {
    minA = 0.0;
    maxA = 1.0;
  }
  else
  {
    minA = inData->GetScalarTypeMin();
    maxA = inData->GetScalarTypeMax();
  }

  double r = opacity;
  double f = 1.0 - r;

  opacity = opacity/(maxA-minA);

  int inC = inData->GetNumberOfScalarComponents();
  int outC = outData->GetNumberOfScalarComponents();

  vtkImageStencilData *stencil = self->GetStencil();
  vtkImageStencilIterator<T> outIter(outData, stencil, extent, self, id);
  vtkImageIterator<T> inIter(inData, extent);

  T *inPtr = inIter.BeginSpan();
  T *inSpanEndPtr = inIter.EndSpan();
  while (!outIter.IsAtEnd())
  {
    T* outPtr = outIter.BeginSpan();
    T* outSpanEndPtr = outIter.EndSpan();
    if (outIter.IsInStencil())
    {
      if (outC == 4 && inC == 4)
      { // RGBA blended with RGBA
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0; i < steps; i += 4)
        {
          double rLocal = opacity*(inPtr[i+3]-minA);
          double fLocal = 1.0-rLocal;
          outPtr[i+0] = T(outPtr[i+0]*fLocal + inPtr[i+0]*rLocal);
          outPtr[i+1] = T(outPtr[i+1]*fLocal + inPtr[i+1]*rLocal);
          outPtr[i+2] = T(outPtr[i+2]*fLocal + inPtr[i+2]*rLocal);
        }
        inPtr += inC * (steps / 4);
      }
      else if (outC >= 3 && inC >= 4)
      { // RGB(A) blended with RGBA
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          double rLocal = opacity*(inPtr[j+3]-minA);
          double fLocal = 1.0-rLocal;
          outPtr[i+0] = T(outPtr[i+0]*fLocal + inPtr[j+0]*rLocal);
          outPtr[i+1] = T(outPtr[i+1]*fLocal + inPtr[j+1]*rLocal);
          outPtr[i+2] = T(outPtr[i+2]*fLocal + inPtr[j+2]*rLocal);
        }
        inPtr += inC * (steps / outC);
      }
      else if (outC == 3 && inC == 3)
      { // RGB blended with RGB
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0; i < steps; i += 3)
        {
          outPtr[i+0] = T(outPtr[i+0]*f + inPtr[i+0]*r);
          outPtr[i+1] = T(outPtr[i+1]*f + inPtr[i+1]*r);
          outPtr[i+2] = T(outPtr[i+2]*f + inPtr[i+2]*r);
        }
        inPtr += inC * (steps / 3);
      }
      else if (outC >= 3 && inC == 3)
      { // RGB(A) blended with RGB
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          outPtr[i+0] = T(outPtr[i+0]*f + inPtr[j+0]*r);
          outPtr[i+1] = T(outPtr[i+1]*f + inPtr[j+1]*r);
          outPtr[i+2] = T(outPtr[i+2]*f + inPtr[j+2]*r);
        }
        inPtr += inC * (steps / outC);
      }
      else if (outC >= 3 && inC == 2)
      { // RGB(A) blended with luminance+alpha
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          double rLocal = opacity*(inPtr[j+1]-minA);
          double fLocal = 1.0-rLocal;
          outPtr[i+0] = T(outPtr[i+0]*fLocal + (inPtr[j])*rLocal);
          outPtr[i+1] = T(outPtr[i+1]*fLocal + (inPtr[j])*rLocal);
          outPtr[i+2] = T(outPtr[i+2]*fLocal + (inPtr[j])*rLocal);
        }
        inPtr += inC * (steps / outC);
      }
      else if (outC >= 3 && inC == 1)
      { // RGB(A) blended with luminance
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          outPtr[i+0] = T(outPtr[i+0]*f + (inPtr[j])*r);
          outPtr[i+1] = T(outPtr[i+1]*f + (inPtr[j])*r);
          outPtr[i+2] = T(outPtr[i+2]*f + (inPtr[j])*r);
        }
        inPtr += inC * (steps / outC);
      }
      else if (inC == 2)
      { // luminance(+alpha) blended with luminance+alpha
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          double rLocal = opacity*(inPtr[j+1]-minA);
          double fLocal = 1.0-rLocal;
          outPtr[i] = T((outPtr[i])*fLocal + (inPtr[j])*rLocal);
        }
        inPtr += inC * (steps / outC);
      }
      else
      { // luminance(+alpha) blended with luminance
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          outPtr[i] = T((outPtr[i])*f + (inPtr[j])*r);
        }
        inPtr += inC * (steps / outC);
      }
    }
    // else !IsInStencil()
    else
    {
      vtkIdType outSpanSize = static_cast<vtkIdType>(outSpanEndPtr - outPtr);
      vtkIdType inSpanSize = outSpanSize/outC*inC;
      inPtr += inSpanSize;
    }

    // go to the next span
    outIter.NextSpan();
    if (inPtr == inSpanEndPtr)
    {
      inIter.NextSpan();
      inPtr = inIter.BeginSpan();
      inSpanEndPtr = inIter.EndSpan();
    }
  }
}

//----------------------------------------------------------------------------
// This templated function executes the filter specifically for char data
template <class T>
void vtkImageBlendExecuteChar(vtkImageBlend *self, int extent[6],
                              vtkImageData *inData, T *,
                              vtkImageData *outData, T *,
                              double opacity, int id)
{
  // round opacity to a value in the range [0,256], because division
  // by 256 can be efficiently achieved by bit-shifting by 8 bits
  unsigned short o = static_cast<unsigned short>(256*opacity + 0.5);
  unsigned short r = o;
  unsigned short f = 256 - o;

  int inC = inData->GetNumberOfScalarComponents();
  int outC = outData->GetNumberOfScalarComponents();

  vtkImageStencilData *stencil = self->GetStencil();
  vtkImageStencilIterator<T> outIter(outData, stencil, extent, self, id);
  vtkImageIterator<T> inIter(inData, extent);

  T *inPtr = inIter.BeginSpan();
  T *inSpanEndPtr = inIter.EndSpan();
  while (!outIter.IsAtEnd())
  {
    T* outPtr = outIter.BeginSpan();
    T* outSpanEndPtr = outIter.EndSpan();
    if (outIter.IsInStencil())
    {
      if (outC == 4 && inC == 4)
      { // RGBA blended with RGBA
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0; i < steps; i += 4)
        {
          // multiply to get a number in the range [0,65280]
          // where 65280 = 255*256 = range of inPtr[3] * range of o
          unsigned short rLocal = inPtr[i+3]*o;
          unsigned short fLocal = 65280 - rLocal;
          int v0 = outPtr[i+0]*fLocal + inPtr[i+0]*rLocal;
          int v1 = outPtr[i+1]*fLocal + inPtr[i+1]*rLocal;
          int v2 = outPtr[i+2]*fLocal + inPtr[i+2]*rLocal;
          // do some math tricks to achieve division by 65280:
          // this is not an approximation, it gives exactly the
          // same result as an integer division by 65280
          outPtr[i+0] = (v0 + (v0 >> 8) + (v0 >> 16) + 1) >> 16;
          outPtr[i+1] = (v1 + (v1 >> 8) + (v1 >> 16) + 1) >> 16;
          outPtr[i+2] = (v2 + (v2 >> 8) + (v2 >> 16) + 1) >> 16;
        }
        inPtr += inC * (steps / 4);
      }
      else if (outC >= 3 && inC >= 4)
      { // RGB(A) blended with RGBA
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          // multiply to get a number in the range [0,65280]
          // where 65280 = 255*256 = range of inPtr[3] * range of o
          unsigned short rLocal = inPtr[j+3]*o;
          unsigned short fLocal = 65280 - rLocal;
          int v0 = outPtr[i+0]*fLocal + inPtr[j+0]*rLocal;
          int v1 = outPtr[i+1]*fLocal + inPtr[j+1]*rLocal;
          int v2 = outPtr[i+2]*fLocal + inPtr[j+2]*rLocal;
          // do some math tricks to achieve division by 65280:
          // this is not an approximation, it gives exactly the
          // same result as an integer division by 65280
          outPtr[i+0] = (v0 + (v0 >> 8) + (v0 >> 16) + 1) >> 16;
          outPtr[i+1] = (v1 + (v1 >> 8) + (v1 >> 16) + 1) >> 16;
          outPtr[i+2] = (v2 + (v2 >> 8) + (v2 >> 16) + 1) >> 16;
        }
        inPtr += inC * (steps / outC);
      }
      else if (outC == 3 && inC == 3)
      { // RGB blended with RGB
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0; i < steps; i += 3)
        {
          // the bit-shift achieves a division by 256
          outPtr[i+0] = (outPtr[i+0]*f + inPtr[i+0]*r) >> 8;
          outPtr[i+1] = (outPtr[i+1]*f + inPtr[i+1]*r) >> 8;
          outPtr[i+2] = (outPtr[i+2]*f + inPtr[i+2]*r) >> 8;
        }
        inPtr += inC * (steps / 3);
      }
      else if (outC >= 3 && inC == 3)
      { // RGB(A) blended with RGB
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          // the bit-shift achieves a division by 256
          outPtr[i+0] = (outPtr[i+0]*f + inPtr[j+0]*r) >> 8;
          outPtr[i+1] = (outPtr[i+1]*f + inPtr[j+1]*r) >> 8;
          outPtr[i+2] = (outPtr[i+2]*f + inPtr[j+2]*r) >> 8;
        }
        inPtr += inC * (steps / outC);
      }
      else if (outC >= 3 && inC == 2)
      { // RGB(A) blended with luminance+alpha
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          // multiply to get a number in the range [0,65280]
          // where 65280 = 255*256 = range of inPtr[1] * range of o
          unsigned short rLocal = inPtr[j+1]*o;
          unsigned short fLocal = 65280 - rLocal;
          int v0 = outPtr[i+0]*fLocal + inPtr[j]*rLocal;
          int v1 = outPtr[i+1]*fLocal + inPtr[j]*rLocal;
          int v2 = outPtr[i+2]*fLocal + inPtr[j]*rLocal;
          // do some math tricks to achieve division by 65280:
          // this is not an approximation, it gives exactly the
          // same result as an integer division by 65280
          outPtr[i+0] = (v0 + (v0 >> 8) + (v0 >> 16) + 1) >> 16;
          outPtr[i+1] = (v1 + (v1 >> 8) + (v1 >> 16) + 1) >> 16;
          outPtr[i+2] = (v2 + (v2 >> 8) + (v2 >> 16) + 1) >> 16;
        }
        inPtr += inC * (steps / outC);
      }
      else if (outC >= 3 && inC == 1)
      { // RGB(A) blended with luminance
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          // the bit-shift achieves a division by 256
          outPtr[i+0] = (outPtr[i+0]*f + inPtr[j]*r) >> 8;
          outPtr[i+1] = (outPtr[i+1]*f + inPtr[j]*r) >> 8;
          outPtr[i+2] = (outPtr[i+2]*f + inPtr[j]*r) >> 8;
        }
        inPtr += inC * (steps / outC);
      }
      else if (inC == 2)
      { // luminance(+alpha) blended with luminance+alpha
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          // multiply to get a number in the range [0,65280]
          // where 65280 = 255*256 = range of inPtr[1] * range of o
          unsigned short rLocal = inPtr[j+1]*o;
          unsigned short fLocal = 65280 - rLocal;
          int v0 = outPtr[i]*fLocal + inPtr[j]*rLocal;
          // do some math tricks to achieve division by 65280:
          // this is not an approximation, it gives exactly the
          // same result as an integer division by 65280
          outPtr[i] = (v0 + (v0 >> 8) + (v0 >> 16) + 1) >> 16;
        }
        inPtr += inC * (steps / outC);
      }
      else
      { // luminance(+alpha) blended with luminance
        intptr_t steps = outSpanEndPtr - outPtr;
        for (intptr_t i = 0, j = 0; i < steps; i += outC, j += inC)
        {
          // the bit-shift achieves a division by 256
          outPtr[i] = (outPtr[i]*f + inPtr[j]*r) >> 8;
        }
        inPtr += inC * (steps / outC);
      }
    }
    // else !IsInStencil()
    else
    {
      vtkIdType outSpanSize = static_cast<vtkIdType>(outSpanEndPtr - outPtr);
      vtkIdType inSpanSize = outSpanSize/outC*inC;
      inPtr += inSpanSize;
    }

    // go to the next span
    outIter.NextSpan();
    if (inPtr == inSpanEndPtr)
    {
      inIter.NextSpan();
      inPtr = inIter.BeginSpan();
      inSpanEndPtr = inIter.EndSpan();
    }
  }
}


//----------------------------------------------------------------------------
// This function simply does a copy (for the first input)
//----------------------------------------------------------------------------
static void vtkImageBlendCopyData(vtkImageData *inData, vtkImageData *outData,
                                  int *ext)
{
  int idxY, idxZ, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned char *inPtr, *inPtr1, *outPtr,*outPtr1;

  inPtr = static_cast<unsigned char *>(inData->GetScalarPointerForExtent(ext));
  outPtr =
    static_cast<unsigned char *>(outData->GetScalarPointerForExtent(ext));

  // Get increments to march through inData
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetIncrements(outIncX, outIncY, outIncZ);

  // find the region to loop over
  rowLength = (ext[1] - ext[0]+1)*inIncX*inData->GetScalarSize();
  maxY = ext[3] - ext[2];
  maxZ = ext[5] - ext[4];

  inIncY *= inData->GetScalarSize();
  inIncZ *= inData->GetScalarSize();
  outIncY *= outData->GetScalarSize();
  outIncZ *= outData->GetScalarSize();
  // Loop through outData pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    inPtr1 = inPtr + idxZ*inIncZ;
    outPtr1 = outPtr + idxZ*outIncZ;
    for (idxY = 0; idxY <= maxY; idxY++)
    {
      memcpy(outPtr1,inPtr1,rowLength);
      inPtr1 += inIncY;
      outPtr1 += outIncY;
    }
  }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageBlendCompoundExecute(vtkImageBlend *self,
                                  int extent[6],
                                  vtkImageData *inData,
                                  T *,
                                  vtkImageData *tmpData,
                                  double opacity,
                                  double threshold)
{
  // Opacity
  double minA, maxA;
  double r;

  if (inData->GetScalarType() == VTK_DOUBLE ||
      inData->GetScalarType() == VTK_FLOAT)
  {
    minA = 0.0;
    maxA = 1.0;
  }
  else
  {
    minA = static_cast<double>(inData->GetScalarTypeMin());
    maxA = static_cast<double>(inData->GetScalarTypeMax());
  }

  r = opacity;
  opacity = opacity/(maxA-minA);

  int inC = inData->GetNumberOfScalarComponents();
  int tmpC = tmpData->GetNumberOfScalarComponents();

  if ((inC == 3 || inC == 1) && r <= threshold)
  {
    return;
  }

  // Loop through output pixels
  vtkImageStencilData *stencil = self->GetStencil();
  vtkImageStencilIterator<double> tmpIter(tmpData, stencil, extent);
  vtkImageIterator<T> inIter(inData, extent);

  T *inPtr = inIter.BeginSpan();
  T *inSpanEndPtr = inIter.EndSpan();
  while (!tmpIter.IsAtEnd())
  {
    double *tmpPtr = tmpIter.BeginSpan();
    double *tmpSpanEndPtr = tmpIter.EndSpan();

    if (tmpIter.IsInStencil())
    {
      if (tmpC >= 3)
      {
        // RGB(A) blended with RGBA
        if (inC >= 4)
        {
          while (tmpPtr != tmpSpanEndPtr)
          {
            r = opacity * (static_cast<double>(inPtr[3]) - minA);
            if (r > threshold)
            {
              tmpPtr[0] += static_cast<double>(inPtr[0]) * r;
              tmpPtr[1] += static_cast<double>(inPtr[1]) * r;
              tmpPtr[2] += static_cast<double>(inPtr[2]) * r;
              tmpPtr[3] += r;
            }
            tmpPtr += 4;
            inPtr += inC;
          }
        }

        // RGB(A) blended with RGB
        else if (inC == 3)
        {
          while (tmpPtr != tmpSpanEndPtr)
          {
            tmpPtr[0] += static_cast<double>(inPtr[0]) * r;
            tmpPtr[1] += static_cast<double>(inPtr[1]) * r;
            tmpPtr[2] += static_cast<double>(inPtr[2]) * r;
            tmpPtr[3] += r;
            tmpPtr += 4;
            inPtr += inC;
          }
        }

        // RGB(A) blended with luminance+alpha
        else if (inC == 2)
        {
          while (tmpPtr != tmpSpanEndPtr)
          {
            r = opacity * (static_cast<double>(inPtr[1]) - minA);
            if (r > threshold)
            {
              tmpPtr[0] += static_cast<double>(*inPtr) * r;
              tmpPtr[1] += static_cast<double>(*inPtr) * r;
              tmpPtr[2] += static_cast<double>(*inPtr) * r;
              tmpPtr[3] += r;
            }
            tmpPtr += 4;
            inPtr += 2;
          }
        }

        // RGB(A) blended with luminance
        else if (inC == 1)
        {
          while (tmpPtr != tmpSpanEndPtr)
          {
            tmpPtr[0] += static_cast<double>(*inPtr) * r;
            tmpPtr[1] += static_cast<double>(*inPtr) * r;
            tmpPtr[2] += static_cast<double>(*inPtr) * r;
            tmpPtr[3] += r;
            tmpPtr += 4;
            inPtr++;
          }
        }
      }

      // luminance(+alpha) blended with luminance+alpha
      else if (inC == 2)
      {
        while (tmpPtr != tmpSpanEndPtr)
        {
          r = opacity * (static_cast<double>(inPtr[1]) - minA);
          if (r > threshold)
          {
            tmpPtr[0] = static_cast<double>(*inPtr) * r;
            tmpPtr[1] += r;
          }
          tmpPtr += 2;
          inPtr += 2;
        }
      }

      // luminance(+alpha) blended with luminance
      else
      {
        while (tmpPtr != tmpSpanEndPtr)
        {
          tmpPtr[0] = static_cast<double>(*inPtr) * r;
          tmpPtr[1] += r;
          tmpPtr += 2;
          inPtr++;
        }
      }
    }
    // else !IsInStencil()
    else
    {
      vtkIdType tmpSpanSize = static_cast<vtkIdType>(tmpSpanEndPtr - tmpPtr);
      vtkIdType inSpanSize = tmpSpanSize/tmpC*inC;
      inPtr += inSpanSize;
    }

    // go to the next span
    tmpIter.NextSpan();
    if (inPtr == inSpanEndPtr)
    {
      inIter.NextSpan();
      inPtr = inIter.BeginSpan();
      inSpanEndPtr = inIter.EndSpan();
    }
  }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageBlendCompoundTransferExecute(vtkImageBlend *self,
                                          int extent[6],
                                          vtkImageData *outData,
                                          T *,
                                          vtkImageData *tmpData)
{
  int outC = outData->GetNumberOfScalarComponents();
  int tmpC = tmpData->GetNumberOfScalarComponents();

  // Loop through output pixels
  vtkImageStencilData *stencil = self->GetStencil();
  vtkImageStencilIterator<T> outIter(outData, stencil, extent);
  vtkImageIterator<double> tmpIter(tmpData, extent);

  double *tmpPtr = tmpIter.BeginSpan();
  double *tmpSpanEndPtr = tmpIter.EndSpan();
  while (!outIter.IsAtEnd())
  {
    T *outPtr = outIter.BeginSpan();
    T *outSpanEndPtr = outIter.EndSpan();

    if (outIter.IsInStencil())
    {
      if (tmpC >= 3)
      {
        while (outPtr != outSpanEndPtr)
        {
          double factor = 0.0;
          if (tmpPtr[3] != 0)
          {
            factor = 1.0/tmpPtr[3];
          }
          outPtr[0] = T(tmpPtr[0]*factor);
          outPtr[1] = T(tmpPtr[1]*factor);
          outPtr[2] = T(tmpPtr[2]*factor);
          tmpPtr += 4;
          outPtr += outC;
        }
      }
      else
      {
        while (outPtr != outSpanEndPtr)
        {
          double factor = 0.0;
          if (tmpPtr[1] != 0)
          {
            factor = 1.0/tmpPtr[1];
          }
          outPtr[0] = T(tmpPtr[0]*factor);
          tmpPtr += 2;
          outPtr += outC;
        }
      }
    }
    // else !IsInStencil()
    else
    {
      vtkIdType outSpanSize = static_cast<vtkIdType>(outSpanEndPtr - outPtr);
      vtkIdType tmpSpanSize = outSpanSize/outC*tmpC;
      tmpPtr += tmpSpanSize;
    }

    // go to the next span
    outIter.NextSpan();
    if (tmpPtr == tmpSpanEndPtr)
    {
      tmpIter.NextSpan();
      tmpPtr = tmpIter.BeginSpan();
      tmpSpanEndPtr = tmpIter.EndSpan();
    }
  }
}


//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageBlend::ThreadedRequestData (
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  int extent[6];
  void *inPtr;
  void *outPtr;

  double opacity;

  vtkImageData *tmpData = NULL;

  // check
  if (inData[0][0]->GetNumberOfScalarComponents() > 4)
  {
    vtkErrorMacro("The first input can have a maximum of four components");
    return;
  }

  // init
  switch (this->BlendMode)
  {
    case VTK_IMAGE_BLEND_MODE_NORMAL:
      // copy the first image directly to the output
      vtkDebugMacro("Execute: copy input 0 to the output.");
      vtkImageBlendCopyData(inData[0][0], outData[0], outExt);
      break;

    case VTK_IMAGE_BLEND_MODE_COMPOUND:
      tmpData = vtkImageData::New();
      if (tmpData == NULL)
      {
        vtkErrorMacro(<< "Execute: Unable to allocate memory");
        return;
      }
      tmpData->SetExtent(outExt);
      tmpData->AllocateScalars(
        VTK_DOUBLE,
        (outData[0]->GetNumberOfScalarComponents() >= 3 ? 3 : 1) + 1);
      memset(static_cast<void *>(tmpData->GetScalarPointer()), 0,
             (outExt[1] - outExt[0] + 1) *
             (outExt[3] - outExt[2] + 1) *
             (outExt[5] - outExt[4] + 1) *
             tmpData->GetNumberOfScalarComponents() *
             tmpData->GetScalarSize());
      break;

    default:
      vtkErrorMacro(<< "Execute: Unknown blending mode");
      return;
  }

  // process each input
  int first_index = (this->BlendMode == VTK_IMAGE_BLEND_MODE_NORMAL ? 1 : 0);
  for (int idx1 = first_index;
       idx1 < this->GetNumberOfInputConnections(0); ++idx1)
  {
    if (inData[0][idx1] != NULL)
    {

      // RGB with RGB, greyscale with greyscale
      if ((inData[0][idx1]->GetNumberOfScalarComponents()+1)/2 == 2 &&
          (inData[0][0]->GetNumberOfScalarComponents()+1)/2 == 1)
      {
        vtkErrorMacro("input has too many components, can't blend RGB data \
                       into greyscale data");
        continue;
      }

      // this filter expects that input is the same type as output.
      if (inData[0][idx1]->GetScalarType() != outData[0]->GetScalarType())
      {
        vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" <<
        inData[0][idx1]->GetScalarType() <<
        "), must match output ScalarType (" << outData[0]->GetScalarType()
        << ")");
        continue;
      }

      // input extents
      vtkInformation *inInfo =
        inputVector[0]->GetInformationObject(idx1);
      int *inWextent =
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
      this->InternalComputeInputUpdateExtent(extent, outExt, inWextent);

      bool skip = false;
      for (int i = 0; i < 3; i++)
      {
        if (outExt[2*i+1] < extent[2*i] || outExt[2*i] > extent[2*i+1])
        {
          // extents don't overlap, skip this input
          skip = true;
        }
      }

      if (skip)
      {
        vtkDebugMacro("Execute: skipping input.");
        continue;
      }

      opacity = this->GetOpacity(idx1);

      inPtr = inData[0][idx1]->GetScalarPointerForExtent(extent);

      // vtkDebugMacro("Execute: " << idx1 << "=>" << extent[0] << ", " << extent[1] << " / " << extent[2] << ", " << extent[3] << " / " << extent[4] << ", " << extent[5]);

      switch (this->BlendMode)
      {
        case VTK_IMAGE_BLEND_MODE_NORMAL:
          outPtr = outData[0]->GetScalarPointerForExtent(extent);
          // for performance reasons, use a special method for unsigned char
          if (inData[0][idx1]->GetScalarType() == VTK_UNSIGNED_CHAR)
          {
            vtkImageBlendExecuteChar(this, extent,
                                     inData[0][idx1],
                                     static_cast<unsigned char *>(inPtr),
                                     outData[0],
                                     static_cast<unsigned char *>(outPtr),
                                     opacity, id);
          }
          else
          {
            switch (inData[0][idx1]->GetScalarType())
            {
              vtkTemplateMacro(
                vtkImageBlendExecute(this, extent,
                                     inData[0][idx1],
                                     static_cast<VTK_TT *>(inPtr),
                                     outData[0],
                                     static_cast<VTK_TT *>(outPtr),
                                     opacity, id));
              default:
                vtkErrorMacro(<< "Execute: Unknown ScalarType");
                return;
            }
          }
          break;

        case VTK_IMAGE_BLEND_MODE_COMPOUND:
          switch (inData[0][idx1]->GetScalarType())
          {
            vtkTemplateMacro(
              vtkImageBlendCompoundExecute(this,
                                           extent,
                                           inData[0][idx1],
                                           static_cast<VTK_TT *>(inPtr),
                                           tmpData,
                                           opacity,
                                           this->CompoundThreshold));
            default:
              vtkErrorMacro(<< "Execute: Unknown ScalarType");
              return;
          }
          break;

        default:
          vtkErrorMacro(<< "Execute: Unknown blending mode");
      }
    }
  }

  // conclude
  switch (this->BlendMode)
  {
    case VTK_IMAGE_BLEND_MODE_NORMAL:
      break;

    case VTK_IMAGE_BLEND_MODE_COMPOUND:
      outPtr = outData[0]->GetScalarPointerForExtent(outExt);
      switch (outData[0]->GetScalarType())
      {
        vtkTemplateMacro(
          vtkImageBlendCompoundTransferExecute(this,
                                               outExt,
                                               outData[0],
                                               static_cast<VTK_TT *>(outPtr),
                                               tmpData));
        default:
          vtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
      }
      tmpData->Delete();
      break;

    default:
      vtkErrorMacro(<< "Execute: Unknown blending mode");
  }
}


//----------------------------------------------------------------------------
void vtkImageBlend::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  int i;
  for (i = 0; i < this->OpacityArrayLength; i++)
  {
    os << indent << "Opacity(" << i << "): " << this->GetOpacity(i) << endl;
  }
  os << indent << "Stencil: " << this->GetStencil() << endl;
  os << indent << "BlendMode: " << this->GetBlendModeAsString() << endl
     << indent << "CompoundThreshold: " << this->CompoundThreshold << endl;
}

//----------------------------------------------------------------------------
int vtkImageBlend::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    // the stencil input is optional
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
