//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/CoordinateSystem.h>

namespace fides
{
namespace datamodel
{

void CoordinateSystem::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  this->Array.reset();
  if (!json.HasMember("array") || !json["array"].IsObject())
  {
    throw std::runtime_error(this->ObjectName + " must provide an array object.");
  }
  this->Array = std::make_shared<fides::datamodel::Array>();
  this->Array->ObjectName = "array";
  this->Array->ProcessJSON(json["array"], sources);
}

size_t CoordinateSystem::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const std::string& groupName /*=""*/)
{
  // blocks can change per timestep, but it's also possible that the
  // mesh is written once, so we have to handle both situations
  auto nBlocks = this->Array->GetNumberOfBlocks(paths, sources, groupName);
  if (nBlocks > 0)
  {
    this->NumberOfBlocks = nBlocks;
  }
  return this->NumberOfBlocks;
}

std::set<std::string> CoordinateSystem::GetGroupNames(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  return this->Array->GetGroupNames(paths, sources);
}

std::vector<size_t> CoordinateSystem::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  OutputBuilder& builder)
{
  return this->Array->Read(paths, sources, selections, builder);
}

void CoordinateSystem::CollectReadRequests(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  std::vector<ReadRequest>& out)
{
  this->Array->CollectReadRequests(paths, sources, selections, out);
}

std::vector<size_t> CoordinateSystem::EmitTokens(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  const ReadResultMap& results,
  OutputBuilder& builder)
{
  return this->Array->EmitTokens(paths, sources, selections, results, builder);
}

void CoordinateSystem::PostRead(fides::DataContainer& partitions,
                                const fides::metadata::MetaData& selections)
{
  this->Array->PostRead(partitions, selections);
}

}
}
