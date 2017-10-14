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
  vtkm::cont::DataSet in = tovtkm::Convert(input,
                                           tovtkm::FieldsFlag::PointsAndCells);

  // we need to map the given property to the data set
  int association = this->GetInputArrayAssociation(0, inputVector);
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  vtkm::cont::Field inField = tovtkm::Convert(inputArray, association);

  const bool dataSetValid =
      in.GetNumberOfCoordinateSystems() > 0 && in.GetNumberOfCellSets() > 0;
  const bool fieldValid =
      (inField.GetAssociation() != vtkm::cont::Field::ASSOC_ANY);

  vtkm::filter::Result result;
  bool convertedDataSet = false;
  if (dataSetValid && fieldValid)
  {
    vtkmInputFilterPolicy policy;
    result = filter.Execute(in, inField, policy);

    // convert other scalar arrays
    if (result.IsDataSetValid())
    {
      vtkm::Id numFields = static_cast<vtkm::Id>(in.GetNumberOfFields());
      for (vtkm::Id fieldIdx = 0; fieldIdx < numFields; ++fieldIdx)
      {
        const vtkm::cont::Field &field = in.GetField(fieldIdx);
        try
        {
          filter.MapFieldOntoOutput(result, field, policy);
        }
        catch (vtkm::cont::Error &e)
        {
          vtkWarningMacro(<< "Unable to use VTKm to convert field( "
                          << field.GetName() << " ) to the Threshold"
                          << " output: " << e.what());
        }
      }

      // now we are done the algorithm and conversion of arrays so
      // convert back the dataset to VTK
      convertedDataSet = fromvtkm::Convert(result.GetDataSet(), output, input);
    }
  }

  if (!result.IsDataSetValid() || !convertedDataSet)
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
