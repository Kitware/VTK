// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmCoordinateSystemTransform.h"
#include "vtkmConfigFilters.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include <viskores/filter/field_transform/CylindricalCoordinateTransform.h>
#include <viskores/filter/field_transform/SphericalCoordinateTransform.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmCoordinateSystemTransform);

//------------------------------------------------------------------------------
vtkmCoordinateSystemTransform::vtkmCoordinateSystemTransform()
{
  this->TransformType = TransformTypes::None;
}

//------------------------------------------------------------------------------
vtkmCoordinateSystemTransform::~vtkmCoordinateSystemTransform() = default;

//------------------------------------------------------------------------------
void vtkmCoordinateSystemTransform::SetCartesianToCylindrical()
{
  this->TransformType = TransformTypes::CarToCyl;
}

//------------------------------------------------------------------------------
void vtkmCoordinateSystemTransform::SetCylindricalToCartesian()
{
  this->TransformType = TransformTypes::CylToCar;
}

//------------------------------------------------------------------------------
void vtkmCoordinateSystemTransform::SetCartesianToSpherical()
{
  this->TransformType = TransformTypes::CarToSph;
}

//------------------------------------------------------------------------------
void vtkmCoordinateSystemTransform::SetSphericalToCartesian()
{
  this->TransformType = TransformTypes::SphToCar;
}

//------------------------------------------------------------------------------
int vtkmCoordinateSystemTransform::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmCoordinateSystemTransform::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//------------------------------------------------------------------------------
int vtkmCoordinateSystemTransform::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  output->CopyStructure(input);

  vtkPoints* inPts = input->GetPoints();

  if (!inPts || this->TransformType == TransformTypes::None)
  {
    vtkErrorMacro(<< "Miss input points or transform type has not been specified");
    return 0;
  }

  try
  {
    viskores::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::Points);
    vtkPoints* points = nullptr;
    if (this->TransformType == TransformTypes::CarToCyl ||
      this->TransformType == TransformTypes::CylToCar)
    { // Cylindrical coordinate transform
      viskores::filter::field_transform::CylindricalCoordinateTransform cylindricalCT;
      cylindricalCT.SetUseCoordinateSystemAsField(true);
      (this->TransformType == TransformTypes::CarToCyl) ? cylindricalCT.SetCartesianToCylindrical()
                                                        : cylindricalCT.SetCylindricalToCartesian();
      auto result = cylindricalCT.Execute(in);
      points = fromvtkm::Convert(result.GetCoordinateSystem());
    }
    else
    { // Spherical coordinate system
      viskores::filter::field_transform::SphericalCoordinateTransform sphericalCT;
      sphericalCT.SetUseCoordinateSystemAsField(true);
      (this->TransformType == TransformTypes::CarToSph) ? sphericalCT.SetCartesianToSpherical()
                                                        : sphericalCT.SetSphericalToCartesian();
      auto result = sphericalCT.Execute(in);
      points = fromvtkm::Convert(result.GetCoordinateSystem());
    }

    if (points)
    {
      output->SetPoints(points);
      points->FastDelete();
    }
  }
  catch (const viskores::cont::Error& e)
  {
    vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
    return 0;
  }

  // Update ourselves and release memory
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//------------------------------------------------------------------------------
void vtkmCoordinateSystemTransform::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
