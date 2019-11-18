/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkImageToStructuredGrid.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkImageToStructuredGrid.h"
#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredGrid.h"

#include <cassert>

//
// Standard methods
//
vtkStandardNewMacro(vtkImageToStructuredGrid);

vtkImageToStructuredGrid::vtkImageToStructuredGrid() = default;

//------------------------------------------------------------------------------
vtkImageToStructuredGrid::~vtkImageToStructuredGrid() = default;

//------------------------------------------------------------------------------
void vtkImageToStructuredGrid::PrintSelf(std::ostream& oss, vtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
}

//------------------------------------------------------------------------------
int vtkImageToStructuredGrid::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageToStructuredGrid::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkImageToStructuredGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  assert(inInfo != nullptr);
  assert(outInfo != nullptr);

  vtkImageData* img = vtkImageData::SafeDownCast(inInfo->Get(vtkImageData::DATA_OBJECT()));
  assert(img != nullptr);

  vtkStructuredGrid* grid =
    vtkStructuredGrid::SafeDownCast(outInfo->Get(vtkStructuredGrid::DATA_OBJECT()));
  assert(grid != nullptr);

  int dims[3];
  img->GetDimensions(dims);

  vtkPoints* gridPoints = vtkPoints::New();
  assert(gridPoints != nullptr);
  gridPoints->SetDataTypeToDouble();
  gridPoints->SetNumberOfPoints(img->GetNumberOfPoints());

  double pnt[3];
  for (int i = 0; i < img->GetNumberOfPoints(); ++i)
  {
    img->GetPoint(i, pnt);
    gridPoints->SetPoint(i, pnt);
  }
  grid->SetDimensions(dims);
  grid->SetPoints(gridPoints);
  gridPoints->Delete();

  this->CopyPointData(img, grid);
  this->CopyCellData(img, grid);

  return 1;
}

//------------------------------------------------------------------------------
void vtkImageToStructuredGrid::CopyPointData(vtkImageData* img, vtkStructuredGrid* sgrid)
{
  assert(img != nullptr);
  assert(sgrid != nullptr);

  if (img->GetPointData()->GetNumberOfArrays() == 0)
    return;

  for (int i = 0; i < img->GetPointData()->GetNumberOfArrays(); ++i)
  {
    vtkDataArray* myArray = img->GetPointData()->GetArray(i);
    sgrid->GetPointData()->AddArray(myArray);
  } // END for all node arrays
}

//------------------------------------------------------------------------------
void vtkImageToStructuredGrid::CopyCellData(vtkImageData* img, vtkStructuredGrid* sgrid)
{
  assert(img != nullptr);
  assert(sgrid != nullptr);

  if (img->GetCellData()->GetNumberOfArrays() == 0)
    return;

  for (int i = 0; i < img->GetCellData()->GetNumberOfArrays(); ++i)
  {
    vtkDataArray* myArray = img->GetCellData()->GetArray(i);
    sgrid->GetCellData()->AddArray(myArray);
  } // END for all cell arrays
}
