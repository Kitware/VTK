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
#include "vtkmAverageToCells.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/filter/CellAverage.h>
#include <vtkm/filter/CellAverage.hxx>

vtkStandardNewMacro(vtkmAverageToCells);

//------------------------------------------------------------------------------
vtkmAverageToCells::vtkmAverageToCells() {}

//------------------------------------------------------------------------------
vtkmAverageToCells::~vtkmAverageToCells() {}

//------------------------------------------------------------------------------
int vtkmAverageToCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  // grab the input array to process to determine the field we want to average
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS || fieldArray == nullptr ||
    fieldArray->GetName() == nullptr || fieldArray->GetName()[0] == '\0')
  {
    vtkErrorMacro(<< "Invalid field: Requires a point field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    // convert the input dataset to a vtkm::cont::DataSet
    vtkm::cont::DataSet in = tovtkm::Convert(input);
    auto field = tovtkm::Convert(fieldArray, association);
    in.AddField(field);

    vtkmInputFilterPolicy policy;
    vtkm::filter::CellAverage filter;
    filter.SetActiveField(fieldName, vtkm::cont::Field::Association::POINTS);
    filter.SetOutputFieldName(fieldName); // should we expose this control?

    auto result = filter.Execute(in, policy);

    // convert back the dataset to VTK, and add the field as a cell field
    vtkDataArray* resultingArray = fromvtkm::Convert(result.GetCellField(fieldName));
    if (resultingArray == nullptr)
    {
      vtkErrorMacro(<< "Unable to convert result array from VTK-m to VTK");
      return 0;
    }

    output->GetCellData()->AddArray(resultingArray);
    resultingArray->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmAverageToCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
