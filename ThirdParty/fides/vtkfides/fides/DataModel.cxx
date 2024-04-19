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

struct ArrayHandleWithoutDataOwnership
{
public:
  vtkm::cont::UnknownArrayHandle Handle;

  template <typename T, typename S>
  void operator()(vtkm::cont::ArrayHandle<T, S> handle)
  {
    this->operator()(handle);
  }

  template <typename T>
  void operator()(vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> handle)
  {
    if (handle.GetBuffers().empty())
    {
      return;
    }
    handle.SyncControlArray();

    auto bufInfo = handle.GetBuffers()[0].GetHostBufferInfo();
    auto data = bufInfo.GetPointer();
    auto size = vtkm::internal::NumberOfValuesToNumberOfBytes<T>(handle.GetNumberOfValues());

    // clang-format off
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> cacheHandle(std::vector<vtkm::cont::internal::Buffer>{
      vtkm::cont::internal::MakeBuffer(
        /*device=*/      vtkm::cont::DeviceAdapterTagUndefined{},
        /*memory=*/      data,
        /*container=*/   data,
        /*size=*/        size,
        /*deleter=*/     [](void*) {}, // delete method is no-op
        /*reallocater=*/ vtkm::cont::internal::InvalidRealloc)
    });
    // clang-format on

    this->Handle = cacheHandle;
  }

  template <typename T>
  void operator()(vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagSOA> handle)
  {
    if (handle.GetBuffers().empty())
    {
      return;
    }
    handle.SyncControlArray();
    auto srcBuffers = handle.GetBuffers();
    std::vector<vtkm::cont::internal::Buffer> buffers;
    for (size_t i = 0; i < srcBuffers.size(); ++i)
    {
      auto bufInfo = srcBuffers[i].GetHostBufferInfo();
      auto data = bufInfo.GetPointer();
      auto size = bufInfo.GetSize();
      // clang-format off
      buffers.emplace_back(vtkm::cont::internal::MakeBuffer(
        /*device=*/      vtkm::cont::DeviceAdapterTagUndefined{},
        /*memory=*/      data,
        /*container=*/   data,
        /*size=*/        size,
        /*deleter=*/     [](void*) {}, // delete method is no-op
        /*reallocater=*/ vtkm::cont::internal::InvalidRealloc)
      );
      // clang-format on
    }

    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagSOA> cacheHandle(std::move(buffers));
    this->Handle = cacheHandle;
  }
};

vtkm::cont::UnknownArrayHandle make_ArrayHandleWithoutDataOwnership(
  const vtkm::cont::UnknownArrayHandle& uah)
{
  ArrayHandleWithoutDataOwnership ownerlessAHBuilder;
  vtkm::cont::CastAndCall(uah, ownerlessAHBuilder);
  return ownerlessAHBuilder.Handle;
}

std::vector<vtkm::cont::UnknownArrayHandle> make_ArrayHandlesWithoutDataOwnership(
  const std::vector<vtkm::cont::UnknownArrayHandle>& uahs)
{
  std::vector<vtkm::cont::UnknownArrayHandle> ownerlessUAHs;
  ownerlessUAHs.reserve(uahs.size());
  for (auto& uah : uahs)
  {
    ownerlessUAHs.emplace_back(make_ArrayHandleWithoutDataOwnership(uah));
  }
  return ownerlessUAHs;
}

std::vector<vtkm::cont::UnknownArrayHandle> DataModelBase::ReadSelf(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  fides::io::IsVector isItVector)
{
  if (this->IsStatic && !this->Cache.empty())
  {
    return make_ArrayHandlesWithoutDataOwnership(this->Cache);
  }
  auto itr = paths.find(this->DataSourceName);
  if (itr == paths.end())
  {
    throw std::runtime_error("Could not find data_source with name " + this->DataSourceName +
                             " among the input paths.");
  }

  const auto& ds = sources[this->DataSourceName];
  std::string path = itr->second + ds->FileName;
  ds->OpenSource(path);
  std::vector<vtkm::cont::UnknownArrayHandle> var;
  bool readAsMultiBlock = false;
  if (selections.Has(fides::keys::READ_AS_MULTIBLOCK()))
  {
    readAsMultiBlock =
      selections.Get<fides::metadata::Bool>(fides::keys::READ_AS_MULTIBLOCK()).Value;
  }

  if (readAsMultiBlock)
  {
    var = ds->ReadMultiBlockVariable(this->VariableName, selections);
  }
  else
  {
    var = ds->ReadVariable(this->VariableName, selections, isItVector);
  }
  if (this->IsStatic)
  {
    this->Cache = var;
    return make_ArrayHandlesWithoutDataOwnership(var);
  }
  else
  {
    return var;
  }
}

std::string DataModelBase::FindDataSource(const rapidjson::Value& dataModel,
                                          DataSourcesType& sources) const
{
  if (!dataModel.HasMember("data_source"))
  {
    throw std::runtime_error(this->ObjectName + " must provide a data_source.");
  }
  std::string dsname = dataModel["data_source"].GetString();
  auto iter = sources.find(dsname);
  if (iter == sources.end())
  {
    throw std::runtime_error("data_source." + dsname + " was not found.");
  }
  return dsname;
}

void DataModelBase::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  if (!json.HasMember("variable"))
  {
    throw std::runtime_error(this->ObjectName + " must provide a variable.");
  }

  std::string varName = json["variable"].GetString();
  this->VariableName = varName;
  this->DataSourceName = this->FindDataSource(json, sources);

  if (json.HasMember("static") && json["static"].IsBool())
  {
    if (json["static"].GetBool())
    {
      this->IsStatic = true;
    }
  }
}

}
}
