// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmPointElevation.h"
#include "vtkmConfigFilters.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include <viskores/filter/field_transform/PointElevation.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmPointElevation);

//------------------------------------------------------------------------------
vtkmPointElevation::vtkmPointElevation() = default;

//------------------------------------------------------------------------------
vtkmPointElevation::~vtkmPointElevation() = default;

//------------------------------------------------------------------------------
int vtkmPointElevation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);
  // Check the size of the input.
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    vtkDebugMacro("No input!");
    return 1;
  }

  try
  {
    // Convert the input dataset to a viskores::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::Points);

    // Setup input
    viskores::filter::field_transform::PointElevation filter;
    filter.SetLowPoint(this->LowPoint[0], this->LowPoint[1], this->LowPoint[2]);
    filter.SetHighPoint(this->HighPoint[0], this->HighPoint[1], this->HighPoint[2]);
    filter.SetRange(this->ScalarRange[0], this->ScalarRange[1]);
    filter.SetOutputFieldName("elevation");
    filter.SetUseCoordinateSystemAsField(true);
    auto result = filter.Execute(in);

    // Convert the result back
    vtkDataArray* resultingArray = fromvtkm::Convert(result.GetField("elevation"));
    if (resultingArray == nullptr)
    {
      vtkErrorMacro(<< "Unable to convert result array from Viskores to VTK");
      return 0;
    }
    output->GetPointData()->AddArray(resultingArray);
    output->GetPointData()->SetActiveScalars("elevation");
    resultingArray->FastDelete();
  }
  catch (const viskores::cont::Error& e)
  {
    if (this->ForceVTKm)
    {
      vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
      return 0;
    }
    else
    {
      vtkWarningMacro(<< "Viskores error: " << e.GetMessage()
                      << "Falling back to serial implementation");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkmPointElevation::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
