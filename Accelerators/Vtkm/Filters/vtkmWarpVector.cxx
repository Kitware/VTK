// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmWarpVector.h"
#include "vtkmConfigFilters.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkm/cont/DataSet.h"

#include <vtkm/filter/field_transform/WarpVector.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmWarpVector);

//------------------------------------------------------------------------------
vtkmWarpVector::vtkmWarpVector() = default;

//------------------------------------------------------------------------------
vtkmWarpVector::~vtkmWarpVector() = default;

//------------------------------------------------------------------------------
int vtkmWarpVector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkSmartPointer<vtkPointSet> output = vtkPointSet::GetData(outputVector);

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
  vtkIdType numPts = input->GetPoints()->GetNumberOfPoints();

  vtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);
  int vectorsAssociation = this->GetInputArrayAssociation(0, inputVector);

  if (!vectors || !numPts)
  {
    vtkDebugMacro(<< "no input data");
    return 1;
  }

  output->CopyStructure(input);

  try
  {
    vtkm::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    vtkm::cont::Field vectorField = tovtkm::Convert(vectors, vectorsAssociation);
    in.AddField(vectorField);

    vtkm::filter::field_transform::WarpVector warpVector(this->ScaleFactor);
    warpVector.SetUseCoordinateSystemAsField(true);
    warpVector.SetVectorField(vectorField.GetName(), vectorField.GetAssociation());
    auto result = warpVector.Execute(in);

    vtkDataArray* warpVectorResult =
      fromvtkm::Convert(result.GetField("warpvector", vtkm::cont::Field::Association::Points));
    vtkNew<vtkPoints> newPts;

    newPts->SetNumberOfPoints(warpVectorResult->GetNumberOfTuples());
    newPts->SetData(warpVectorResult);
    output->SetPoints(newPts);
    warpVectorResult->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
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
void vtkmWarpVector::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
