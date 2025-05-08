//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/predefined/DataModelFactory.h>
#include <fides/predefined/InternalMetadataSource.h>
#include <fides/predefined/SupportedDataModels.h>

namespace fides
{
namespace predefined
{

DataModelFactory* DataModelFactory::Instance = nullptr;
bool DataModelFactory::Destroyed = false;

DataModelFactory::~DataModelFactory()
{
  Instance = nullptr;
  Destroyed = true;
}

DataModelFactory& DataModelFactory::GetInstance()
{
  if (!Instance)
  {
    if (Destroyed)
    {
      throw std::runtime_error("Dead reference to DataModelFactory singleton");
    }
    else
    {
      CreateInstance();
    }
  }
  return *Instance;
}

void DataModelFactory::CreateInstance()
{
  static DataModelFactory theInstance;
  Instance = &theInstance;
}

bool DataModelFactory::RegisterDataModel(DataModelTypes modelId, CreateDataModelCallback createFn)
{
  return this->Callbacks.insert(CallbackMap::value_type(modelId, createFn)).second;
}

bool DataModelFactory::RegisterDataModelFromDS(DataModelTypes modelId,
                                               CreateDataModelCallbackFromDS createFn)
{
  return this->CallbacksFromDS.insert(CallbackMapFromDS::value_type(modelId, createFn)).second;
}

bool DataModelFactory::UnregisterDataModel(DataModelTypes modelId)
{
  return this->Callbacks.erase(modelId) == 1;
}

struct GetDataSetTypeFunctor
{
  template <typename S>
  VISKORES_CONT void operator()(const viskores::cont::CellSetSingleType<S>&,
                                DataModelTypes& type) const
  {
    type = DataModelTypes::UNSTRUCTURED_SINGLE;
  }

  template <typename ShapesStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VISKORES_CONT void operator()(
    const viskores::cont::CellSetExplicit<ShapesStorage, ConnectivityStorage, OffsetsStorage>&,
    DataModelTypes& type) const
  {
    type = DataModelTypes::UNSTRUCTURED;
  }

  VISKORES_CONT void operator()(const viskores::cont::CellSet&, DataModelTypes& type) const
  {
    // in this case we didn't find an appropriate dataset type
    type = DataModelTypes::UNSUPPORTED;
  }
};

using CellSetSingleTypeList = viskores::List<
  viskores::cont::CellSetSingleType<>,
  viskores::cont::CellSetSingleType<
    viskores::cont::StorageTagCast<viskores::Int32, viskores::cont::StorageTagBasic>>>;

using CellSetExplicitList = viskores::List<
  viskores::cont::CellSetExplicit<>,
  viskores::cont::CellSetExplicit<
    viskores::cont::StorageTagBasic,
    viskores::cont::StorageTagCast<viskores::Int32, viskores::cont::StorageTagBasic>,
    viskores::cont::StorageTagCast<viskores::Int32, viskores::cont::StorageTagBasic>>>;

using FullCellSetExplicitList = viskores::ListAppend<CellSetSingleTypeList, CellSetExplicitList>;

std::shared_ptr<PredefinedDataModel> DataModelFactory::CreateDataModel(
  const viskores::cont::DataSet& ds)
{
  using UniformCoordType = viskores::cont::ArrayHandleUniformPointCoordinates;
  using RectilinearCoordType = viskores::cont::ArrayHandleCartesianProduct<
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>>;

  DataModelTypes modelId;
  if (ds.GetCoordinateSystem().GetData().IsType<UniformCoordType>())
  {
    modelId = DataModelTypes::UNIFORM;
  }
  else if (ds.GetCoordinateSystem().GetData().IsType<RectilinearCoordType>())
  {
    modelId = DataModelTypes::RECTILINEAR;
  }
  else
  {
    viskores::cont::UncertainCellSet<FullCellSetExplicitList> uncertainCS(ds.GetCellSet());
    uncertainCS.CastAndCall(GetDataSetTypeFunctor{}, modelId);
    if (modelId == DataModelTypes::UNSUPPORTED)
    {
      throw std::runtime_error("Unsupported data set type");
    }
  }

  auto it = this->CallbacksFromDS.find(modelId);
  if (it == this->CallbacksFromDS.end())
  {
    throw std::runtime_error(
      "Unknown data model ID provided to Fides for selecting predefined data model");
  }

  return (it->second)(ds);
}


std::shared_ptr<PredefinedDataModel> DataModelFactory::CreateDataModel(
  std::shared_ptr<InternalMetadataSource> source)
{
  auto modelId = source->GetDataModelType();
  auto it = this->Callbacks.find(modelId);
  if (it == this->Callbacks.end())
  {
    throw std::runtime_error(
      "Unknown data model ID provided to Fides for selecting predefined data model");
  }
  return (it->second)(source);
}

}
}
