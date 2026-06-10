//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataModel_H_
#define fides_datamodel_DataModel_H_

#include <fides/DataSource.h>

#if FIDES_USE_VISKORES
#include <viskores/cont/UnknownArrayHandle.h>
#endif

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
// clang-format on

#include <string>
#include <unordered_map>
#include <vector>

namespace fides
{
namespace datamodel
{

using DataSourceType = fides::io::DataSource;
using DataSourcesType = std::unordered_map<std::string, std::shared_ptr<DataSourceType>>;

/// \brief Superclass for all data model classes.
///
/// Data model classes represent different structures that reside
/// in a dataset and that are mapped to different variables
/// read by data sources. This class provides common basic functionality
/// to all data model objects. These can be overriden by subclasses.
struct DataModelBase
{
  DataModelBase() = default;
  DataModelBase(const DataModelBase& other)
  {
    if (this != &other)
    {
      this->ObjectName = other.ObjectName;
      this->DataSourceName = other.DataSourceName;
      this->VariableName = other.VariableName;
      this->IsStatic = other.IsStatic;
    }
  }

  virtual ~DataModelBase() = default;

  /// Perform basic parsing of the JSON object, filling in
  /// common data members such as the data source and variable
  /// name.
  virtual void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources);

  std::string ObjectName = "";
  std::string DataSourceName = "";
  std::string VariableName = "";
  // Is the variable time dependent or static.
  bool IsStatic = false;

protected:
  std::string FindDataSource(const rapidjson::Value& dataModel, DataSourcesType& sources) const;

  // Data reading usually happens through this method, which works
  // with the data source. This also handles data caching for static
  // variables. RawArray caching works naturally via shared_ptr refcounting.
  std::vector<fides::RawArray> ReadSelf(const std::unordered_map<std::string, std::string>& paths,
                                        DataSourcesType& sources,
                                        const fides::metadata::MetaData& selections,
                                        fides::io::IsVector isItVector = fides::io::IsVector::Auto);

  std::vector<fides::RawArray> Cache;
};

template <typename T>
T GetRawArrayValueAs(const fides::RawArray& raw, size_t index)
{
  switch (raw.Type)
  {
    case fides::DataType::Float64:
      return static_cast<T>(raw.GetValue<double>(index));
    case fides::DataType::Float32:
      return static_cast<T>(raw.GetValue<float>(index));
    case fides::DataType::Int8:
      return static_cast<T>(raw.GetValue<int8_t>(index));
    case fides::DataType::Int16:
      return static_cast<T>(raw.GetValue<int16_t>(index));
    case fides::DataType::Int32:
      return static_cast<T>(raw.GetValue<int32_t>(index));
    case fides::DataType::Int64:
      return static_cast<T>(raw.GetValue<int64_t>(index));
    case fides::DataType::UInt8:
      return static_cast<T>(raw.GetValue<uint8_t>(index));
    case fides::DataType::UInt16:
      return static_cast<T>(raw.GetValue<uint16_t>(index));
    case fides::DataType::UInt32:
      return static_cast<T>(raw.GetValue<uint32_t>(index));
    case fides::DataType::UInt64:
      return static_cast<T>(raw.GetValue<uint64_t>(index));
    default:
      throw std::runtime_error("GetRawArrayValueAs: unsupported data type");
  }
}

#if FIDES_USE_VISKORES
/// Utility function that returns an ArrayHandle with the data from `uah`.
/// However, the returned ArrayHandle does not own data. The deleter of the Buffer is a no-op.
/// Kept temporarily for XGC/GTC/GX code that still uses Viskores types directly.
viskores::cont::UnknownArrayHandle make_ArrayHandleWithoutDataOwnership(
  const viskores::cont::UnknownArrayHandle& uah);
#endif

}
}

#endif
