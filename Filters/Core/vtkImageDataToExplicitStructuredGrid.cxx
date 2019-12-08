/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToExplicitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDataToExplicitStructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageDataToExplicitStructuredGrid);

//----------------------------------------------------------------------------
int vtkImageDataToExplicitStructuredGrid::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int extent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageDataToExplicitStructuredGrid::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
  vtkExplicitStructuredGrid* output = vtkExplicitStructuredGrid::GetData(outputVector, 0);

  if (!input)
  {
    vtkErrorMacro(<< "No input!");
    return 0;
  }
  if (input->GetDataDimension() != 3)
  {
    vtkErrorMacro(<< "Cannot convert non 3D image data!");
    return 0;
  }

  // Copy input point and cell data to output
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->GetCellData()->ShallowCopy(input->GetCellData());

  vtkIdType nbCells = input->GetNumberOfCells();
  vtkIdType nbPoints = input->GetNumberOfPoints();

  // Extract points coordinates from the image
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nbPoints);
  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    double p[3];
    input->GetPoint(i, p);
    points->SetPoint(i, p);
  }

  // Build hexahedrons cells from input voxels
  vtkNew<vtkCellArray> cells;
  cells->AllocateEstimate(nbCells, 8);
  vtkNew<vtkIdList> ptIds;
  for (vtkIdType i = 0; i < nbCells; i++)
  {
    input->GetCellPoints(i, ptIds.Get());
    assert(ptIds->GetNumberOfIds() == 8);
    // Change point order: voxels and hexahedron don't have same connectivity.
    vtkIdType ids[8];
    ids[0] = ptIds->GetId(0);
    ids[1] = ptIds->GetId(1);
    ids[2] = ptIds->GetId(3);
    ids[3] = ptIds->GetId(2);
    ids[4] = ptIds->GetId(4);
    ids[5] = ptIds->GetId(5);
    ids[6] = ptIds->GetId(7);
    ids[7] = ptIds->GetId(6);
    cells->InsertNextCell(8, ids);
  }

  int extents[6];
  input->GetExtent(extents);
  output->SetExtent(extents);
  output->SetPoints(points.Get());
  output->SetCells(cells.Get());
  output->ComputeFacesConnectivityFlagsArray();
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageDataToExplicitStructuredGrid::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
