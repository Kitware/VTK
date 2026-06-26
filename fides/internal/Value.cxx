//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/Value.h>

#include <algorithm>

namespace fides
{
namespace datamodel
{

namespace
{

fides::FieldAssociation ParseAssociation(const std::string& assoc, const std::string& objectName)
{
  if (assoc == "points")
  {
    return fides::FieldAssociation::Points;
  }
  if (assoc == "cell_set" || assoc == "cells")
  {
    return fides::FieldAssociation::Cells;
  }

  throw std::runtime_error(objectName + " provided unknown association: " + assoc);
}

}

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

std::vector<fides::RawArray> Value::Read(const std::unordered_map<std::string, std::string>& paths,
                                         DataSourcesType& sources,
                                         const fides::metadata::MetaData& selections,
                                         fides::io::ReadMode mode)
{
  return this->ValueImpl->Read(paths, sources, selections, mode);
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

void ValueVariableDimensions::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->DataModelBase::ProcessJSON(json, sources);

  if (json.HasMember("association") && json["association"].IsString())
  {
    this->Association = ParseAssociation(json["association"].GetString(), this->ObjectName);
  }
}

std::vector<fides::RawArray> ValueVariableDimensions::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  fides::io::ReadMode /*mode*/)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  auto arrays = ds->GetVariableDimensions(this->VariableName, selections, this->Association);

  if (this->Association != fides::FieldAssociation::Cells)
  {
    return arrays;
  }

  std::vector<fides::RawArray> adjustedArrays;
  adjustedArrays.reserve(arrays.size());
  for (const auto& array : arrays)
  {
    auto dims = fides::AllocateRawArray<std::size_t>(array.NumValues);
    std::copy_n(
      array.GetPointer<std::size_t>(), array.NumValues, dims.GetWritePointer<std::size_t>());
    auto* ptr = dims.GetWritePointer<std::size_t>();
    auto numDims = dims.NumValues;
    if (numDims > 3)
    {
      numDims /= 2;
    }
    for (std::size_t i = 0; i < numDims; ++i)
    {
      ptr[i] += 1;
    }
    adjustedArrays.push_back(std::move(dims));
  }

  return adjustedArrays;
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
  for (const auto& v : json["values"].GetArray())
  {
    this->Values.push_back(v.GetDouble());
  }
}

std::vector<fides::RawArray> ValueArray::Read(
  const std::unordered_map<std::string, std::string>& fidesNotUsed(paths),
  DataSourcesType& fidesNotUsed(sources),
  const fides::metadata::MetaData& fidesNotUsed(selections),
  fides::io::ReadMode /*mode*/)
{
  auto raw = fides::AllocateRawArray<double>(this->Values.size());
  std::memcpy(
    raw.GetWritePointer<double>(), this->Values.data(), this->Values.size() * sizeof(double));

  std::vector<fides::RawArray> retVal;
  retVal.push_back(std::move(raw));
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

std::vector<fides::RawArray> ValueScalar::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  fides::io::ReadMode /*mode*/)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->GetScalarVariable(this->VariableName, selections);
}

std::vector<fides::RawArray> ValueArrayVariable::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  fides::io::ReadMode mode)
{
  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  return ds->ReadVariable(this->VariableName, selections, fides::io::IsVector::Auto, mode);
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
