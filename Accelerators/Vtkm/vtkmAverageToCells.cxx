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
#include "vtkmlib/Storage.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/CellAverage.h>

vtkStandardNewMacro(vtkmAverageToCells)

//------------------------------------------------------------------------------
vtkmAverageToCells::vtkmAverageToCells()
{
}

//------------------------------------------------------------------------------
vtkmAverageToCells::~vtkmAverageToCells()
{
}

//------------------------------------------------------------------------------
int vtkmAverageToCells::RequestData(vtkInformation* vtkNotUsed(request),
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output =
      vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  // grab the input array to process to determine the field we want to average
  int association = this->GetInputArrayAssociation(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkErrorMacro(<< "Must be asked to average a cell based field.");
    return 1;
  }

  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input);
  // convert the array over to vtkm
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  vtkm::cont::Field field = tovtkm::Convert(inputArray, association);

  const bool dataSetValid =
      in.GetNumberOfCoordinateSystems() > 0 && in.GetNumberOfCellSets() > 0;
  const bool fieldValid =
      (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS) &&
      (field.GetName() != std::string());
  if (!(dataSetValid && fieldValid))
  {
    vtkErrorMacro(<< "Unable convert dataset over to VTK-m for input.");
    return 0;
  }

  vtkmInputFilterPolicy policy;
  vtkm::filter::CellAverage filter;
  filter.SetOutputFieldName(field.GetName()); // should we expose this control?
  vtkm::filter::ResultField result = filter.Execute(in, field, policy);

  if (result.IsValid())
  {
    // convert back the dataset to VTK, and add the field as a cell field
    vtkDataArray* resultingArray = fromvtkm::Convert(result.GetField());
    output->GetCellData()->AddArray(resultingArray);
    resultingArray->FastDelete();
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkmAverageToCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
