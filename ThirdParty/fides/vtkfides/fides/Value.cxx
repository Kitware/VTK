//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/Value.h>

namespace fides
{
namespace datamodel
{

void Value::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("source") || !json["source"].IsString())
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid source.");
  }
  const std::string& source = json["source"].GetString();
  if (source == "variable_dimensions")
  {
    this->ValueImpl.reset(new ValueVariableDimensions());
  }
  else if (source == "array_variable")
  {
    this->ValueImpl.reset(new ValueArrayVariable());
  }
  else if (source == "array")
  {
    this->ValueImpl.reset(new ValueArray());
  }
  else if (source == "scalar")
  {
    this->ValueImpl.reset(new ValueScalar());
  }
  else
  {
    throw std::runtime_error(source + " is not a valid source type.");
  }
  this->ValueImpl->ProcessJSON(json, sources);
}

std::vector<viskores::cont::UnknownArrayHandle> Value::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ValueImpl->Read(paths, sources, selections);
}

size_t Value::GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                DataSourcesType& sources,
                                const std::string& groupName /*=""*/)
{
  return this->ValueImpl->GetNumberOfBlocks(paths, sources, groupName);
}

std::set<std::string> Value::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->ValueImpl->GetGroupNames(paths, sources);
}

std::vector<viskores::cont::UnknownArrayHandle> ValueVariableDimensions::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetVariableDimensions(this->VariableName, selections);
}

size_t ValueVariableDimensions::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const std::string& groupName /*=""*/)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetNumberOfBlocks(this->VariableName, groupName);
}

std::set<std::string> ValueVariableDimensions::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetGroupNames(this->VariableName);
}

void ValueArray::ProcessJSON(const rapidjson::Value& json, DataSourcesType& fidesNotUsed(sources))
{
  if (!json.HasMember("values") || !json["values"].IsArray())
  {
    throw std::runtime_error(this->ObjectName + " must provide a valid values array.");
  }
  this->Values.clear();
  this->Values.reserve(json["values"].GetArray().Capacity());
  for (auto& v : json["values"].GetArray())
  {
    this->Values.push_back(v.GetDouble());
  }
}

std::vector<viskores::cont::UnknownArrayHandle> ValueArray::Read(
  const std::unordered_map<std::string, std::string>& fidesNotUsed(paths),
  DataSourcesType& fidesNotUsed(sources),
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  std::vector<viskores::cont::UnknownArrayHandle> retVal;
  retVal.push_back(viskores::cont::make_ArrayHandle(this->Values, viskores::CopyFlag::On));
  return retVal;
}

std::set<std::string> ValueScalar::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetGroupNames(this->VariableName);
}

std::vector<viskores::cont::UnknownArrayHandle> ValueScalar::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetScalarVariable(this->VariableName, selections);
}

std::vector<viskores::cont::UnknownArrayHandle> ValueArrayVariable::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);

  auto arrays = ds->ReadVariable(this->VariableName, selections);
  return arrays;
}

size_t ValueArrayVariable::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const std::string& groupName /*=""*/)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetNumberOfBlocks(this->VariableName, groupName);
}

std::set<std::string> ValueArrayVariable::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetGroupNames(this->VariableName);
}

}
}
