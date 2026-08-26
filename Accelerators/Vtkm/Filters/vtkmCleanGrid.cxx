// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmCleanGrid.h"

#include "vtkCellData.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include <viskores/filter/clean_grid/CleanGrid.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmCleanGrid);

//------------------------------------------------------------------------------
vtkmCleanGrid::vtkmCleanGrid()
{
  this->ForceVTKm = true; // Because it's NOT VTKm a implementation of  VTK filter.
}

//------------------------------------------------------------------------------
vtkmCleanGrid::~vtkmCleanGrid() = default;

//------------------------------------------------------------------------------
void vtkmCleanGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CompactPoints: " << (this->CompactPoints ? "On" : "Off") << "\n";
}

//------------------------------------------------------------------------------
int vtkmCleanGrid::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmCleanGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  tovtkm::FieldsFlag fieldsFlag = tovtkm::FieldsFlag::None;
  if (this->CompactPoints || this->MergePoints)
  {
    fieldsFlag = fieldsFlag | tovtkm::FieldsFlag::Points;
  }
  if (this->RemoveDegenerateCells)
  {
    fieldsFlag = fieldsFlag | tovtkm::FieldsFlag::Cells;
  }

  try
  {
    // convert the input dataset to a viskores::cont::DataSet
    viskores::cont::DataSet in = tovtkm::Convert(input, fieldsFlag, this->ForceVTKm);

    // apply the filter
    viskores::filter::clean_grid::CleanGrid filter;
    filter.SetCompactPointFields(this->CompactPoints);
    filter.SetMergePoints(this->MergePoints);
    filter.SetTolerance(this->Tolerance);
    filter.SetToleranceIsAbsolute(this->ToleranceIsAbsolute);
    filter.SetRemoveDegenerateCells(this->RemoveDegenerateCells);
    filter.SetFastMerge(this->FastMerge);
    auto result = filter.Execute(in);

    // convert back to vtkDataSet (vtkUnstructuredGrid)
    if (!fromvtkm::Convert(result, output, input, this->ForceVTKm))
    {
      vtkErrorMacro(<< "Unable to convert Viskores DataSet back to VTK");
      return 0;
    }
  }
  catch (const viskores::cont::Error& e)
  {
    vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
    return 0;
  }

  // pass data that was not sent to filter
  if ((fieldsFlag & tovtkm::FieldsFlag::Points) != tovtkm::FieldsFlag::Points)
  {
    output->GetPointData()->PassData(input->GetPointData());
  }
  if ((fieldsFlag & tovtkm::FieldsFlag::Cells) != tovtkm::FieldsFlag::Cells)
  {
    output->GetCellData()->PassData(input->GetCellData());
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
