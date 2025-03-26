// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridIOQuery.h"

#include "vtkCellGrid.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"

#include <vtk_nlohmannjson.h>        // For API.
#include VTK_NLOHMANN_JSON(json.hpp) // For API.

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridIOQuery);

void vtkCellGridIOQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Data: ";
  if (this->Data)
  {
    os << this->Data->dump(2) << "\n";
  }
  else
  {
    os << "null\n";
  }
}

void vtkCellGridIOQuery::PrepareToDeserialize(const nlohmann::json& sourceData,
  const nlohmann::json& attributeData, std::vector<vtkCellAttribute*>& attributeList)
{
  this->Data = const_cast<nlohmann::json*>(&sourceData);
  this->AttributeData = const_cast<nlohmann::json*>(&attributeData);
  this->AttributeList = &attributeList;
  this->ArrayLocations.clear();
  this->Serializing = false;
}

void vtkCellGridIOQuery::PrepareToSerialize(nlohmann::json& destination,
  nlohmann::json& attributeData,
  const std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations)
{
  this->Data = &destination;
  this->AttributeData = &attributeData;
  this->AttributeList = nullptr;
  this->ArrayLocations = arrayLocations;
  this->Serializing = true;
}

bool vtkCellGridIOQuery::ExtractCellTypeAttributeInfo(vtkCellGrid* grid,
  vtkCellAttribute::CellTypeInfo& cellTypeInfo, const nlohmann::json& jsonInfo,
  vtkStringToken cellTypeName)
{
  if (!cellTypeName.IsValid())
  {
    return false;
  }

  auto it = jsonInfo.find(cellTypeName.Data());
  if (it == jsonInfo.end())
  {
    return false;
  }

  vtkStringToken invalid;
  // dof-sharing is not mandatory (and absent for discontinuous attributes):
  cellTypeInfo.DOFSharing =
    it->contains("dof-sharing") ? (*it)["dof-sharing"].get<std::string>() : invalid;
  // function-space, basis, and order are mandatory
  cellTypeInfo.FunctionSpace = (*it)["function-space"].get<vtkStringToken>();
  cellTypeInfo.Basis = (*it)["basis"].get<vtkStringToken>();
  cellTypeInfo.Order = (*it)["order"].get<int>();

  if (it->contains("arrays"))
  {
    nlohmann::json jArrays = it->at("arrays");
    for (const auto& arraySpec : jArrays.items())
    {
      vtkStringToken group(arraySpec.value()[0].get<std::string>());
      vtkStringToken arrayName(arraySpec.value()[1].get<std::string>());
      // std::cout << cellTypeName.Data() << " " << arraySpec.key() << " has " << group.Data() <<
      // ", " << arrayName.Data() << "\n";
      auto* arrayGroup = grid->GetAttributes(group.GetId());
      if (arrayGroup)
      {
        auto* array = arrayGroup->GetArray(arrayName.Data().c_str());
        if (array)
        {
          cellTypeInfo.ArraysByRole[arraySpec.key()] = array;
        }
        else
        {
          vtkWarningMacro(
            "Array \"" << arrayName.Data() << "\" not present in \"" << group.Data() << "\".");
        }
      }
    }
  }
  return true;
}

bool vtkCellGridIOQuery::InsertCellTypeAttributeInfo(vtkCellGrid* grid,
  const vtkCellAttribute::CellTypeInfo& cellTypeInfo, nlohmann::json& jsonInfo,
  vtkStringToken cellTypeName)
{
  (void)grid;
  bool ok = true;
  nlohmann::json jCellTypeInfo;
  nlohmann::json jCellBlock;
  nlohmann::json arraysByRole;
  for (const auto& entry : cellTypeInfo.ArraysByRole)
  {
    auto groupIt = this->ArrayLocations.find(entry.second);
    if (groupIt == this->ArrayLocations.end())
    {
      ok = false;
      vtkWarningMacro(
        "Unmanaged array " << entry.second << " in role " << entry.first.Data() << " skipped.");
      continue;
    }
    arraysByRole[entry.first.Data()] =
      nlohmann::json::array({ groupIt->second.Data(), entry.second->GetName() });
  }
  jCellBlock = { { "function-space", cellTypeInfo.FunctionSpace.Data() },
    { "basis", cellTypeInfo.Basis.Data() }, { "order", cellTypeInfo.Order } };
  if (!arraysByRole.empty())
  {
    jCellBlock["arrays"] = arraysByRole;
  }
  if (cellTypeInfo.DOFSharing.IsValid())
  {
    jCellBlock["dof-sharing"] = cellTypeInfo.DOFSharing.Data();
  }
  jCellTypeInfo[cellTypeName.Data()] = jCellBlock;
  jsonInfo["cell-info"] = jCellTypeInfo;
  return ok;
}

nlohmann::json& vtkCellGridIOQuery::AddMetadataEntry(vtkStringToken cellTypeName)
{
  nlohmann::json entry;
  entry["type"] = cellTypeName.Data();
  this->Data->push_back(entry);
  return this->Data->back();
}

VTK_ABI_NAMESPACE_END
