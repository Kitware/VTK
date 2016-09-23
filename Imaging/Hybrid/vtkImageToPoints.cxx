/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToPoints.h"

#include <vtkObjectFactory.h>
#include <vtkMath.h>
#include <vtkPoints.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkImageStencilData.h>
#include <vtkImagePointIterator.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkSmartPointer.h>

vtkStandardNewMacro(vtkImageToPoints);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageToPoints::vtkImageToPoints()
{
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkImageToPoints::~vtkImageToPoints()
{
}

//----------------------------------------------------------------------------
void vtkImageToPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputPointsPrecision: "
     << this->OutputPointsPrecision << "\n";
}

//----------------------------------------------------------------------------
void vtkImageToPoints::SetStencilConnection(
  vtkAlgorithmOutput *stencil)
{
  this->SetInputConnection(1, stencil);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput *vtkImageToPoints::GetStencilConnection()
{
  return this->GetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
void vtkImageToPoints::SetStencilData(vtkImageStencilData *stencil)
{
  this->SetInputData(1, stencil);
}

//----------------------------------------------------------------------------
int vtkImageToPoints::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToPoints::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToPoints::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToPoints::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int inExt[6];
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the input extent
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    vtkInformation *stencilInfo = inputVector[1]->GetInformationObject(0);
    stencilInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                     inExt, 6);
  }

  return 1;
}

namespace {

//----------------------------------------------------------------------------
vtkIdType vtkImageToPointsCount(
  vtkImageData *inData, vtkImageStencilData *stencil, const int extent[6])
{
  // count the number of points so that we can pre-allocate the space
  vtkIdType count = 0;

  // iterate over all spans for the stencil
  vtkImagePointDataIterator inIter(inData, extent, stencil);
  for (; !inIter.IsAtEnd(); inIter.NextSpan())
  {
    if (inIter.IsInStencil())
    {
      count += inIter.SpanEndId() - inIter.GetId();
    }
  }

  return count;
}

//----------------------------------------------------------------------------
// The execute method is templated over the point type (float or double)
template<class T>
void vtkImageToPointsExecute(
  vtkImageToPoints *self, vtkImageData *inData, const int extent[6],
  vtkImageStencilData *stencil, T *outPoints, vtkPointData *outPD)
{
  vtkPointData *inPD = inData->GetPointData();
  vtkImagePointIterator inIter(inData, extent, stencil, self, 0);
  vtkIdType outId = 0;

  // iterate over all spans for the stencil
  while (!inIter.IsAtEnd())
  {
    if (inIter.IsInStencil())
    {
      // if span is inside stencil, generate points
      vtkIdType n = inIter.SpanEndId() - inIter.GetId();
      outPD->CopyData(inPD, outId, n, inIter.GetId());
      outId += n;
      for (vtkIdType i = 0; i < n; i++)
      {
        inIter.GetPosition(outPoints);
        outPoints += 3;
        inIter.Next();
      }
    }
    else
    {
      // if span is outside stencil, skip to next span
      inIter.NextSpan();
    }
  }
}

}

//----------------------------------------------------------------------------
int vtkImageToPoints::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the input
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  vtkInformation *stencilInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  // use a stencil, if a stencil is connected
  vtkImageStencilData* stencil = 0;
  if (stencilInfo)
  {
    stencil = static_cast<vtkImageStencilData *>(
      stencilInfo->Get(vtkDataObject::DATA_OBJECT()));
  }

  // get the requested precision
  int pointsType = VTK_DOUBLE;
  if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    pointsType = VTK_FLOAT;
  }

  // get the output data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkPolyData *outData = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // count the total number of output points
  const int *extent = inData->GetExtent();
  vtkIdType numPoints = vtkImageToPointsCount(inData, stencil, extent);

  // create the points
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(pointsType);
  points->SetNumberOfPoints(numPoints);
  outData->SetPoints(points);

  // pre-allocate output arrays
  vtkPointData *outPD = outData->GetPointData();
  outPD->CopyAllocate(inData->GetPointData(), numPoints);

  // iterate over the input and create the point data
  void *ptr = points->GetVoidPointer(0);
  if (pointsType == VTK_FLOAT)
  {
    vtkImageToPointsExecute(
      this, inData, extent, stencil, static_cast<float *>(ptr), outPD);
  }
  else
  {
    vtkImageToPointsExecute(
      this, inData, extent, stencil, static_cast<double *>(ptr), outPD);
  }

  return 1;
}
