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
  : CompactPoints(false)
{
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

  try
  {
    // convert the input dataset to a viskores::cont::DataSet
    auto fieldsFlag = this->CompactPoints ? tovtkm::FieldsFlag::Points : tovtkm::FieldsFlag::None;
    viskores::cont::DataSet in = tovtkm::Convert(input, fieldsFlag);

    // apply the filter
    viskores::filter::clean_grid::CleanGrid filter;
    filter.SetCompactPointFields(this->CompactPoints);
    auto result = filter.Execute(in);

    // convert back to vtkDataSet (vtkUnstructuredGrid)
    if (!fromvtkm::Convert(result, output, input))
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

  // pass cell data
  if (!this->CompactPoints)
  {
    output->GetPointData()->PassData(input->GetPointData());
  }
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}
VTK_ABI_NAMESPACE_END
