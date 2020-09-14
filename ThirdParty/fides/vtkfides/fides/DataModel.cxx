//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/DataModel.h>

#include <fides/DataSource.h>

namespace fides
{
namespace datamodel
{

std::vector<vtkm::cont::VariantArrayHandle> DataModelBase::ReadSelf(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  fides::io::IsVector isItVector)
{
  if(this->IsStatic && !this->Cache.empty())
  {
    return this->Cache;
  }
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name "
      + this->DataSourceName + " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  std::vector<vtkm::cont::VariantArrayHandle> var =
    ds->ReadVariable(this->VariableName, selections, isItVector);
  if (this->IsStatic)
  {
    this->Cache = var;
  }
  return var;
}

std::string DataModelBase::FindDataSource(
  const rapidjson::Value& dataModel,
  DataSourcesType& sources) const
{
  if (!dataModel.HasMember("data_source"))
  {
    throw std::runtime_error(this->ObjectName + " must provide a data_source.");
  }
  std::string dsname = dataModel["data_source"].GetString();
  auto iter = sources.find(dsname);
  if(iter == sources.end())
  {
    throw std::runtime_error("data_source." + dsname + " was not found.");
  }
  return dsname;
}

void DataModelBase::ProcessJSON(const rapidjson::Value& json,
                                DataSourcesType& sources)
{
  if (!json.HasMember("variable"))
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a variable.");
  }
  std::string varName = json["variable"].GetString();
  this->VariableName = varName;
  this->DataSourceName = this->FindDataSource(json, sources);

  if (json.HasMember("static") && json["static"].IsBool())
  {
    if(json["static"].GetBool())
    {
      this->IsStatic = true;
    }
  }
}

}
}
