/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageStencilToImage.h"

#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageStencilToImage);

//----------------------------------------------------------------------------
vtkImageStencilToImage::vtkImageStencilToImage()
{
  this->OutsideValue = 0;
  this->InsideValue = 1;
  this->OutputScalarType = VTK_UNSIGNED_CHAR;

  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkImageStencilToImage::~vtkImageStencilToImage()
{
}

//----------------------------------------------------------------------------
int vtkImageStencilToImage::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int extent[6];
  double spacing[3];
  double origin[3];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  inInfo->Get(vtkDataObject::ORIGIN(), origin);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  vtkDataObject::SetPointDataActiveScalarInfo(
    outInfo, this->OutputScalarType, -1);

  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkImageStencilToImageExecute(
  vtkImageStencilToImage *self,
  vtkImageStencilData *stencil,
  vtkImageData *outData, T *,
  int outExt[6], int id)
{
  double inValueD = self->GetInsideValue();
  double outValueD = self->GetOutsideValue();

  double tmin = outData->GetScalarTypeMin();
  double tmax = outData->GetScalarTypeMax();

  if (inValueD < tmin)
    {
    inValueD = tmin;
    }
  if (inValueD > tmax)
    {
    inValueD = tmax;
    }
  if (outValueD < tmin)
    {
    outValueD = tmin;
    }
  if (outValueD > tmax)
    {
    outValueD = tmax;
    }

  T inValue = static_cast<T>(inValueD);
  T outValue = static_cast<T>(outValueD);

  vtkImageStencilIterator<T> outIter(outData, stencil, outExt, self, id);

  // Loop through output pixels
  while (!outIter.IsAtEnd())
    {
    T* outPtr = outIter.BeginSpan();
    T* spanEndPtr = outIter.EndSpan();

    if (outIter.IsInStencil())
      {
      while (outPtr != spanEndPtr)
        {
        *outPtr++ = inValue;
        }
      }
    else
      {
      while (outPtr != spanEndPtr)
        {
        *outPtr++ = outValue;
        }
      }

    outIter.NextSpan();
    }
}

//----------------------------------------------------------------------------
int vtkImageStencilToImage::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int updateExtent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               updateExtent);
  vtkImageData *outData = static_cast<vtkImageData *>(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->AllocateOutputData(outData, outInfo, updateExtent);
  void *outPtr = outData->GetScalarPointerForExtent(updateExtent);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageStencilData *inData = static_cast<vtkImageStencilData *>(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageStencilToImageExecute(
        this, inData, outData, static_cast<VTK_TT *>(outPtr),
        updateExtent, 0));
    default:
      vtkErrorMacro("Execute: Unknown ScalarType");
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageStencilToImage::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageStencilToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InsideValue: " << this->InsideValue << "\n";
  os << indent << "OutsideValue: " << this->OutsideValue << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
}
