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
#include "vtkmExternalFaces.h"

#include "vtkCellData.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"


#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/Storage.h"
#include "vtkmlib/CellSetConverters.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/ExternalFaces.h>


vtkStandardNewMacro(vtkmExternalFaces)

//------------------------------------------------------------------------------
vtkmExternalFaces::vtkmExternalFaces()
  : CompactPoints(false)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkmExternalFaces::~vtkmExternalFaces()
{
}

//------------------------------------------------------------------------------
void vtkmExternalFaces::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkmExternalFaces::SetInputData(vtkUnstructuredGrid *ds)
{
  this->SetInputDataObject(0, ds);
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkmExternalFaces::GetOutput()
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
int vtkmExternalFaces::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmExternalFaces::FillOutputPortInformation(int vtkNotUsed(port),
                                                  vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmExternalFaces::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkmExternalFaces::RequestData(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input);
  if (in.GetNumberOfCoordinateSystems() <= 0 || in.GetNumberOfCellSets() <= 0)
  {
    vtkErrorMacro(<< "Could not convert vtk dataset to vtkm dataset");
    return 0;
  }

  vtkmInputFilterPolicy policy;

  // apply the filter
  vtkm::filter::ExternalFaces filter;
  filter.SetCompactPoints(this->CompactPoints);
  vtkm::filter::ResultDataSet result = filter.Execute(in, policy);
  if (!result.IsValid())
  {
    vtkErrorMacro(<< "VTKm ExternalFaces algorithm failed to run");
    return 0;
  }

  if (this->CompactPoints)
  {
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
  }
  else
  {
    // convert just the cellset from vtkm to vtk
    vtkNew<vtkCellArray> cells;
    vtkNew<vtkUnsignedCharArray> types;
    vtkNew<vtkIdTypeArray> locations;
    vtkm::cont::DynamicCellSet cellSet = result.GetDataSet().GetCellSet();
    if (!fromvtkm::Convert(cellSet, cells.GetPointer(), types.GetPointer(),
                           locations.GetPointer()))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }

    // copy points from input to output
    output->SetPoints(input->GetPoints());

    // add the new cellset to output
    output->SetCells(types.GetPointer(), locations.GetPointer(),
                     cells.GetPointer());

    // copy the point data from input to output
    output->GetPointData()->PassData(input->GetPointData());
  }

  return 1;
}
