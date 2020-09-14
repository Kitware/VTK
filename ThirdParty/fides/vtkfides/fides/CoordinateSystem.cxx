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

void CoordinateSystem::ProcessJSON(const rapidjson::Value& json,
                                   DataSourcesType& sources)
{
  this->Array.reset();
  if (!json.HasMember("array") || !json["array"].IsObject())
  {
    throw std::runtime_error(
      this->ObjectName  + " must provide an array object.");
  }
  this->Array = std::make_shared<fides::datamodel::Array>();
  this->Array->ObjectName = "array";
  this->Array->ProcessJSON(json["array"], sources);
}

size_t CoordinateSystem::GetNumberOfBlocks(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources)
{
  if (!this->NumberOfBlocksInitialized)
  {
    this->NumberOfBlocks =
      this->Array->GetNumberOfBlocks(paths, sources);
    this->NumberOfBlocksInitialized = true;
  }
  return this->NumberOfBlocks;
}

std::vector<vtkm::cont::CoordinateSystem> CoordinateSystem::Read(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections)
{
  //using CoordinateStorageTypes =
  //  vtkm::List<vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
  //                    vtkm::cont::StorageTagBasic>;
  // using VecType = vtkm::Vec<float, 3>;
  std::vector<vtkm::cont::VariantArrayHandle> arrays =
    this->Array->Read(paths, sources, selections);
  std::vector<vtkm::cont::CoordinateSystem> coordSystems;
   coordSystems.reserve(arrays.size());
  // using CoordsHandleType = vtkm::cont::ArrayHandle<VecType>;
  for(auto& array : arrays)
  {
    // auto expandedTypeArray = array.ResetStorageList(CoordinateStorageTypes{});
    // CoordsHandleType coordsHandle = array.Cast<CoordsHandleType>();
    // coordSystems.push_back(
    //   vtkm::cont::CoordinateSystem("coordinates", coordsHandle));
    coordSystems.push_back(
      vtkm::cont::CoordinateSystem("coordinates", array));
  }
  return coordSystems;
}

}
}
