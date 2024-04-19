// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGIOResponder.h"

#include "vtkCellGrid.h"
#include "vtkDGCell.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

nlohmann::json CellSourceToJSON(
  const vtkDGCell::Source& spec, const std::map<vtkAbstractArray*, vtkStringToken>& arrayLocations)
{
  nlohmann::json result;
  std::string connGroup;
  std::string connName;
  auto it = arrayLocations.find(spec.Connectivity);
  if (it != arrayLocations.end())
  {
    if (!it->second.Data().empty())
    {
      connGroup = it->second.Data();
    }
    else
    {
      std::ostringstream groupId;
      groupId << it->second.Data();
      connGroup = groupId.str();
    }
  }
  connName = spec.Connectivity ? spec.Connectivity->GetName() : "";
  if (!connGroup.empty() && !connName.empty())
  {
    result["connectivity"] = nlohmann::json::array({ connGroup, connName });
  }
  else
  {
    vtkGenericWarningMacro(
      "No array group contains array " << spec.Connectivity << " or array is null.");
  }
  result["shape"] = vtkDGCell::GetShapeName(spec.SourceShape).Data();
  if (spec.Offset != 0)
  {
    result["offset"] = spec.Offset;
  }
  if (spec.Blanked)
  {
    result["blanked"] = true;
  }
  return result;
}

vtkDGCell::Source JSONToCellSource(const nlohmann::json& jSpec, vtkCellGrid* grid)
{
  vtkDGCell::Source result;
  auto jConn = jSpec.find("connectivity");
  if (jConn == jSpec.end() || !jConn->is_array() || jConn->size() != 2)
  {
    vtkGenericWarningMacro("Specification has missing or malformed connectivity.");
  }
  else
  {
    auto arrayGroupName = (*jConn)[0].get<std::string>();
    auto arrayName = (*jConn)[1].get<std::string>();
    auto* arrayGroup = grid->FindAttributes(vtkStringToken(arrayGroupName));
    if (arrayGroup)
    {
      result.Connectivity = arrayGroup->GetArray(arrayName.c_str());
    }
    else
    {
      vtkGenericWarningMacro(
        "Connectivity \"" << arrayGroupName << "\", \"" << arrayName << "\" not found.");
    }
  }

  auto jShape = jSpec.find("shape");
  if (jShape != jSpec.end())
  {
    auto shapeName = jShape->get<std::string>();
    result.SourceShape = vtkDGCell::GetShapeEnum(shapeName);
  }

  auto jOffset = jSpec.find("offset");
  if (jOffset != jSpec.end())
  {
    result.Offset = jOffset->get<vtkIdType>();
  }

  auto jBlank = jSpec.find("blanked");
  if (jBlank != jSpec.end())
  {
    result.Blanked = jBlank->get<bool>();
  }

  return result;
}

} // anonymous namespace

vtkStandardNewMacro(vtkDGIOResponder);

void vtkDGIOResponder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGIOResponder::Query(
  vtkCellGridIOQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;
  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!query || !query->GetData())
  {
    return false;
  }
  if (!dgCell)
  {
    // Allow this responder to "work" as a default for any cell type
    // by doing nothing for non-DG cells.
    // Do not register this responder with non-DG cell types if you
    // have implemented a different, non-trivial responder.
    return true;
  }

  auto* grid = dgCell->GetCellGrid();
  nlohmann::json& jj(*query->GetData());
  auto& cellSpec = dgCell->GetCellSpec();
  auto& sideSpecs = dgCell->GetSideSpecs();

  bool ok = true;
  // Serialize or deserialize as requested:
  if (query->IsSerializing())
  {
    // Build a lookup table of arrays so we can find the group they belong to.
    std::map<vtkAbstractArray*, vtkStringToken> arrayLocations;
    for (const auto& entry : grid->GetArrayGroups())
    {
      auto groupToken = entry.first;
      for (vtkIdType ii = 0; ii < entry.second->GetNumberOfArrays(); ++ii)
      {
        auto* arr = entry.second->GetAbstractArray(ii);
        if (arr)
        {
          arrayLocations[arr] = groupToken;
        }
      }
    }
    // Convert CellSpec and SideSpecs to JSON
    bool didAdd = false;
    for (auto& jCellType : jj)
    {
      if (!jCellType.contains("type") || jCellType["type"] != dgCell->GetClassName())
      {
        continue;
      }
      jCellType["cell-spec"] = CellSourceToJSON(cellSpec, arrayLocations);
      if (!sideSpecs.empty())
      {
        auto& jSideSpecs = jCellType["side-specs"];
        for (const auto& sideSpec : sideSpecs)
        {
          jSideSpecs.push_back(CellSourceToJSON(sideSpec, arrayLocations));
        }
      }
      didAdd = true;
      break; // At most one entry can match.
    }
    ok &= didAdd;
  }
  else
  {
    bool didAdd = false;
    // Read CellSpec and SideSpecs from JSON
    for (const auto& jCellType : jj)
    {
      if (!jCellType.contains("type") || jCellType["type"] != dgCell->GetClassName())
      {
        continue;
      }
      cellSpec = JSONToCellSource(jCellType["cell-spec"], grid);
      auto jit = jCellType.find("side-specs");
      if (jit != jCellType.end())
      {
        for (const auto& spec : *jit)
        {
          sideSpecs.push_back(JSONToCellSource(spec, grid));
        }
      }
      didAdd = true;
      break; // Only one entry per cell type should exist.
    }
    ok &= didAdd;
  }

  return ok;
}

VTK_ABI_NAMESPACE_END
