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
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/filter/Threshold.h>
#include <vtkm/filter/Threshold.hxx>

vtkStandardNewMacro(vtkmThreshold);

//------------------------------------------------------------------------------
vtkmThreshold::vtkmThreshold() {}

//------------------------------------------------------------------------------
vtkmThreshold::~vtkmThreshold() {}

//------------------------------------------------------------------------------
int vtkmThreshold::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (inputArray == nullptr || inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    vtkErrorMacro("Invalid input array.");
    return 0;
  }

  try
  {
    // convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);

    vtkmInputFilterPolicy policy;
    vtkm::filter::Threshold filter;
    filter.SetActiveField(inputArray->GetName());
    filter.SetLowerThreshold(this->GetLowerThreshold());
    filter.SetUpperThreshold(this->GetUpperThreshold());
    auto result = filter.Execute(in, policy);

    // now we are done the algorithm and conversion of arrays so
    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkWarningMacro(<< "VTK-m error: " << e.GetMessage()
                    << "Falling back to serial implementation");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
