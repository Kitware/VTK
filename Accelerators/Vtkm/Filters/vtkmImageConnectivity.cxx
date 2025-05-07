// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkmImageConnectivity.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"

#include <viskores/filter/connected_components/ImageConnectivity.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmImageConnectivity);

//------------------------------------------------------------------------------
vtkmImageConnectivity::vtkmImageConnectivity() = default;

//------------------------------------------------------------------------------
vtkmImageConnectivity::~vtkmImageConnectivity() = default;

//------------------------------------------------------------------------------
void vtkmImageConnectivity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkmImageConnectivity::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkImageData* output = static_cast<vtkImageData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* input = static_cast<vtkImageData*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS || inputArray == nullptr ||
    inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    vtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  try
  {
    viskores::filter::connected_components::ImageConnectivity filter;
    filter.SetActiveField(inputArray->GetName(), viskores::cont::Field::Association::Points);
    // the field should be named 'RegionId'
    filter.SetOutputFieldName("RegionId");

    // explicitly convert just the field we need
    auto inData = tovtkm::Convert(input, tovtkm::FieldsFlag::None);
    auto inField = tovtkm::Convert(inputArray, association);
    inData.AddField(inField);

    // don't pass this field
    filter.SetFieldsToPass(
      viskores::filter::FieldSelection(viskores::filter::FieldSelection::Mode::None));

    viskores::cont::DataSet result;
    result = filter.Execute(inData);

    // Make sure the output has all the fields / etc that the input has
    output->ShallowCopy(input);

    // convert back the regionId field to VTK
    if (!fromvtkm::ConvertArrays(result, output))
    {
      vtkWarningMacro(<< "Unable to convert Viskores DataSet back to VTK.\n"
                      << "Falling back to serial implementation.");
      return 0;
    }
  }
  catch (const viskores::cont::Error& e)
  {
    vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
    return 0;
  }
  return 1;
}
VTK_ABI_NAMESPACE_END
