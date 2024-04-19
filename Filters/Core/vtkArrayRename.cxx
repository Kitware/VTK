// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayRename.h"

#include "vtkCompositeDataSetRange.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkArrayRename);

//------------------------------------------------------------------------------
void vtkArrayRename::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  for (const auto& typeMap : this->ArrayMapping)
  {
    os << indent << "ArrayMapping for " << vtkDataObject::GetAssociationTypeAsString(typeMap.first)
       << std::endl;
    vtkIndent next = indent.GetNextIndent();
    for (const auto& arrayMap : typeMap.second)
    {
      os << next << arrayMap.first << " -> " << arrayMap.second << std::endl;
    }
  }
}

//------------------------------------------------------------------------------
int vtkArrayRename::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    // Skip composite data sets so that executives will treat this as a simple filter
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
void vtkArrayRename::ClearAll()
{
  vtkDebugMacro(<< "Clearing all mapping");
  this->ArrayMapping.clear();
}

//------------------------------------------------------------------------------
void vtkArrayRename::ClearMapping(int attributeType)
{
  vtkDebugMacro(<< "Clearing mapping for ");
  this->ArrayMapping[attributeType].clear();
}

//------------------------------------------------------------------------------
void vtkArrayRename::SetArrayName(int attributeType, int idx, const char* newName)
{
  const char* originalName = this->GetArrayOriginalName(attributeType, idx);
  this->SetArrayName(attributeType, originalName, newName);
}

//------------------------------------------------------------------------------
void vtkArrayRename::SetArrayName(int attributeType, const char* inputName, const char* newName)
{
  if (strcmp(newName, "") == 0)
  {
    vtkWarningMacro(<< "Setting an empty name is not allowed, aborting");
    return;
  }

  vtkDebugMacro(<< "Setting " << inputName << " "
                << vtkDataObject::GetAssociationTypeAsString(attributeType) << " array name to "
                << newName);
  this->ArrayMapping[attributeType][inputName] = newName;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkArrayRename::GetNumberOfArrays(int attributeType)
{
  auto input = this->GetInput();
  vtkFieldData* inFd = input->GetAttributesAsFieldData(attributeType);
  if (!inFd)
  {
    return 0;
  }

  return inFd->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkArrayRename::GetArrayOriginalName(int attributeType, int idx)
{
  auto input = this->GetInput();
  vtkFieldData* inFd = input->GetAttributesAsFieldData(attributeType);
  if (inFd && inFd->GetNumberOfArrays() > idx)
  {
    return inFd->GetAbstractArray(idx)->GetName();
  }

  return "";
}

//------------------------------------------------------------------------------
const char* vtkArrayRename::GetArrayNewName(int attributeType, int idx)
{
  const char* originalName = this->GetArrayOriginalName(attributeType, idx);
  if (this->ArrayMapping[attributeType].find(originalName) ==
    this->ArrayMapping[attributeType].end())
  {
    vtkWarningMacro(<< "Array not found in input");
    return "";
  }

  vtkDebugMacro(<< "Returning " << originalName << " "
                << vtkDataObject::GetAssociationTypeAsString(attributeType) << " array name as "
                << this->ArrayMapping[attributeType][originalName]);

  return this->ArrayMapping[attributeType][originalName].c_str();
}

//------------------------------------------------------------------------------
int vtkArrayRename::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output objects
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  bool abort = false;

  for (int type = vtkDataObject::POINT; type < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES && !abort;
       type++)
  {
    if (type == vtkDataObject::POINT_THEN_CELL)
    {
      continue;
    }

    vtkFieldData* inFd = input->GetAttributesAsFieldData(type);
    vtkFieldData* outFd = output->GetAttributesAsFieldData(type);
    if (!inFd || !outFd)
    {
      continue;
    }

    vtkFieldData::Iterator arrayIterator(inFd);
    vtkIdType checkAbortInterval =
      std::min(input->GetNumberOfElements(type) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType idx = arrayIterator.BeginIndex(); idx != -1; idx = arrayIterator.NextIndex())
    {
      if (idx % checkAbortInterval == 0 && this->CheckAbort())
      {
        abort = true;
        break;
      }
      vtkAbstractArray* array = inFd->GetAbstractArray(idx);
      vtkAbstractArray* newArray = array->NewInstance();

      vtkDataArray* dataArray = vtkDataArray::SafeDownCast(array);
      vtkDataArray* newDataArray = vtkDataArray::SafeDownCast(newArray);
      // Shallow copy if possible, deep copy string arrays.
      if (dataArray)
      {
        newDataArray->ShallowCopy(dataArray);
      }
      else if (vtkStringArray::SafeDownCast(newArray))
      {
        newArray->DeepCopy(array);
      }

      std::map<std::string, std::string>& fieldMap = this->ArrayMapping[type];
      std::string previousName = array->GetName();
      if (fieldMap.find(previousName) != fieldMap.end())
      {
        vtkDebugMacro(<< "Renaming " << previousName << " into " << fieldMap[previousName]);
        newArray->SetName(fieldMap[previousName].c_str());
        outFd->RemoveArray(previousName.c_str());
        if (outFd->HasArray(fieldMap[previousName].c_str()))
        {
          vtkWarningMacro(<< "Array name " << fieldMap[previousName]
                          << " already in use. Overwriting an array.");
        }
        outFd->AddArray(newArray);
      }
      newArray->Delete();
    }
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
