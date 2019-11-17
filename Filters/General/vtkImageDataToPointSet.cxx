/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
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

vtkStandardNewMacro(vtkImageDataToPointSet);

//-------------------------------------------------------------------------
vtkImageDataToPointSet::vtkImageDataToPointSet() = default;

vtkImageDataToPointSet::~vtkImageDataToPointSet() = default;

void vtkImageDataToPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
int vtkImageDataToPointSet::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//-------------------------------------------------------------------------
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
