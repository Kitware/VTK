// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStatisticalModel.h"

#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkStatisticalModel);

vtkStatisticalModel::vtkStatisticalModel() = default;

vtkStatisticalModel::~vtkStatisticalModel()
{
  this->SetAlgorithmParameters(nullptr);
}

void vtkStatisticalModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AlgorithmParameters: \""
     << (this->AlgorithmParameters ? this->AlgorithmParameters : "(empty)") << "\"\n";
  os << indent << "ModelTables (" << this->ModelTables.size() << ")\n";
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  vtkIndent i4 = i3.GetNextIndent();
  for (const auto& tableTypeEntry : this->ModelTables)
  {
    os << i2 << vtkStatisticalModel::GetTableTypeName(tableTypeEntry.first) << " tables ("
       << tableTypeEntry.second.size() << ")\n";
    int ii = 0;
    for (const auto& table : tableTypeEntry.second)
    {
      if (table)
      {
        auto tableName = this->GetTableName(tableTypeEntry.first, ii);
        os << i3 << "Table: " << ii << " \"" << tableName << "\"\n";
        table->PrintSelf(os, i4);
      }
      else
      {
        os << i3 << "Table: " << ii << " is empty\n";
      }
      ++ii;
    }
  }
}

vtkMTimeType vtkStatisticalModel::GetMTime()
{
  vtkMTimeType result = this->Superclass::GetMTime();
  for (const auto& entry : this->ModelTables)
  {
    for (const auto& table : entry.second)
    {
      vtkMTimeType tableTime = table->GetMTime();
      result = std::max(tableTime, result);
    }
  }
  return result;
}

void vtkStatisticalModel::Initialize()
{
  bool mod = !!this->AlgorithmParameters || !this->ModelTables.empty();
  this->SetAlgorithmParameters(nullptr);
  this->ModelTables.clear();
  if (mod)
  {
    this->Modified();
  }
}

bool vtkStatisticalModel::IsEmpty()
{
  if (this->ModelTables.empty() && !this->AlgorithmParameters)
  {
    return true;
  }
  if (this->AlgorithmParameters && this->AlgorithmParameters[0])
  {
    return false;
  }
  // We might have allocated for tables but not set any. Verify:
  for (const auto& tableTypeEntry : this->ModelTables)
  {
    for (const auto& table : tableTypeEntry.second)
    {
      if (!!table)
      {
        return false;
      }
    }
  }
  return true;
}

unsigned long vtkStatisticalModel::GetActualMemorySize()
{
  unsigned long result = 0;
  int numNodes = 0;
  for (const auto& tableTypeEntry : this->ModelTables)
  {
    ++numNodes;
    for (const auto& table : tableTypeEntry.second)
    {
      ++numNodes;
      if (table)
      {
        result += table->GetActualMemorySize();
      }
    }
  }
  result += numNodes * 3 * sizeof(void*) / 1024; // (estimate size of TableMap)
  result += static_cast<unsigned long>(
    (this->AlgorithmParameters ? strlen(this->AlgorithmParameters) : 0) / 1024);
  return result;
}

void vtkStatisticalModel::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }

  if (!src)
  {
    return;
  }

  if (auto* source = vtkStatisticalModel::SafeDownCast(src))
  {
    this->Superclass::ShallowCopy(source);
    this->SetAlgorithmParameters(source->GetAlgorithmParameters());
    this->ModelTables = source->ModelTables;
    this->Modified();
  }
  else
  {
    vtkErrorMacro(
      "Can only copy another vtkStatisticalModel, but was passed " << src->GetClassName() << ".");
  }
}

void vtkStatisticalModel::DeepCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }

  if (auto* source = vtkStatisticalModel::SafeDownCast(src))
  {
    this->Superclass::DeepCopy(source);
    this->SetAlgorithmParameters(source->GetAlgorithmParameters());
    this->ModelTables.clear();
    for (const auto& tableTypeEntry : source->ModelTables)
    {
      this->SetNumberOfTables(tableTypeEntry.first, static_cast<int>(tableTypeEntry.second.size()));
      int ii = 0;
      for (const auto& table : tableTypeEntry.second)
      {
        if (table)
        {
          auto tname = source->GetTableName(tableTypeEntry.first, ii);
          auto tableCopy = vtkSmartPointer<vtkTable>::New();
          tableCopy->DeepCopy(table);
          this->SetTable(tableTypeEntry.first, ii, tableCopy, tname);
        }
        ++ii;
      }
    }
    this->Modified();
  }
  else
  {
    vtkErrorMacro("Can only copy another vtkStatisticalModel.");
  }
}

const char* vtkStatisticalModel::GetTableTypeName(int tableType)
{
  switch (tableType)
  {
    case TableType::Learned:
      return "Learned";
    case TableType::Derived:
      return "Derived";
    case TableType::Test:
      return "Test";
    default:
      break;
  }
  return "(none)";
}

