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
#include "vtkmlib/Storage.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/CleanGrid.h>


vtkStandardNewMacro(vtkmCleanGrid)

//------------------------------------------------------------------------------
vtkmCleanGrid::vtkmCleanGrid()
: CompactPoints(false)
{
}

//------------------------------------------------------------------------------
vtkmCleanGrid::~vtkmCleanGrid()
{
}

//------------------------------------------------------------------------------
void vtkmCleanGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CompactPoints: " << (this->CompactPoints ? "On" : "Off")
     << "\n";
}

//------------------------------------------------------------------------------
int vtkmCleanGrid::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmCleanGrid::RequestData(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input =
    vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input);
  if (in.GetNumberOfCoordinateSystems() <= 0 || in.GetNumberOfCellSets() <= 0)
  {
    vtkErrorMacro(<< "Could not convert vtk dataset to vtkm dataset");
    return 0;
  }


  // apply the filter
  vtkmInputFilterPolicy policy;

  vtkm::filter::CleanGrid filter;
  filter.SetCompactPointFields(this->CompactPoints);
  vtkm::filter::ResultDataSet result = filter.Execute(in, policy);
  if (!result.IsValid())
  {
    vtkErrorMacro(<< "VTKm CleanGrid algorithm failed to run");
    return 0;
  }

  // map each point array
  vtkPointData* pd = input->GetPointData();
  for (vtkIdType i = 0; i < pd->GetNumberOfArrays(); i++)
  {
    vtkDataArray* array = pd->GetArray(i);
    if (array == NULL)
    {
      continue;
    }

    vtkm::cont::Field pfield =
      tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_POINTS);
    try
    {
      filter.MapFieldOntoOutput(result, pfield, policy);
    }
    catch (vtkm::cont::Error&)
    {
      vtkWarningMacro(<< "Unable to use VTKm to convert field ("
      << array->GetName() << ") to the output");
    }
  }

  // convert back to vtkDataSet (vtkUnstructuredGrid)
  if (!fromvtkm::Convert(result.GetDataSet(), output, input))
  {
    vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
    return 0;
  }
  // pass cell data
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}
