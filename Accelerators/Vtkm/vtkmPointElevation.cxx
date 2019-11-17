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
#include "vtkmPointElevation.h"
#include "vtkmConfig.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/filter/PointElevation.h>

vtkStandardNewMacro(vtkmPointElevation);

//------------------------------------------------------------------------------
vtkmPointElevation::vtkmPointElevation() {}

//------------------------------------------------------------------------------
vtkmPointElevation::~vtkmPointElevation() {}

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
    // Convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::Points);

    vtkmInputFilterPolicy policy;
    // Setup input
    vtkm::filter::PointElevation filter;
    filter.SetLowPoint(this->LowPoint[0], this->LowPoint[1], this->LowPoint[2]);
    filter.SetHighPoint(this->HighPoint[0], this->HighPoint[1], this->HighPoint[2]);
    filter.SetRange(this->ScalarRange[0], this->ScalarRange[1]);
    filter.SetOutputFieldName("elevation");
    filter.SetUseCoordinateSystemAsField(true);
    auto result = filter.Execute(in, policy);

    // Convert the result back
    vtkDataArray* resultingArray = fromvtkm::Convert(result.GetField("elevation"));
    if (resultingArray == nullptr)
    {
      vtkErrorMacro(<< "Unable to convert result array from VTK-m to VTK");
      return 0;
    }
    output->GetPointData()->AddArray(resultingArray);
    output->GetPointData()->SetActiveScalars("elevation");
    resultingArray->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage() << "Falling back to serial implementation");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkmPointElevation::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
