//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "vtkmImageConnectivity.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/filter/ImageConnectivity.h>

vtkStandardNewMacro(vtkmImageConnectivity);

//------------------------------------------------------------------------------
vtkmImageConnectivity::vtkmImageConnectivity() {}

//------------------------------------------------------------------------------
vtkmImageConnectivity::~vtkmImageConnectivity() {}

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
    vtkm::filter::ImageConnectivity filter;
    filter.SetActiveField(inputArray->GetName(), vtkm::cont::Field::Association::POINTS);
    // the field should be named 'RegionId'
    filter.SetOutputFieldName("RegionId");

    // explicitly convert just the field we need
    auto inData = tovtkm::Convert(input, tovtkm::FieldsFlag::None);
    auto inField = tovtkm::Convert(inputArray, association);
    inData.AddField(inField);

    // don't pass this field
    filter.SetFieldsToPass(vtkm::filter::FieldSelection(vtkm::filter::FieldSelection::MODE_NONE));

    vtkm::cont::DataSet result;
    vtkmInputFilterPolicy policy;
    result = filter.Execute(inData, policy);

    // Make sure the output has all the fields / etc that the input has
    output->ShallowCopy(input);

    // convert back the regionId field to VTK
    if (!fromvtkm::ConvertArrays(result, output))
    {
      vtkWarningMacro(<< "Unable to convert VTKm DataSet back to VTK.\n"
                      << "Falling back to serial implementation.");
      return 0;
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }
  return 1;
}
