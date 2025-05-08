//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/CoordinateSystem.h>

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

std::vector<viskores::cont::CoordinateSystem> CoordinateSystem::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  //using CoordinateStorageTypes =
  //  viskores::List<viskores::cont::ArrayHandleUniformPointCoordinates::StorageTag,
  //                    viskores::cont::StorageTagBasic>;
  // using VecType = viskores::Vec<float, 3>;
  std::vector<viskores::cont::UnknownArrayHandle> arrays =
    this->Array->Read(paths, sources, selections);
  std::vector<viskores::cont::CoordinateSystem> coordSystems;
  coordSystems.reserve(arrays.size());
  // using CoordsHandleType = viskores::cont::ArrayHandle<VecType>;
  for (auto& array : arrays)
  {
    // auto expandedTypeArray = array.ResetStorageList(CoordinateStorageTypes{});
    // CoordsHandleType coordsHandle = array.Cast<CoordsHandleType>();
    // coordSystems.push_back(
    //   viskores::cont::CoordinateSystem("coordinates", coordsHandle));
    coordSystems.push_back(viskores::cont::CoordinateSystem("coordinates", array));
  }
  return coordSystems;
}

void CoordinateSystem::PostRead(std::vector<viskores::cont::DataSet>& partitions,
                                const fides::metadata::MetaData& selections)
{
  this->Array->PostRead(partitions, selections);
}

}
}
