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

bool PutArray(vtkDataArray* array, const std::string& arrayRole, nlohmann::json& jSpec,
  const std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations)
{
  std::string arrayGroup;
  std::string arrayName;
  auto it = arrayLocations.find(array);
  if (it != arrayLocations.end())
  {
    if (!it->second.Data().empty())
    {
      arrayGroup = it->second.Data();
    }
    else
    {
      std::ostringstream groupId;
      groupId << it->second.Data();
      arrayGroup = groupId.str();
    }
  }
  arrayName = array ? array->GetName() : "";
  if (!arrayGroup.empty() && !arrayName.empty())
  {
    jSpec[arrayRole] = nlohmann::json::array({ arrayGroup, arrayName });
  }
  else
  {
    vtkGenericWarningMacro("No array group contains array " << array << " or array is null.");
    return false;
  }
  return true;
}

bool FetchArray(vtkDataArray*& array, const std::string& arrayName, const nlohmann::json& jSpec,
  vtkCellGrid* grid, bool mandatory = true)
{
  auto jArray = jSpec.find(arrayName);
  if (jArray == jSpec.end() || !jArray->is_array() || jArray->size() != 2)
  {
    if (mandatory)
    {
      vtkGenericWarningMacro("Specification has missing or malformed connectivity.");
    }
    return false;
  }
  else
  {
    auto arrayGroupName = (*jArray)[0].get<std::string>();
    auto jArrayName = (*jArray)[1].get<std::string>();
    auto* arrayGroup = grid->FindAttributes(vtkStringToken(arrayGroupName));
    if (arrayGroup)
    {
      array = arrayGroup->GetArray(jArrayName.c_str());
    }
    else
    {
      vtkGenericWarningMacro(
        "Connectivity \"" << arrayGroupName << "\", \"" << jArrayName << "\" not found.");
      return false;
    }
  }
  return true;
}

nlohmann::json CellSourceToJSON(const vtkDGCell::Source& spec,
  const std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations)
{
  nlohmann::json result;
  PutArray(spec.Connectivity, "connectivity", result, arrayLocations);
  if (spec.NodalGhostMarks)
  {
    PutArray(spec.NodalGhostMarks, "ghost-node", result, arrayLocations);
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
  if (spec.SideType >= 0)
  {
    result["side-type"] = spec.SideType;
  }
  if (spec.SelectionType != -1)
  {
    result["selection-type"] = spec.SelectionType;
  }
  return result;
}

vtkDGCell::Source JSONToCellSource(const nlohmann::json& jSpec, vtkCellGrid* grid)
{
  vtkDGCell::Source result;
  FetchArray(result.Connectivity, "connectivity", jSpec, grid);
  FetchArray(result.NodalGhostMarks, "ghost-node", jSpec, grid, /*mandatory*/ false);

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

  auto jSideType = jSpec.find("side-type");
  if (jSideType != jSpec.end())
  {
    result.SideType = jSideType->get<int>();
  }

  auto jSelnType = jSpec.find("selection-type");
  if (jSelnType != jSpec.end())
  {
    result.SelectionType = jSelnType->get<int>();
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

  vtkStringToken cellTypeName = dgCell->GetClassName();
  auto* grid = dgCell->GetCellGrid();
  nlohmann::json& jj(*query->GetData());
  auto& cellSpec = dgCell->GetCellSpec();
  auto& sideSpecs = dgCell->GetSideSpecs();

  bool ok = true;
  // Serialize or deserialize as requested:
  if (query->IsSerializing())
  {
    // Build a lookup table of arrays so we can find the group they belong to.
    std::unordered_map<vtkAbstractArray*, vtkStringToken> arrayLocations;
    grid->MapArrayLocations(arrayLocations);
    auto& jCellType = query->AddMetadataEntry(cellTypeName);
    // Convert CellSpec and SideSpecs to JSON
    jCellType["cell-spec"] = CellSourceToJSON(cellSpec, arrayLocations);
    if (!sideSpecs.empty())
    {
      auto& jSideSpecs = jCellType["side-specs"];
      for (const auto& sideSpec : sideSpecs)
      {
        jSideSpecs.push_back(CellSourceToJSON(sideSpec, arrayLocations));
      }
    }
    // Add CellTypeInfo for each vtkCellAttribute.
    // We assume that the order of attributes in GetCellAttributeList()
    // is identical to the order of attributes in the JSON array.
    auto& jAttributeList = *query->GetAttributeData();
    auto ait = jAttributeList.begin();
    for (const auto& cellAttId : grid->GetCellAttributeIds())
    {
      auto attribute = grid->GetCellAttributeById(cellAttId);
      if (!attribute)
      {
        continue;
      }
      auto cellTypeInfo = attribute->GetCellTypeInfo(cellTypeName);
      query->InsertCellTypeAttributeInfo(grid, cellTypeInfo, *ait, cellTypeName);
      ++ait;

      // ait and attribute must match since jAttributeList was created by
      // iterating over grid->GetCellAttributeIds().
    }
    ok = true;
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
      // Force the class and cell shape to correspond (only for cellSpec,
      // not for sideSpecs).
      cellSpec.SourceShape = dgCell->GetShape();
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
    // Add CellTypeInfo to vtkCellAttribute instances.
    auto* jAttributes = query->GetAttributeData();
    if (!jAttributes)
    {
      ok = false;
    }
    else
    {
      auto jit = jAttributes->begin();
      for (auto* attribute : query->GetAttributeList())
      {
#if 0
        auto cit = jit->find(dgCell->GetClassName());
        if (cit == jit->end())
        {
          continue;
        }
        auto iit = cit->find("cell-info");
        if (iit == cit->end())
        {
          vtkWarningMacro(
            "No cell-info for \"" << cellTypeName.Data() << "\" cells "
            "of \"" << attribute->GetName().Data() << "\" attribute.");
          ok = false;
          continue;
        }
#else
        auto iit = jit->find("cell-info");
        if (iit == jit->end())
        {
          vtkWarningMacro("No cell-info for \"" << cellTypeName.Data()
                                                << "\" cells "
                                                   "of \""
                                                << attribute->GetName().Data() << "\" attribute.");
          ok = false;
          continue;
        }
#endif
        vtkCellAttribute::CellTypeInfo cellTypeInfo;
        if (query->ExtractCellTypeAttributeInfo(grid, cellTypeInfo, *iit, cellTypeName))
        {
          attribute->SetCellTypeInfo(cellTypeName, cellTypeInfo);
        }
        else
        {
          vtkWarningMacro("Could not fetch cell-info for \"" << cellTypeName.Data()
                                                             << "\" cells "
                                                                "of \""
                                                             << attribute->GetName().Data()
                                                             << "\" attribute.");
          ok = false;
        }
        ++jit;
      }
    }
    ok &= didAdd;
  }

  return ok;
}

VTK_ABI_NAMESPACE_END
