//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/DataSetModel.h>

#include <fides/Keys.h>
#include <fides/internal/CellSet.h>
#include <fides/internal/CoordinateSystem.h>
#include <fides/internal/Field.h>
#include <fides/internal/OutputBuilder.h>
#include <fides/internal/ReadPlan.h>
#include <fides/internal/predefined/DataModelHelperFunctions.h>
#include <fides/internal/predefined/InternalMetadataSource.h>

#if FIDES_USE_VISKORES
#include <fides/xgc/XGCCommon.h>
#endif

#include <algorithm>
#include <stdexcept>

namespace fides
{
namespace datamodel
{

namespace
{

const rapidjson::Value& FindAndReturnObject(const rapidjson::Value& root, const std::string& name)
{
  if (!root.HasMember(name.c_str()))
  {
    throw std::runtime_error("Missing " + name + " member.");
  }
  const auto& val = root[name.c_str()];
  if (!val.IsObject())
  {
    throw std::runtime_error(name + " is expected to be an object.");
  }
  return val;
}

}

void DataSetModel::ProcessJSON(const rapidjson::Value& root, DataSourcesType& sources)
{
#if FIDES_USE_VISKORES
  if (root.HasMember("number_of_planes"))
  {
    const auto& nPlanes = root["number_of_planes"];
    fides::datamodel::XGCCommon::ProcessNumberOfPlanes(nPlanes, sources);
  }
#endif

  if (!root.HasMember("coordinate_system"))
  {
    throw std::runtime_error("Missing coordinate_system member.");
  }
  this->ProcessCoordinateSystem(FindAndReturnObject(root, "coordinate_system"), sources);

  if (!root.HasMember("cell_set"))
  {
    throw std::runtime_error("Missing cell_set member.");
  }
  this->ProcessCellSet(FindAndReturnObject(root, "cell_set"), sources);

  if (root.HasMember("fields"))
  {
    this->ProcessFields(root["fields"], sources);
  }
}

void DataSetModel::ProcessCoordinateSystem(const rapidjson::Value& coordSys,
                                           DataSourcesType& sources)
{
  this->Coordinates = std::make_shared<fides::datamodel::CoordinateSystem>();
  this->Coordinates->ObjectName = "coordinate_system";

  this->Coordinates->ProcessJSON(coordSys, sources);
}

void DataSetModel::ProcessCellSet(const rapidjson::Value& cellSet, DataSourcesType& sources)
{
  this->Cells = std::make_shared<fides::datamodel::CellSet>();
  this->Cells->ObjectName = "cell_set";

  this->Cells->ProcessJSON(cellSet, sources);
}

std::shared_ptr<Field> DataSetModel::ProcessField(const rapidjson::Value& fieldJson,
                                                  DataSourcesType& sources)
{
  if (!fieldJson.IsObject())
  {
    throw std::runtime_error("field needs to be an object.");
  }
  auto field = std::make_shared<fides::datamodel::Field>();
  field->ProcessJSON(fieldJson, sources);
  field->ObjectName = "field";
  return field;
}

void DataSetModel::ProcessFields(const rapidjson::Value& fields, DataSourcesType& sources)
{
  this->Fields.clear();
  if (!fields.IsArray())
  {
    throw std::runtime_error("fields is not an array.");
  }
  auto fieldsArray = fields.GetArray();
  for (const auto& field : fieldsArray)
  {
    auto fieldPtr = this->ProcessField(field, sources);
    this->Fields[std::make_pair(fieldPtr->Name, fieldPtr->Association)] = fieldPtr;
  }
}

size_t DataSetModel::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                       DataSourcesType& sources,
                                       const std::string& groupName)
{
  if (!this->Coordinates)
  {
    throw std::runtime_error("Cannot read missing coordinate system.");
  }
  return this->Coordinates->GetNumberOfBlocks(paths, sources, groupName);
}

std::set<std::string> DataSetModel::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  if (!this->Coordinates)
  {
    throw std::runtime_error("Cannot read missing coordinate system.");
  }
  return this->Coordinates->GetGroupNames(paths, sources);
}

std::vector<fides::metadata::FieldInformation> DataSetModel::CollectFieldInformation(
  std::shared_ptr<fides::predefined::InternalMetadataSource>& metadataSource,
  DataSourcesType& sources)
{
  std::vector<fides::metadata::FieldInformation> result;
  if (this->Fields.empty())
  {
    return result;
  }
  this->ExpandWildcardFields(metadataSource, sources);
  for (auto& item : this->Fields)
  {
    auto& field = item.second;
    result.emplace_back(field->Name, field->Association);
  }
  return result;
}

