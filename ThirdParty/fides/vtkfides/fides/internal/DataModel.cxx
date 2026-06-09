//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/DataModel.h>

#include <fides/DataSource.h>

#if FIDES_USE_VISKORES
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/Storage.h>
#endif

namespace fides
{
namespace datamodel
{

std::vector<fides::RawArray> DataModelBase::ReadSelf(
  const std::unordered_map<std::string, std::string>& paths,
  DataSourcesType& sources,
  const fides::metadata::MetaData& selections,
  fides::io::IsVector isItVector)
{
  if (this->IsStatic && !this->Cache.empty())
  {
    // RawArray uses shared_ptr internally, so returning a copy of the cache
    // shares the underlying data buffer without extra ownership machinery.
    return this->Cache;
  }

  const auto& ds = sources[this->DataSourceName];
  ds->OpenSource(paths, this->DataSourceName);
  std::vector<fides::RawArray> var;
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
  }
  return var;
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

#if FIDES_USE_VISKORES
struct ArrayHandleWithoutDataOwnership
{
public:
  viskores::cont::UnknownArrayHandle Handle;

  template <typename T, typename S>
  void operator()(viskores::cont::ArrayHandle<T, S> handle)
  {
    this->operator()(handle);
  }

  template <typename T>
  void operator()(viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> handle)
  {
    if (handle.GetBuffers().empty())
    {
      return;
    }
    handle.SyncControlArray();

    auto bufInfo = handle.GetBuffers()[0].GetHostBufferInfo();
    auto data = bufInfo.GetPointer();
    auto size = viskores::internal::NumberOfValuesToNumberOfBytes<T>(handle.GetNumberOfValues());

    // clang-format off
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> cacheHandle(std::vector<viskores::cont::internal::Buffer>{
      viskores::cont::internal::MakeBuffer(
        /*device=*/      viskores::cont::DeviceAdapterTagUndefined{},
        /*memory=*/      data,
        /*container=*/   data,
        /*size=*/        size,
        /*deleter=*/     [](void*) {}, // delete method is no-op
        /*reallocater=*/ viskores::cont::internal::InvalidRealloc)
    });
    // clang-format on

    this->Handle = cacheHandle;
  }

  template <typename T>
  void operator()(viskores::cont::ArrayHandle<T, viskores::cont::StorageTagSOA> handle)
  {
    if (handle.GetBuffers().empty())
    {
      return;
    }
    handle.SyncControlArray();
    auto srcBuffers = handle.GetBuffers();
    std::vector<viskores::cont::internal::Buffer> buffers;
    for (size_t i = 0; i < srcBuffers.size(); ++i)
    {
      auto bufInfo = srcBuffers[i].GetHostBufferInfo();
      auto data = bufInfo.GetPointer();
      auto size = bufInfo.GetSize();
      // clang-format off
      buffers.emplace_back(viskores::cont::internal::MakeBuffer(
        /*device=*/      viskores::cont::DeviceAdapterTagUndefined{},
        /*memory=*/      data,
        /*container=*/   data,
        /*size=*/        size,
        /*deleter=*/     [](void*) {}, // delete method is no-op
        /*reallocater=*/ viskores::cont::internal::InvalidRealloc)
      );
      // clang-format on
    }

    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagSOA> cacheHandle(std::move(buffers));
    this->Handle = cacheHandle;
  }
};

viskores::cont::UnknownArrayHandle make_ArrayHandleWithoutDataOwnership(
  const viskores::cont::UnknownArrayHandle& uah)
{
  ArrayHandleWithoutDataOwnership ownerlessAHBuilder;
  viskores::cont::CastAndCall(uah, ownerlessAHBuilder);
  return ownerlessAHBuilder.Handle;
}
#endif // FIDES_USE_VISKORES

}
}
