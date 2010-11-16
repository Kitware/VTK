/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStencil.h"

#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageStencil);

//----------------------------------------------------------------------------
vtkImageStencil::vtkImageStencil()
{
  this->ReverseStencil = 0;

  this->BackgroundColor[0] = 1;
  this->BackgroundColor[1] = 1;
  this->BackgroundColor[2] = 1;
  this->BackgroundColor[3] = 1;
  this->SetNumberOfInputPorts(3);
}

//----------------------------------------------------------------------------
vtkImageStencil::~vtkImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetStencil(vtkImageStencilData *stencil)
{
  this->SetInput(2, stencil); 
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageStencil::GetStencil()
{
  if (this->GetNumberOfInputConnections(2) < 1)
    {
    return NULL;
    }
  else
    {
    return vtkImageStencilData::SafeDownCast(
      this->GetExecutive()->GetInputData(2, 0));
    }
}

//----------------------------------------------------------------------------
void vtkImageStencil::SetBackgroundInput(vtkImageData *data)
{
  this->SetInput(1, data); 
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageStencil::GetBackgroundInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  else
    {
    return vtkImageData::SafeDownCast(
      this->GetExecutive()->GetInputData(1, 0));
    }
}

//----------------------------------------------------------------------------
// Some helper functions for 'ThreadedRequestData'
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// copy a pixel, advance the output pointer but not the input pointer

template<class T>
inline void vtkCopyPixel(T *&out, const T *in, int numscalars)
{
  do
    {
    *out++ = *in++;
    }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Convert background color from double to appropriate type

template <class T>
void vtkAllocBackground(vtkImageStencil *self, T *&background,
                        vtkInformation *outInfo)
{
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int numComponents = output->GetNumberOfScalarComponents();
  int scalarType = output->GetScalarType();

  background = new T[numComponents];

  for (int i = 0; i < numComponents; i++)
    {
    if (i < 4)
      {
      if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
        {
        background[i] = static_cast<T>(self->GetBackgroundColor()[i]);
        }
      else
        { // round float to nearest int
        background[i] =
          static_cast<T>(floor(self->GetBackgroundColor()[i] + 0.5));
        }
      }
    else
      { // all values past 4 are set to zero
      background[i] = 0;
      }
    }
}

//----------------------------------------------------------------------------
template <class T>
void vtkFreeBackground(vtkImageStencil *vtkNotUsed(self), T *&background)
{
  delete [] background;
  background = NULL;
}

//----------------------------------------------------------------------------
template <class T>
void vtkImageStencilExecute(vtkImageStencil *self,
                            vtkImageData *inData, T *,
                            vtkImageData *inData2, T *,
                            vtkImageData *outData, T *,
                            int outExt[6], int id,
                            vtkInformation *outInfo)
{
  vtkImageStencilData *stencil = self->GetStencil();

  vtkImageIterator<T> inIter(inData, outExt);
  vtkImageStencilIterator<T> outIter(outData, stencil, outExt, self, id);

  int numscalars = outData->GetNumberOfScalarComponents();

  // whether to reverse the stencil
  bool reverseStencil = (self->GetReverseStencil() != 0);

  // if no background image is provided in inData2
  if (inData2 == 0)
    {
    // set color for area outside of input volume extent
    T *background;
    vtkAllocBackground(self, background, outInfo);

    T *inPtr = inIter.BeginSpan();
    T *inSpanEndPtr = inIter.EndSpan();
    while (!outIter.IsAtEnd())
      {
      T* outPtr = outIter.BeginSpan();
      T* outSpanEndPtr = outIter.EndSpan();

      T *tmpPtr = inPtr;
      int tmpInc = numscalars;
      if (!(outIter.IsInStencil() ^ reverseStencil))
        {
        tmpPtr = background;
        tmpInc = 0;
        }

      // move inPtr forward by the span size
      inPtr += (outSpanEndPtr - outPtr);

      while (outPtr != outSpanEndPtr)
        {
        // CopyPixel increments outPtr but not tmpPtr
        vtkCopyPixel(outPtr, tmpPtr, numscalars);
        tmpPtr += tmpInc;
        }

      outIter.NextSpan();

      // this occurs at the end of a full row
      if (inPtr == inSpanEndPtr)
        {
        inIter.NextSpan();
        inPtr = inIter.BeginSpan();
        inSpanEndPtr = inIter.EndSpan();
        }
      }

    vtkFreeBackground(self, background);
    }

  // if a background image is given in inData2
  else
    {
    vtkImageIterator<T> inIter2(inData2, outExt);

    T *inPtr = inIter.BeginSpan();
    T *inPtr2 = inIter2.BeginSpan();
    T *inSpanEndPtr = inIter.EndSpan();
    while (!outIter.IsAtEnd())
      {
      T* outPtr = outIter.BeginSpan();
      T* outSpanEndPtr = outIter.EndSpan();

      T *tmpPtr = inPtr;
      if (!(outIter.IsInStencil() ^ reverseStencil))
        {
        tmpPtr = inPtr2;
        }

      // move inPtr forward by the span size
      inPtr += (outSpanEndPtr - outPtr);
      inPtr2 += (outSpanEndPtr - outPtr);

      while (outPtr != outSpanEndPtr)
        {
        // CopyPixel increments outPtr but not tmpPtr
        vtkCopyPixel(outPtr, tmpPtr, numscalars);
        tmpPtr += numscalars;
        }

      outIter.NextSpan();

      // this occurs at the end of a full row
      if (inPtr == inSpanEndPtr)
        {
        inIter.NextSpan();
        inIter2.NextSpan();
        inPtr = inIter.BeginSpan();
        inPtr2 = inIter2.BeginSpan();
        inSpanEndPtr = inIter.EndSpan();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageStencil::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector,
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr, *inPtr2;
  void *outPtr;
  vtkImageData *inData2 = this->GetBackgroundInput();
  
  inPtr = inData[0][0]->GetScalarPointer();
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inPtr2 = NULL;
  if (inData2)
    {
    inPtr2 = inData2->GetScalarPointer();
    if (inData2->GetScalarType() != inData[0][0]->GetScalarType())
      {
      if (id == 0)
        {
        vtkErrorMacro("Execute: BackgroundInput ScalarType " 
                      << inData2->GetScalarType()
                      << ", must match Input ScalarType "
                      << inData[0][0]->GetScalarType());
        }
      return;
      }
    else if (inData2->GetNumberOfScalarComponents() 
             != inData[0][0]->GetNumberOfScalarComponents())
      {
      if (id == 0)
        {
        vtkErrorMacro("Execute: BackgroundInput NumberOfScalarComponents " 
                      << inData2->GetNumberOfScalarComponents()
                      << ", must match Input NumberOfScalarComponents "
                      << inData[0][0]->GetNumberOfScalarComponents());
        }
      return;
      }
    int wholeExt1[6], wholeExt2[6];
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *inInfo2 = inputVector[1]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt1);
    inInfo2->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExt2);

    for (int i = 0; i < 6; i++)
      {
      if (wholeExt1[i] != wholeExt2[i])
        {
        if (id == 0)
          {
          vtkErrorMacro("Execute: BackgroundInput must have the same "
                        "WholeExtent as the Input");
          }
        return;
        }
      }
    }
  
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageStencilExecute(this, 
                             inData[0][0],
                             static_cast<VTK_TT *>(inPtr), 
                             inData2, 
                             static_cast<VTK_TT *>(inPtr2), 
                             outData[0], 
                             static_cast<VTK_TT *>(outPtr), 
                             outExt, 
                             id, 
                             outInfo));
    default:
      vtkErrorMacro("Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
int vtkImageStencil::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 2)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    if (port == 1)
      {
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
      }
    }
  return 1;
}

void vtkImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ?
                                         "On\n" : "Off\n");

  os << indent << "BackgroundInput: " << this->GetBackgroundInput() << "\n";
  os << indent << "BackgroundValue: " << this->BackgroundColor[0] << "\n";

  os << indent << "BackgroundColor: (" << this->BackgroundColor[0] << ", "
                                    << this->BackgroundColor[1] << ", "
                                    << this->BackgroundColor[2] << ", "
                                    << this->BackgroundColor[3] << ")\n";
}
