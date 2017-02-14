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
#include "vtkmThreshold.h"
#include "vtkmConfig.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/Storage.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/Threshold.h>

vtkStandardNewMacro(vtkmThreshold)

namespace
{

  template <typename FilterType, typename PolicyType>
  bool convert_fields(FilterType & filter, vtkm::filter::ResultDataSet & result,
                      const PolicyType& policy, vtkFieldData* fields,
                      int association)
  {
    for (vtkIdType i = 0; i < fields->GetNumberOfArrays(); i++)
    {
      vtkDataArray* array = fields->GetArray(i);
      if (array == NULL)
      {
        continue;
      }

      vtkm::cont::Field f = tovtkm::Convert(array, association);
      try
      {
        filter.MapFieldOntoOutput(result, f, policy);
      }
      catch (vtkm::cont::Error&)
      { // todo: should signal we had an issue converting
      }
    }
    return true;
  }
}

//------------------------------------------------------------------------------
vtkmThreshold::vtkmThreshold()
{
}

//------------------------------------------------------------------------------
vtkmThreshold::~vtkmThreshold()
{
}

//------------------------------------------------------------------------------
int vtkmThreshold::RequestData(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkm::filter::Threshold filter;

  // set local variables
  filter.SetLowerThreshold(this->GetLowerThreshold());
  filter.SetUpperThreshold(this->GetUpperThreshold());

  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input);

  // we need to map the given property to the data set
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  vtkm::cont::Field field = tovtkm::Convert(inputArray, association);

  const bool dataSetValid =
      in.GetNumberOfCoordinateSystems() > 0 && in.GetNumberOfCellSets() > 0;
  const bool fieldValid =
      (field.GetAssociation() != vtkm::cont::Field::ASSOC_ANY);

  vtkm::filter::ResultDataSet result;
  bool convertedDataSet = false;
  if (dataSetValid && fieldValid)
  {
    vtkmInputFilterPolicy policy;
    result = filter.Execute(in, field, policy);

    // convert other scalar arrays
    if (result.IsValid())
    {
      // now convert point & cell data
      vtkPointData* pd = input->GetPointData();
      vtkCellData* cd = input->GetCellData();

      convert_fields(filter, result, policy, pd,
                     vtkDataObject::FIELD_ASSOCIATION_POINTS);
      convert_fields(filter, result, policy, cd,
                     vtkDataObject::FIELD_ASSOCIATION_CELLS);

      // now we are done the algorithm and conversion of arrays so
      // convert back the dataset to VTK
      convertedDataSet = fromvtkm::Convert(result.GetDataSet(), output, input);
    }
  }

  if (!result.IsValid() || !convertedDataSet)
  {
    vtkWarningMacro(<< "Could not use VTKm to generate threshold. "
                    << "Falling back to serial implementation.");

    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