int vtkStatisticalModel::GetTableTypeValue(const std::string& tableType)
{
  if (tableType == "Learned")
  {
    return TableType::Learned;
  }
  else if (tableType == "Derived")
  {
    return TableType::Derived;
  }
  else if (tableType == "Test")
  {
    return TableType::Test;
  }
  return -1;
}

vtkStatisticalModel* vtkStatisticalModel::GetData(vtkInformation* info)
{
  return info ? vtkStatisticalModel::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

vtkStatisticalModel* vtkStatisticalModel::GetData(vtkInformationVector* vv, int ii)
{
  return vtkStatisticalModel::GetData(vv->GetInformationObject(ii));
}

int vtkStatisticalModel::GetNumberOfTables()
{
  int numberOfTables = 0;
  for (int ii = 0; ii < this->GetNumberOfTableTypes(); ++ii)
  {
    numberOfTables += this->GetNumberOfTables(ii);
  }
  return numberOfTables;
}

int vtkStatisticalModel::GetNumberOfTables(int type)
{
  if (type < TableType::Learned || type > TableType::Test)
  {
    return 0;
  }

  auto it = this->ModelTables.find(type);
  if (it == this->ModelTables.end())
  {
    return 0;
  }
  return static_cast<int>(it->second.size());
}

vtkTable* vtkStatisticalModel::GetTable(int type, int index)
{
  if (type < TableType::Learned || type > TableType::Test)
  {
    return nullptr;
  }
  auto it = this->ModelTables.find(type);
  if (it == this->ModelTables.end())
  {
    return nullptr;
  }
  if (index < 0 || index >= static_cast<int>(it->second.size()))
  {
    return nullptr;
  }
  return it->second[index].GetPointer();
}

std::string vtkStatisticalModel::GetTableName(int type, int index)
{
  std::string name;
  auto* table = this->GetTable(type, index);
  if (!table)
  {
    return name;
  }
  auto* info = table->GetInformation();
  if (!info)
  {
    return name;
  }
  name = info->Get(vtkCompositeDataSet::NAME());
  return name;
}

vtkTable* vtkStatisticalModel::FindTableByName(int type, const std::string& tableName)
{
  int index;
  return this->FindTableByName(type, tableName, index);
}

vtkTable* vtkStatisticalModel::FindTableByName(int type, const std::string& tableName, int& index)
{
  if (type < TableType::Learned || type > TableType::Test)
  {
    return nullptr;
  }
  auto it = this->ModelTables.find(type);
  if (it == this->ModelTables.end())
  {
    return nullptr;
  }
  int ii = 0;
  for (const auto& table : it->second)
  {
    if (table)
    {
      if (auto* info = table->GetInformation())
      {
        if (info->Has(vtkCompositeDataSet::NAME()))
        {
          if (info->Get(vtkCompositeDataSet::NAME()) == tableName)
          {
            index = ii;
            return table.GetPointer();
          }
        }
      }
    }
    ++ii;
  }
  return nullptr;
}

bool vtkStatisticalModel::SetNumberOfTables(int type, int number)
{
  if (type < TableType::Learned || type > TableType::Test)
  {
    return false;
  }
  auto it = this->ModelTables.find(type);
  if (it == this->ModelTables.end())
  {
    this->ModelTables[type].resize(number);
    this->Modified();
    return true;
  }
  if (number < 0 || number == static_cast<int>(it->second.size()))
  {
    return false;
  }
  it->second.resize(number);
  this->Modified();
  return true;
}

bool vtkStatisticalModel::SetTable(
  int type, int index, vtkTable* table, const std::string& tableName)
{
  auto it = this->ModelTables.find(type);
  if (it == this->ModelTables.end())
  {
    // Call SetNumberOfTables(type, number) first.
    return false;
  }
  if (index < 0 || index >= static_cast<int>(it->second.size()))
  {
    return false;
  }
  if (it->second[index].GetPointer() == table)
  {
    if (it->second[index] &&
      it->second[index]->GetInformation()->Get(vtkCompositeDataSet::NAME()) != tableName)
    {
      if (tableName.empty())
      {
        it->second[index]->GetInformation()->Remove(vtkCompositeDataSet::NAME());
      }
      else
      {
        it->second[index]->GetInformation()->Set(vtkCompositeDataSet::NAME(), tableName.c_str());
      }
      this->Modified();
      return true;
    }
    // Table is already set to the same value.
    return false;
  }
  it->second[index] = table;
  if (!tableName.empty())
  {
    it->second[index]->GetInformation()->Set(vtkCompositeDataSet::NAME(), tableName.c_str());
  }
  this->Modified();
  return true;
}

bool vtkStatisticalModel::SetTableName(int type, int index, const std::string& name)
{
  if (auto* table = this->GetTable(type, index))
  {
    if (auto* info = table->GetInformation())
    {
      std::string existingName = info->Get(vtkCompositeDataSet::NAME());
      if (existingName != name)
      {
        info->Set(vtkCompositeDataSet::NAME(), name.c_str());
        this->Modified();
        return true;
      }
    }
  }
  return false;
}

VTK_ABI_NAMESPACE_END
