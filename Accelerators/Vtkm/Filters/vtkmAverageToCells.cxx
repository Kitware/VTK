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
#include "vtkPointData.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/field_conversion/CellAverage.h>

vtkStandardNewMacro(vtkmAverageToCells);

//------------------------------------------------------------------------------
vtkmAverageToCells::vtkmAverageToCells() = default;

//------------------------------------------------------------------------------
vtkmAverageToCells::~vtkmAverageToCells() = default;

//------------------------------------------------------------------------------
int vtkmAverageToCells::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->CopyStructure(input);

  // Pass the cell data first. The fields and attributes
  // which also exist in the point data of the input will
  // be over-written during CopyAllocate
  output->GetCellData()->PassData(input->GetCellData());

  // Pass data if requested.
  if (!this->PassPointData)
  {
    output->GetPointData()->CopyAllOff();
    output->GetPointData()->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  output->GetPointData()->PassData(input->GetPointData());
  output->GetFieldData()->PassData(input->GetFieldData());

  if (input->GetNumberOfCells() < 1)
  {
    vtkWarningMacro(<< "No input cells");
    return 1;
  }

  try
  {
    if (this->CategoricalData)
    {
      throw vtkm::cont::ErrorFilterExecution("CategoricalData is not supported.");
    }

    // convert the input dataset to a vtkm::cont::DataSet
    vtkm::cont::DataSet in;
    if (this->ProcessAllArrays)
    {
      in = tovtkm::Convert(input, tovtkm::FieldsFlag::Points);
    }
    else
    {
      std::vector<const char*> pointArrayNames(this->GetNumberOfPointArraysToProcess());
      this->GetPointArraysToProcess(pointArrayNames.data());

      in = tovtkm::Convert(input);
      for (auto name : pointArrayNames)
      {
        auto array = input->GetPointData()->GetArray(name);
        if (!array)
        {
          vtkWarningMacro(<< name << " is not a data array.");
          continue;
        }
        auto field = tovtkm::Convert(array, vtkDataObject::FIELD_ASSOCIATION_POINTS);
        in.AddField(field);
      }
    }

    if (in.GetNumberOfFields() < 1) // `in` should only have point fields, if any
    {
      vtkWarningMacro(<< "No point arrays to process.");
      return 1;
    }

    // track attribute types to preserve them
    std::vector<int> attributeTypes(in.GetNumberOfFields(), -1);
    for (int i = 0; i < input->GetPointData()->GetNumberOfArrays(); ++i)
    {
      int at = input->GetPointData()->IsArrayAnAttribute(i);
      if (at == -1) // not an attribute, skip
      {
        continue;
      }

      const char* name = input->GetPointData()->GetArrayName(i);
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
    vtkm::filter::field_conversion::CellAverage filter;
    for (int i = 0; i < in.GetNumberOfFields(); ++i)
    {
      const auto& name = in.GetField(i).GetName();
      filter.SetActiveField(name, vtkm::cont::Field::Association::Points);
      auto result = filter.Execute(in);

      // convert back the dataset to VTK, and add the field as a point field
      vtkDataArray* resultingArray = fromvtkm::Convert(result.GetCellField(name));
      if (resultingArray == nullptr)
      {
        throw vtkm::cont::ErrorFilterExecution("Unable to convert result array from VTK-m to VTK");
      }

      int outIdx = output->GetCellData()->AddArray(resultingArray);
      if (attributeTypes[i] != -1)
      {
        output->GetCellData()->SetActiveAttribute(outIdx, attributeTypes[i]);
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
void vtkmAverageToCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
