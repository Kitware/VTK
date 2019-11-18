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

#include "vtkmFilterPolicy.h"

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/CleanGrid.hxx>

vtkStandardNewMacro(vtkmCleanGrid);

//------------------------------------------------------------------------------
vtkmCleanGrid::vtkmCleanGrid()
  : CompactPoints(false)
{
}

//------------------------------------------------------------------------------
vtkmCleanGrid::~vtkmCleanGrid() {}

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
    // convert the input dataset to a vtkm::cont::DataSet
    auto fieldsFlag = this->CompactPoints ? tovtkm::FieldsFlag::Points : tovtkm::FieldsFlag::None;
    vtkm::cont::DataSet in = tovtkm::Convert(input, fieldsFlag);

    // apply the filter
    vtkmInputFilterPolicy policy;
    vtkm::filter::CleanGrid filter;
    filter.SetCompactPointFields(this->CompactPoints);
    auto result = filter.Execute(in, policy);

    // convert back to vtkDataSet (vtkUnstructuredGrid)
    if (!fromvtkm::Convert(result, output, input))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
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
