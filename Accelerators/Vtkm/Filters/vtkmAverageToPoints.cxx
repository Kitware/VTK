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
#include "vtkmAverageToPoints.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/field_conversion/PointAverage.h>

vtkStandardNewMacro(vtkmAverageToPoints);

//------------------------------------------------------------------------------
vtkmAverageToPoints::vtkmAverageToPoints() = default;

//------------------------------------------------------------------------------
vtkmAverageToPoints::~vtkmAverageToPoints() = default;

//------------------------------------------------------------------------------
int vtkmAverageToPoints::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->CopyStructure(input);

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->PassData(input->GetPointData());

  if (!this->PassCellData)
  {
    output->GetCellData()->CopyAllOff();
    output->GetCellData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  if (input->GetNumberOfPoints() < 1)
  {
    vtkWarningMacro(<< "No input points");
    return 1;
  }

  try
  {
    if ((input->IsA("vtkUnstructuredGrid") || input->IsA("vtkPolyData")) &&
      this->ContributingCellOption != vtkCellDataToPointData::All)
    {
      throw vtkm::cont::ErrorFilterExecution("Only `All` is supported for ContributingCellOption.");
    }

    vtkStructuredGrid* sGrid = vtkStructuredGrid::SafeDownCast(input);
    vtkUniformGrid* uniformGrid = vtkUniformGrid::SafeDownCast(input);
    if ((sGrid && sGrid->HasAnyBlankCells()) || (uniformGrid && uniformGrid->HasAnyBlankCells()))
    {
      throw vtkm::cont::ErrorFilterExecution("Processing blank cells is not supported.");
    }

    // convert the input dataset to a vtkm::cont::DataSet
    vtkm::cont::DataSet in;
    if (this->ProcessAllArrays)
    {
      in = tovtkm::Convert(input, tovtkm::FieldsFlag::Cells);
    }
    else
    {
      std::vector<const char*> cellArrayNames(this->GetNumberOfCellArraysToProcess());
      this->GetCellArraysToProcess(cellArrayNames.data());

      in = tovtkm::Convert(input);
      for (auto name : cellArrayNames)
      {
        auto array = input->GetCellData()->GetArray(name);
        if (!array)
        {
          vtkWarningMacro(<< name << " is not a data array.");
          continue;
        }
        auto field = tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_CELLS);
        in.AddField(field);
      }
    }

    if (in.GetNumberOfFields() < 1) // `in` should only have cell fields, if any
    {
      vtkWarningMacro(<< "No cell arrays to process.");
      return 1;
    }

    // track attribute types to preserve them
    std::vector<int> attributeTypes(in.GetNumberOfFields(), -1);
    for (int i = 0; i < input->GetCellData()->GetNumberOfArrays(); ++i)
    {
      int at = input->GetCellData()->IsArrayAnAttribute(i);
      if (at == -1) // not an attribute, skip
      {
        continue;
      }

      const char* name = input->GetCellData()->GetArrayName(i);
      if (!name || (name[0] == '\0'))
      {
        name = tovtkm::NoNameVTKFieldName();
      }

      auto idx = in.GetFieldIndex(name);
      if (idx != -1)
      {
        attributeTypes[idx] = at;
      }
    }

    // Execute the vtk-m filter
    vtkm::filter::field_conversion::PointAverage filter;
    for (int i = 0; i < in.GetNumberOfFields(); ++i)
    {
      const auto& name = in.GetField(i).GetName();
      filter.SetActiveField(name, vtkm::cont::Field::Association::Cells);
      auto result = filter.Execute(in);

      // convert back the dataset to VTK, and add the field as a point field
      vtkDataArray* resultingArray = fromvtkm::Convert(result.GetPointField(name));
      if (resultingArray == nullptr)
      {
        throw vtkm::cont::ErrorFilterExecution("Unable to convert result array from VTK-m to VTK");
      }

      int outIdx = output->GetPointData()->AddArray(resultingArray);
      if (attributeTypes[i] != -1)
      {
        output->GetPointData()->SetActiveAttribute(outIdx, attributeTypes[i]);
      }
      resultingArray->FastDelete();
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkWarningMacro(<< "VTK-m failed with message: " << e.GetMessage() << "\n"
                    << "Falling back to the default VTK implementation.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmAverageToPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
