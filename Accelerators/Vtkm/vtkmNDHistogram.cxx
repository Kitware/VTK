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
#include "vtkmNDHistogram.h"
#include "vtkmConfig.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSparseArray.h"
#include "vtkTable.h"
#include "vtkmFilterPolicy.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include <vtkm/filter/NDHistogram.h>

vtkStandardNewMacro(vtkmNDHistogram);

//------------------------------------------------------------------------------
void vtkmNDHistogram::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldNames: "
     << "\n";
  for (const auto& fieldName : FieldNames)
  {
    os << indent << fieldName << " ";
  }
  os << indent << "\n";
  os << indent << "NumberOfBins: "
     << "\n";
  for (const auto& nob : NumberOfBins)
  {
    os << indent << nob << " ";
  }
  os << indent << "\n";
  os << indent << "BinDeltas: "
     << "\n";
  for (const auto& bd : BinDeltas)
  {
    os << indent << bd << " ";
  }
  os << indent << "\n";
  os << indent << "DataRanges: "
     << "\n";
  for (const auto& dr : DataRanges)
  {
    os << indent << dr.first << " " << dr.second << " ";
  }
  os << indent << "\n";
}

//------------------------------------------------------------------------------
vtkmNDHistogram::vtkmNDHistogram() {}

//------------------------------------------------------------------------------
vtkmNDHistogram::~vtkmNDHistogram() {}

//------------------------------------------------------------------------------
void vtkmNDHistogram::AddFieldAndBin(const std::string& fieldName, const vtkIdType& numberOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumberOfBins.push_back(numberOfBins);
  this->SetInputArrayToProcess(static_cast<int>(this->FieldNames.size()), 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName.c_str());
}

//------------------------------------------------------------------------------
double vtkmNDHistogram::GetBinDelta(size_t fieldIndex)
{
  return this->BinDeltas[fieldIndex];
}

//------------------------------------------------------------------------------
std::pair<double, double> vtkmNDHistogram::GetDataRange(size_t fieldIndex)
{
  return this->DataRanges[fieldIndex];
}

//------------------------------------------------------------------------------
int vtkmNDHistogram::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmNDHistogram::GetFieldIndexFromFieldName(const std::string& fieldName)
{
  auto iter = std::find(this->FieldNames.begin(), this->FieldNames.end(), fieldName);
  return (iter == std::end(this->FieldNames)) ? -1
                                              : static_cast<int>(iter - this->FieldNames.begin());
}

//------------------------------------------------------------------------------
int vtkmNDHistogram::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkArrayData* output = vtkArrayData::GetData(outputVector, 0);
  output->ClearArrays();

  try
  {
    vtkm::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);

    vtkmInputFilterPolicy policy;
    vtkm::filter::NDHistogram filter;
    for (size_t i = 0; i < this->FieldNames.size(); i++)
    {
      filter.AddFieldAndBin(this->FieldNames[i], this->NumberOfBins[i]);
    }
    vtkm::cont::DataSet out = filter.Execute(in, policy);

    vtkm::Id numberOfFields = out.GetNumberOfFields();
    this->BinDeltas.clear();
    this->DataRanges.clear();
    this->BinDeltas.reserve(static_cast<size_t>(numberOfFields));
    this->DataRanges.reserve(static_cast<size_t>(numberOfFields));

    // Fetch the field array out of the vtkm filter result
    size_t index = 0;
    std::vector<vtkDataArray*> fArrays;
    for (auto& fn : this->FieldNames)
    {
      vtkDataArray* fnArray = fromvtkm::Convert(out.GetField(fn));
      fnArray->SetName(fn.c_str());
      fArrays.push_back(fnArray);
      this->BinDeltas.push_back(filter.GetBinDelta(index));
      this->DataRanges.push_back(
        std::make_pair(filter.GetDataRange(index).Min, filter.GetDataRange(index).Max));
      index++;
    }
    vtkDataArray* frequencyArray = fromvtkm::Convert(out.GetField("Frequency"));
    frequencyArray->SetName("Frequency");

    // Create the sparse array
    vtkSparseArray<double>* sparseArray = vtkSparseArray<double>::New();
    vtkArrayExtents sae; // sparse array extent
    size_t ndims(fArrays.size());
    sae.SetDimensions(static_cast<vtkArrayExtents::DimensionT>(ndims));
    for (size_t i = 0; i < ndims; i++)
    {
      sae[static_cast<vtkArrayExtents::DimensionT>(i)] =
        vtkArrayRange(0, fArrays[i]->GetNumberOfValues());
    }
    sparseArray->Resize(sae);

    // Set the dimension label
    for (size_t i = 0; i < ndims; i++)
    {
      sparseArray->SetDimensionLabel(static_cast<vtkIdType>(i), fArrays[i]->GetName());
    }
    // Fill in the sparse array
    for (vtkIdType i = 0; i < frequencyArray->GetNumberOfValues(); i++)
    {
      vtkArrayCoordinates coords;
      coords.SetDimensions(static_cast<vtkArrayCoordinates::DimensionT>(ndims));
      for (size_t j = 0; j < ndims; j++)
      {
        coords[static_cast<vtkArrayCoordinates::DimensionT>(j)] = fArrays[j]->GetComponent(i, 0);
      }
      sparseArray->SetValue(coords, frequencyArray->GetComponent(i, 0));
    }
    output->AddArray(sparseArray);

    // Clean up the memory
    for (auto& fArray : fArrays)
    {
      fArray->FastDelete();
    }
    frequencyArray->FastDelete();
    sparseArray->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }
  return 1;
}
