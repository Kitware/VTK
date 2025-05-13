//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataModelFactory_H
#define fides_datamodel_DataModelFactory_H

#include <fides/predefined/PredefinedDataModel.h>
#include <fides/predefined/SupportedDataModels.h>

#include <map>
#include <memory>

namespace fides
{
namespace predefined
{

// fwd declaration
class InternalMetadataSource;

/// \brief Singleton that is used to create a predefined data model.
class DataModelFactory
{
public:
  /// Returns a reference to the instance of this class
  static DataModelFactory& GetInstance();

  /// A callback used to create data model of a specific type
  typedef std::shared_ptr<PredefinedDataModel> (*CreateDataModelCallback)(
    std::shared_ptr<InternalMetadataSource>);
  typedef std::shared_ptr<PredefinedDataModel> (*CreateDataModelCallbackFromDS)(
    const viskores::cont::DataSet&);

  /// Register a predefined data model's callback with the factory
  bool RegisterDataModel(DataModelTypes modelId, CreateDataModelCallback createFn);
  bool RegisterDataModelFromDS(DataModelTypes modelId, CreateDataModelCallbackFromDS createFn);

  /// Unregister a predefined data model's callback with the factory
  bool UnregisterDataModel(DataModelTypes modelId);

  /// Create the predefined data model specified in the internal metadata source
  std::shared_ptr<PredefinedDataModel> CreateDataModel(
    std::shared_ptr<InternalMetadataSource> source);
  std::shared_ptr<PredefinedDataModel> CreateDataModel(const viskores::cont::DataSet& ds);

private:
  DataModelFactory() = default;
  DataModelFactory(const DataModelFactory&) = delete;
  DataModelFactory& operator=(const DataModelFactory&) = delete;
  virtual ~DataModelFactory();

  static void CreateInstance();

  static DataModelFactory* Instance;
  static bool Destroyed;

  typedef std::map<DataModelTypes, CreateDataModelCallback> CallbackMap;
  typedef std::map<DataModelTypes, CreateDataModelCallbackFromDS> CallbackMapFromDS;
  CallbackMap Callbacks;
  CallbackMapFromDS CallbacksFromDS;
};

}
}

#endif
