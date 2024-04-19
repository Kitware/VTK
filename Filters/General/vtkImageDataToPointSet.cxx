// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageDataToPointSet.h"

#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"

#include "vtkNew.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageDataToPointSet);

//------------------------------------------------------------------------------
vtkImageDataToPointSet::vtkImageDataToPointSet() = default;

vtkImageDataToPointSet::~vtkImageDataToPointSet() = default;

void vtkImageDataToPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkImageDataToPointSet::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageDataToPointSet::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkImageData* inData = vtkImageData::GetData(inputVector[0]);
  vtkStructuredGrid* outData = vtkStructuredGrid::GetData(outputVector);

  if (inData == nullptr)
  {
    vtkErrorMacro(<< "Input data is nullptr.");
    return 0;
  }
  if (outData == nullptr)
  {
    vtkErrorMacro(<< "Output data is nullptr.");
    return 0;
  }

  // Copy input point and cell data to output
  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());

  // Extract points coordinates from the image
  vtkIdType nbPoints = inData->GetNumberOfPoints();
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nbPoints);
  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    double p[3];
    inData->GetPoint(i, p);
    points->SetPoint(i, p);
  }
  outData->SetPoints(points);

  // Copy Extent
  int extent[6];
  inData->GetExtent(extent);
  outData->SetExtent(extent);

  return 1;
}
VTK_ABI_NAMESPACE_END
