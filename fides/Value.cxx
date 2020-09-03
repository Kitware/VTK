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

void Value::ProcessJSON(const rapidjson::Value& json,
                        DataSourcesType& sources)
{
  if (!json.HasMember("source") || !json["source"].IsString())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a valid source.");
  }
  const std::string& source = json["source"].GetString();
  if (source == "variable_dimensions")
  {
    this->ValueImpl.reset(new ValueVariableDimensions());
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

std::vector<vtkm::cont::VariantArrayHandle> Value::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  return this->ValueImpl->Read(paths, sources, selections);
}

size_t Value::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->ValueImpl->GetNumberOfBlocks(paths, sources);
}

std::vector<vtkm::cont::VariantArrayHandle> ValueVariableDimensions::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name "
      + this->DataSourceName + " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->GetVariableDimensions(this->VariableName, selections);
}

size_t ValueVariableDimensions::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name "
      + this->DataSourceName + " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->GetNumberOfBlocks(this->VariableName);
}

void ValueArray::ProcessJSON(const rapidjson::Value& json,
                             DataSourcesType& fidesNotUsed(sources))
{
  if (!json.HasMember("values") || !json["values"].IsArray())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide a valid values array.");
  }
  this->Values.clear();
  this->Values.reserve(json["values"].GetArray().Capacity());
  for (auto& v : json["values"].GetArray())
  {
    this->Values.push_back(v.GetDouble());
  }
}

std::vector<vtkm::cont::VariantArrayHandle> ValueArray::Read(
  const std::unordered_map<std::string, std::string>& fidesNotUsed(paths),
  DataSourcesType& fidesNotUsed(sources),
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  std::vector<vtkm::cont::VariantArrayHandle> retVal;
  retVal.push_back(
    vtkm::cont::make_ArrayHandle(this->Values, vtkm::CopyFlag::On));
  return retVal;
}

std::vector<vtkm::cont::VariantArrayHandle> ValueScalar::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name "
      + this->DataSourceName + " among the input paths.");
  }
  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  return ds->GetScalarVariable(this->VariableName, selections);
}

}
}
