// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFieldDataToDataSetAttribute.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h"
#include "vtkConstantArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataArrayMeta.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFieldDataToDataSetAttribute);

//------------------------------------------------------------------------------
int vtkFieldDataToDataSetAttribute::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    // Skip composite data sets so that executives will handle this automatically.
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkFieldDataToDataSetAttribute::AddFieldDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->FieldDataArrays.insert(std::string(name));
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkFieldDataToDataSetAttribute::RemoveFieldDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->FieldDataArrays.erase(name);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkFieldDataToDataSetAttribute::ClearFieldDataArrays()
{
  if (!this->FieldDataArrays.empty())
  {
    this->Modified();
  }
  this->FieldDataArrays.clear();
}

//------------------------------------------------------------------------------
const std::set<std::string>& vtkFieldDataToDataSetAttribute::GetFieldDataArrays()
{
  return this->FieldDataArrays;
}

//------------------------------------------------------------------------------
struct ArrayForwarder
{
  vtkSmartPointer<vtkDataArray> Result;
  vtkIdType NumberOfTuples;

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    using SourceT = vtk::GetAPIType<ArrayType>;
    auto output = vtkSmartPointer<vtkConstantArray<SourceT>>::New();

    vtkDataArrayAccessor<ArrayType> access(array);
    output->ConstructBackend(access.Get(0, 0));
    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(this->NumberOfTuples);
    output->SetName(array->GetName());
    this->Result = output;
  }
};

//------------------------------------------------------------------------------
int vtkFieldDataToDataSetAttribute::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  vtkFieldData* inputFieldData = input->GetFieldData();
  vtkDataSetAttributes* outAttribute = output->GetAttributes(this->OutputFieldType);

  vtkSmartPointer<vtkFieldData> originFieldData;
  if (!this->ProcessAllArrays)
  {
    // create a vtkFieldData containing just the selected arrays.
    originFieldData = vtkSmartPointer<vtkFieldData>::New();

    for (const auto& name : this->FieldDataArrays)
    {
      vtkAbstractArray* arr = inputFieldData->GetAbstractArray(name.c_str());
      if (arr == nullptr)
      {
        vtkWarningMacro("field data array not found: " << name);
        continue;
      }
      originFieldData->AddArray(arr);
    }
  }
  else
  {
    originFieldData = inputFieldData;
  }

  ArrayForwarder forwarder;
  forwarder.NumberOfTuples = outAttribute->GetNumberOfTuples();

  int nbOfArrays = originFieldData->GetNumberOfArrays();
  for (int idx = 0; idx < nbOfArrays; idx++)
  {
    vtkDataArray* dataArray = originFieldData->GetArray(idx);
    if (!dataArray)
    {
      continue;
    }
    if (vtkArrayDispatch::Dispatch::Execute(dataArray, forwarder))
    {
      outAttribute->AddArray(forwarder.Result);
    }
    else if (auto stringArray =
               vtkStringArray::SafeDownCast(originFieldData->GetAbstractArray(idx)))
    {
      vtkWarningMacro("string arrays are not supported, skipping " << stringArray->GetName());
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkFieldDataToDataSetAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "OutputFieldType: " << vtkDataObject::GetAssociationTypeAsString(this->OutputFieldType)
     << "\n";
  os << indent << "ProcessAllArrays" << (this->ProcessAllArrays ? "On\n" : "Off\n");
  os << indent << "FieldDataArrays: \n";
  for (const auto& array : this->FieldDataArrays)
  {
    os << indent << array << "\n";
  }
}
VTK_ABI_NAMESPACE_END