void DataSetModel::Read(const std::unordered_map<std::string, std::string>& paths,
                        DataSourcesType& sources,
                        const fides::metadata::MetaData& selections,
                        fides::OutputBuilder& builder)
{
  if (!this->Coordinates)
  {
    throw std::runtime_error("Cannot read missing coordinate system.");
  }
  if (!this->Cells)
  {
    throw std::runtime_error("Cannot read missing cell set.");
  }

  // Resolve which fields participate in this Read up front; same set is
  // walked in plan and emit phases.
  std::vector<std::shared_ptr<fides::datamodel::Field>> activeFields;
  if (selections.Has(fides::keys::FIELDS()))
  {
    using FieldInfoType = fides::metadata::Vector<fides::metadata::FieldInformation>;
    const auto& fields = selections.Get<FieldInfoType>(fides::keys::FIELDS());
    activeFields.reserve(fields.Data.size());
    for (const auto& field : fields.Data)
    {
      auto itr = this->Fields.find(std::make_pair(field.Name, field.Association));
      if (itr != this->Fields.end())
      {
        activeFields.push_back(itr->second);
      }
    }
  }
  else
  {
    activeFields.reserve(this->Fields.size());
    for (auto& field : this->Fields)
    {
      activeFields.push_back(field.second);
    }
  }

  // Phase 1 — Plan: every component appends the read requests it would
  // have issued inline. Components that haven't been converted to the
  // two-pass flow contribute nothing here; their default EmitTokens will
  // fall back to the legacy single-pass Read, ignoring the result map.
  std::vector<ReadRequest> requests;
  this->Coordinates->CollectReadRequests(paths, sources, selections, requests);
  this->Cells->CollectReadRequests(paths, sources, selections, requests);
  for (auto& field : activeFields)
  {
    field->CollectReadRequests(paths, sources, selections, requests);
  }

  // Phase 2 — Execute: one DataSource read per unique request.
  ReadResultMap results = ReadPlan::Execute(requests, paths, sources, selections);

  // Phase 3 — Emit tokens. Components that converted look themselves up
  // in `results`; legacy components see the default EmitTokens, which
  // re-issues their inline reads.
  std::vector<size_t> coordTokens =
    this->Coordinates->EmitTokens(paths, sources, selections, results, builder);
  std::vector<size_t> cellSetTokens =
    this->Cells->EmitTokens(paths, sources, selections, results, builder);

  size_t nPartitions = std::max(coordTokens.size(), cellSetTokens.size());

  std::vector<size_t> dsTokens(nPartitions);
  for (size_t i = 0; i < nPartitions; i++)
  {
    dsTokens[i] = builder.CreateDataSet();
    if (i < coordTokens.size())
    {
      builder.SetCoordinateSystem(dsTokens[i], coordTokens[i]);
    }
    if (i < cellSetTokens.size())
    {
      builder.SetCellSet(dsTokens[i], cellSetTokens[i]);
    }
  }

  for (auto& field : activeFields)
  {
    auto fieldTokens = field->EmitTokens(paths, sources, selections, results, builder);
    for (size_t i = 0; i < nPartitions; i++)
    {
      if (i < fieldTokens.size())
      {
        builder.AddField(dsTokens[i], field->Name, field->Association, fieldTokens[i]);
      }
    }
  }

  for (size_t i = 0; i < nPartitions; i++)
  {
    builder.AddPartition(dsTokens[i]);
  }
}

void DataSetModel::PostRead(fides::DataContainer& container,
                            const fides::metadata::MetaData& selections)
{
  this->Coordinates->PostRead(container, selections);
  this->Cells->PostRead(container, selections);
  for (auto& f : this->Fields)
  {
    f.second->PostRead(container, selections);
  }
}

void DataSetModel::ExpandWildcardFields(
  std::shared_ptr<fides::predefined::InternalMetadataSource>& metadataSource,
  DataSourcesType& sources)
{
  auto it = this->Fields.begin();
  while (it != this->Fields.end())
  {
    auto& origField = it->second;
    // find fields to expand
    if (origField->IsWildcardField())
    {
      auto lists = origField->GetWildcardFieldLists(metadataSource);
      // need to add each name, association pair to Fields
      // as well as create the associated Field object
      auto& names = lists.Names;
      auto& associations = lists.Associations;

      for (size_t i = 0; i < names.size(); ++i)
      {
        std::string isVector = "auto";
        std::string source = "source";
        std::string arrayType = "basic";
        if (!lists.IsVector.empty() && i < lists.IsVector.size())
        {
          isVector = lists.IsVector[i];
        }
        if (!lists.Sources.empty() && i < lists.Sources.size())
        {
          source = lists.Sources[i];
        }
        if (!lists.ArrayTypes.empty() && i < lists.ArrayTypes.size())
        {
          arrayType = lists.ArrayTypes[i];
        }

        // the wildcard field uses an ArrayPlaceholder. Now we have enough info
        // to create the actual JSON for the Array object for this Field. This can
        // then be passed to Field.ProcessExpandedField which will use it to create
        // the actual array object.
        rapidjson::Document arrayObj;
        arrayObj = predefined::CreateFieldArrayDoc(names[i], source, arrayType, isVector);

        if (!arrayObj.HasMember("array"))
        {
          throw std::runtime_error("Field Array Object was not created correctly");
        }
        auto fieldPtr = std::make_shared<fides::datamodel::Field>();
        fieldPtr->ProcessExpandedField(names[i], associations[i], arrayObj, sources);
        fieldPtr->ObjectName = "field";
        this->Fields[std::make_pair(fieldPtr->Name, fieldPtr->Association)] = fieldPtr;
      }

      // remove the wildcard field now that we're done expanding it
      it = this->Fields.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

}
}
