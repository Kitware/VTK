//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/CellGridModel.h>

#include <fides/DataSource.h>
#include <fides/internal/CellGridAttribute.h>
#include <fides/internal/OutputBuilder.h>
#include <fides/internal/ReadPlan.h>

#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fides
{
namespace datamodel
{

namespace
{

/// Splits a data-source variable name like "coordinates/dg points" into
/// the vtkCellGrid attribute group ("coordinates") and the array name
/// within that group ("dg points"). The split is on the first '/'; group
/// names don't contain slashes per the schema.
void SplitVariableName(const std::string& varName, std::string& group, std::string& arrayName)
{
  auto pos = varName.find('/');
  if (pos == std::string::npos)
  {
    group.clear();
    arrayName = varName;
    return;
  }
  group = varName.substr(0, pos);
  arrayName = varName.substr(pos + 1);
}

template <typename T>
T ReadScalarAttribute(fides::io::DataSource& source, const std::string& name, T fallback)
{
  auto vec = source.ReadAttribute<T>(name);
  return vec.empty() ? fallback : vec.front();
}

std::string ReadStringAttribute(fides::io::DataSource& source,
                                const std::string& name,
                                const std::string& fallback = std::string())
{
  auto vec = source.ReadAttribute<std::string>(name);
  return vec.empty() ? fallback : vec.front();
}

/// Open a data source for cell-grid reads. Cell-grid metadata reads pull
/// from the data source's attributes, which every backend must make
/// visible once the source is open. Some backends impose extra
/// conditions: ADIOS2, for instance, only surfaces attributes when the
/// engine is in random-access mode (attributes processed at Open time)
/// or has an active step (streaming mode + a prior BeginStep). The
/// reader's streamSteps choice — propagated to the source's
/// StreamingMode in DataSetReaderImpl::SetupReader — selects which mode
/// is used at Open time, and in streaming mode the existing
/// PrepareNextStep flow drives BeginStep before any cell-grid method
/// runs. Either way, by the time we reach this function attributes are
/// (or will be after Open) visible.
void OpenAndPrepareForReads(fides::io::DataSource& source,
                            const std::unordered_map<std::string, std::string>& paths,
                            const std::string& sourceName)
{
  source.OpenSource(paths, sourceName);
}

/// Per-(attribute, cell-type) metadata keys that are scalar structural
/// fields rather than array roles. Everything else discovered under
/// "{attr}/{ct}/" names an array role pointing at a data-source variable.
bool IsReservedPerTypeKey(const std::string& key)
{
  return key == "function_space" || key == "basis" || key == "order" || key == "dof_sharing" ||
    key == "offset" || key == "blanked";
}

/// One array role on a (attribute, cell-type) pair. Variable blocks are
/// stored as the deferred RawArray vectors returned by ReadVariable; the
/// vector index is the block index.
struct RoleInfo
{
  /// Full data-source variable name (group + '/' + array). Recorded even
  /// when the cache supplies the data so the wire phase can reconstruct
  /// the matching ReadRequest key.
  std::string Variable;
  std::string Group;
  std::string ArrayName;
  std::vector<fides::RawArray> Blocks;
};

/// Per-(attribute, cell-type) info collected during the read pass before
/// per-block tokens are emitted to the OutputBuilder.
struct PerCellTypeInfo
{
  std::string FunctionSpace;
  std::string Basis;
  int Order = 1;
  std::string DOFSharing;
  /// Array roles discovered for this pair, keyed by role name
  /// ("values", "connectivity", "ghost-node", ...). The "ghost-node"
  /// role, when present on the shape attribute, drives the vtkDGCell
  /// source's NodalGhostMarks; all other roles become attribute arrays.
  std::map<std::string, RoleInfo> Roles;
  /// vtkDGCell source-spec scalars (read from the shape attribute's
  /// "offset" / "blanked" metadata; default to 0 / false).
  int64_t Offset = 0;
  bool Blanked = false;
  /// Data source the role reads were keyed against. Stored per-cell-type
  /// for symmetry with the request key, even though today every entry
  /// under one attribute uses the same source.
  std::string SourceForReads;
};

struct AttributeInfo
{
  std::string Name;
  std::string Space;
  int Components = 1;
  bool IsShape = false;
  // Only contains cell types where the attribute is defined.
  std::map<std::string, PerCellTypeInfo> PerCellType;
  /// Back-pointer to the schema-level attribute; used in the wire pass
  /// to populate the static cache. Lifetime is bounded by Read().
  CellGridAttribute* OwningAttribute = nullptr;
};

}

CellGridModel::CellGridModel() = default;
CellGridModel::~CellGridModel() = default;

void CellGridModel::ProcessJSON(const rapidjson::Value& root, DataSourcesType& sources)
{
  if (!root.HasMember("cell_attributes") || !root["cell_attributes"].IsArray())
  {
    throw std::runtime_error("cell_grid model is missing required cell_attributes array.");
  }
  const auto& attrs = root["cell_attributes"].GetArray();
  this->Attributes.clear();
  this->Attributes.reserve(attrs.Size());
  for (const auto& entry : attrs)
  {
    auto attr = std::make_unique<CellGridAttribute>();
    attr->ProcessJSON(entry, sources);
    this->Attributes.push_back(std::move(attr));
  }
}

size_t CellGridModel::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                        DataSourcesType& sources,
                                        const std::string& /*groupName*/)
{
  if (this->Attributes.empty())
  {
    return 0;
  }
  const std::string& sourceName = this->Attributes.front()->DataSourceName;
  auto it = sources.find(sourceName);
  if (it == sources.end() || !it->second)
  {
    return 0;
  }
  auto& source = *it->second;
  OpenAndPrepareForReads(source, paths, sourceName);

  // Find the first attribute / cell-type / values variable defined in the
  // file. Its block count is the cellgrid partition count: every
  // per-(attr,ct) variable in a well-formed cellgrid file is written in
  // lockstep, so any defined values variable answers the question.
  auto cellTypes = source.ReadAttribute<std::string>("cell_types");
  for (const auto& attr : this->Attributes)
  {
    for (const auto& cellTypeName : cellTypes)
    {
      auto valuesVar =
        ReadStringAttribute(source, attr->ObjectName + "/" + cellTypeName + "/values");
      if (!valuesVar.empty())
      {
        return source.GetNumberOfBlocks(valuesVar);
      }
    }
  }
  return 0;
}

std::set<std::string> CellGridModel::GetGroupNames(
  const std::unordered_map<std::string, std::string>&,
  DataSourcesType&)
{
  return {};
}

std::vector<fides::metadata::FieldInformation> CellGridModel::CollectFieldInformation(
  std::shared_ptr<fides::predefined::InternalMetadataSource>&,
  DataSourcesType&)
{
  std::vector<fides::metadata::FieldInformation> result;
  result.reserve(this->Attributes.size());
  for (const auto& attr : this->Attributes)
  {
    // Cell-grid attributes don't map cleanly onto the traditional
    // points/cells distinction; tag them so consumers can present a
    // dedicated selection UI separate from regular field data.
    result.emplace_back(attr->ObjectName, fides::FieldAssociation::CellGrid);
  }
  return result;
}

namespace
{

/// Build a \c ReadRequest matching what the legacy inline path issued
/// for a cell-grid variable: \c source.ReadVariable(varName, selections).
/// The shared SelectionKey-from-MetaData logic captures BLOCK / STEP /
/// GROUP selections so dedup is safe against block subsetting etc.
ReadRequest MakeCellGridRequest(const std::string& dataSourceName,
                                const std::string& variableName,
                                const fides::metadata::MetaData& selections)
{
  ReadRequest req;
  req.DataSourceName = dataSourceName;
  req.VariableName = variableName;
  req.IsVector = fides::io::IsVector::Auto;
  req.MultiBlock = false;
  req.Selection = SelectionKey::From(selections);
  return req;
}

}

void CellGridModel::Read(const std::unordered_map<std::string, std::string>& paths,
                         DataSourcesType& sources,
                         const fides::metadata::MetaData& selections,
                         fides::OutputBuilder& builder)
{
  if (this->Attributes.empty())
  {
    throw std::runtime_error("cell_grid model has no attributes to read.");
  }

  const std::string& primarySourceName = this->Attributes.front()->DataSourceName;
  auto primaryIt = sources.find(primarySourceName);
  if (primaryIt == sources.end() || !primaryIt->second)
  {
    throw std::runtime_error("cell_grid attribute references unknown data source '" +
                             primarySourceName + "'.");
  }
  auto& primarySource = *primaryIt->second;
  OpenAndPrepareForReads(primarySource, paths, primarySourceName);

  auto cellTypes = primarySource.ReadAttribute<std::string>("cell_types");
  if (cellTypes.empty())
  {
    throw std::runtime_error("cell_grid file is missing required 'cell_types' metadata.");
  }

  // Read the global (cell-type) shape names once.
  std::map<std::string, std::string> shapeByCellType;
  for (const auto& cellTypeName : cellTypes)
  {
    shapeByCellType[cellTypeName] = ReadStringAttribute(primarySource, cellTypeName + "/shape");
  }

  // Honor a per-attribute selection passed in via the FIELDS metadata key.
  // Filtering is active whenever FIELDS is present at all — that's how a
  // consumer signals "I am being explicit about which fields to read."
  // Only entries whose Association is FieldAssociation::CellGrid count
  // toward cellgrid attribute selection; entries with other associations
  // belong to other models (Points/Cells/WholeDataSet field data) and
  // are ignored here.
  //
  // An empty CellGrid set under an active FIELDS therefore means "read
  // no cellgrid attributes" (the user disabled all of them in the UI),
  // not "read everything by default." Consumers that don't set FIELDS at
  // all (e.g. Fides' own tests, which pass an empty MetaData) still get
  // the read-everything fallback.
  const bool filterByCellGridSelection = selections.Has(fides::keys::FIELDS());
  std::set<std::string> selectedCellGridAttrs;
  if (filterByCellGridSelection)
  {
    const auto& fieldsSel =
      selections.Get<fides::metadata::Vector<fides::metadata::FieldInformation>>(
        fides::keys::FIELDS());
    for (const auto& f : fieldsSel.Data)
    {
      if (f.Association == fides::FieldAssociation::CellGrid)
      {
        selectedCellGridAttrs.insert(f.Name);
      }
    }
  }

  // Phase 1 — Plan: walk attributes, read structural attributes
  // (cheap), and emit a ReadRequest for each variable (values /
  // connectivity) that isn't satisfied by the static cache. The
  // PerCellTypeInfo records its variable name strings even when cached
  // so the emit pass can still split them into (group, arrayName) pairs
  // for the OutputBuilder.
  //
  // Attributes marked "static": true in the schema cache their values
  // and connectivity RawArray vectors on first call and reuse them on
  // subsequent calls — typical for geometry (the shape attribute) and
  // any topology that is constant across steps.
  //
  // Multiple attributes may reference the same connectivity variable
  // (e.g. shape, scalar0, scalar1, ... all reference vtkDGHex/conn);
  // ReadPlan::Execute coalesces those into a single ReadVariable call.
  std::vector<AttributeInfo> attrData;
  attrData.reserve(this->Attributes.size());
  std::vector<ReadRequest> requests;

  for (const auto& attribute : this->Attributes)
  {
    if (filterByCellGridSelection &&
        selectedCellGridAttrs.find(attribute->ObjectName) == selectedCellGridAttrs.end())
    {
      // User deselected this attribute; skip planning + reading it.
      continue;
    }
    auto attrSourceIt = sources.find(attribute->DataSourceName);
    if (attrSourceIt == sources.end() || !attrSourceIt->second)
    {
      throw std::runtime_error("cell_grid attribute '" + attribute->ObjectName +
                               "' references unknown data source '" + attribute->DataSourceName +
                               "'.");
    }
    auto& source = *attrSourceIt->second;
    OpenAndPrepareForReads(source, paths, attribute->DataSourceName);

    AttributeInfo info;
    info.Name = attribute->ObjectName;
    info.Space = ReadStringAttribute(source, info.Name + "/space");
    if (info.Space.empty())
    {
      throw std::runtime_error("cell_grid attribute '" + info.Name +
                               "' is missing in the file (no '/space' metadata).");
    }
    info.Components = ReadScalarAttribute<std::int32_t>(source, info.Name + "/components", 1);
    info.IsShape = ReadScalarAttribute<std::int32_t>(source, info.Name + "/is_shape", 0) != 0;

    for (const auto& cellTypeName : cellTypes)
    {
      const std::string prefix = info.Name + "/" + cellTypeName;
      auto fnSpace = ReadStringAttribute(source, prefix + "/function_space");
      if (fnSpace.empty())
      {
        // Attribute is not defined for this cell type — skip.
        continue;
      }
      PerCellTypeInfo perType;
      perType.FunctionSpace = fnSpace;
      perType.Basis = ReadStringAttribute(source, prefix + "/basis");
      perType.Order = ReadScalarAttribute<std::int32_t>(source, prefix + "/order", 1);
      perType.DOFSharing = ReadStringAttribute(source, prefix + "/dof_sharing");
      // vtkDGCell source-spec scalars (only meaningful on the shape
      // attribute). offset is read as int32 today — wide enough for the
      // picking start-id of grids up to ~2B cells.
      perType.Offset = ReadScalarAttribute<std::int32_t>(source, prefix + "/offset", 0);
      perType.Blanked = ReadScalarAttribute<std::int32_t>(source, prefix + "/blanked", 0) != 0;

      // Discover the array roles this (attribute, cell-type) carries from
      // the data source rather than assuming a fixed values/connectivity
      // pair. Every child key under the prefix that isn't a reserved
      // structural scalar names a role whose value is the variable name.
      for (const auto& role : source.GetAttributeNames(prefix))
      {
        if (IsReservedPerTypeKey(role))
        {
          continue;
        }
        auto variable = ReadStringAttribute(source, prefix + "/" + role);
        if (variable.empty())
        {
          continue;
        }
        RoleInfo roleInfo;
        roleInfo.Variable = variable;
        SplitVariableName(variable, roleInfo.Group, roleInfo.ArrayName);

        const bool useCached =
          attribute->IsRoleStatic(role) && attribute->ValidCachedRoles.count(role) > 0;
        if (useCached)
        {
          auto& byCellType = attribute->CachedArraysByRole[role];
          auto it = byCellType.find(cellTypeName);
          if (it != byCellType.end())
          {
            roleInfo.Blocks = it->second;
          }
        }
        else
        {
          requests.push_back(MakeCellGridRequest(attribute->DataSourceName, variable, selections));
        }
        perType.Roles.emplace(role, std::move(roleInfo));
      }

      perType.SourceForReads = attribute->DataSourceName;
      info.PerCellType.emplace(cellTypeName, std::move(perType));
    }

    info.OwningAttribute = attribute.get();
    attrData.push_back(std::move(info));
  }

  // The user may have disabled every cellgrid attribute via the FIELDS
  // selection. There's nothing to read, define, or emit; return without
  // adding any cellgrid to the builder. Treating this as an error would
  // break the perfectly reasonable workflow of unchecking all attributes
  // in the GUI to inspect just the geometry of a non-cellgrid partition
  // in the same dataset.
  if (attrData.empty())
  {
    return;
  }

  // Phase 2 — Execute: one DataSource read per unique request. Two
  // attributes naming the same variable share one read.
  ReadResultMap results = ReadPlan::Execute(requests, paths, sources, selections);

  // Phase 3 — Wire results back into per-(attr, cell-type, role) RawArray
  // vectors. Cached entries are already populated; new reads come from
  // the result map. Each role's cache is tracked independently so a
  // schema can freeze just one of them (e.g. connectivity) while letting
  // others change every step.
  for (auto& info : attrData)
  {
    auto* attribute = info.OwningAttribute;
    std::set<std::string> rolesPopulatedThisCall;
    for (auto& [cellTypeName, perType] : info.PerCellType)
    {
      for (auto& [role, roleInfo] : perType.Roles)
      {
        const bool useCached =
          attribute->IsRoleStatic(role) && attribute->ValidCachedRoles.count(role) > 0;
        if (roleInfo.Variable.empty() || useCached)
        {
          continue;
        }
        auto req = MakeCellGridRequest(perType.SourceForReads, roleInfo.Variable, selections);
        auto it = results.find(req);
        if (it != results.end())
        {
          roleInfo.Blocks = it->second;
        }
        if (attribute->IsRoleStatic(role))
        {
          attribute->CachedArraysByRole[role][cellTypeName] = roleInfo.Blocks;
          rolesPopulatedThisCall.insert(role);
        }
      }
    }
    // Mark each static role valid only after all of its cell types have
    // been cached, so the next call's useCached check is consistent.
    for (const auto& role : rolesPopulatedThisCall)
    {
      attribute->ValidCachedRoles.insert(role);
    }
  }

  // Determine the partition (block) count. Variables are written in
  // lockstep, so the first non-empty block vector answers it. When a block
  // selection limits reads, the vector size is already filtered.
  size_t nBlocks = 0;
  for (const auto& info : attrData)
  {
    for (const auto& [_, perType] : info.PerCellType)
    {
      for (const auto& [role, roleInfo] : perType.Roles)
      {
        if (!roleInfo.Blocks.empty())
        {
          nBlocks = roleInfo.Blocks.size();
          break;
        }
      }
      if (nBlocks > 0)
      {
        break;
      }
    }
    if (nBlocks > 0)
    {
      break;
    }
  }
  if (nBlocks == 0)
  {
    throw std::runtime_error(
      "cell_grid file has no readable variable blocks for the schema-listed attributes.");
  }

  // Pass 2: emit one cellgrid token per block.
  for (size_t blockIndex = 0; blockIndex < nBlocks; ++blockIndex)
  {
    size_t cgToken = builder.CreateCellGrid();
    for (const auto& cellTypeName : cellTypes)
    {
      builder.AddCellTypeToCellGrid(cgToken, cellTypeName, shapeByCellType[cellTypeName]);
    }

    for (const auto& info : attrData)
    {
      size_t attrToken =
        builder.CreateCellAttribute(info.Name, info.Space, info.Components, info.IsShape);

      for (const auto& [cellTypeName, perType] : info.PerCellType)
      {
        builder.SetCellAttributeForType(attrToken,
                                        cellTypeName,
                                        perType.FunctionSpace,
                                        perType.Basis,
                                        perType.Order,
                                        perType.DOFSharing,
                                        perType.Offset,
                                        perType.Blanked);

        for (const auto& [role, roleInfo] : perType.Roles)
        {
          size_t arrayToken = OutputBuilder::InvalidToken;
          if (blockIndex < roleInfo.Blocks.size())
          {
            arrayToken = builder.CreateArray(roleInfo.Blocks[blockIndex]);
          }
          builder.AddCellAttributeArrayForType(
            attrToken, cellTypeName, role, roleInfo.Group, roleInfo.ArrayName, arrayToken);
        }
      }

      builder.AddCellAttributeToCellGrid(cgToken, attrToken);
    }

    builder.AddPartition(cgToken);
  }
}

void CellGridModel::PostRead(fides::DataContainer&, const fides::metadata::MetaData&) {}

}
}
