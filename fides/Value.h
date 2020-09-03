//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_Value_H_
#define fides_datamodel_Value_H_

#include <vtkm/cont/VariantArrayHandle.h>

#include <fides/DataModel.h>

namespace fides
{
namespace datamodel
{
/// \brief Superclass for all specialized value implementations.
struct ValueBase : public DataModelBase
{
  /// Reads and returns array handles. Has to be implemented
  /// by subclasses.
  virtual std::vector<vtkm::cont::VariantArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) = 0;

  virtual size_t GetNumberOfBlocks(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources) = 0;

    virtual ~ValueBase() {};
};

/// \brief Class to handle values needed at dataset creation time.
///
/// \c Value is a data model object that handles values that are needed
/// at the time of a dataset's creation. This is different than at the
/// time of loading of actual data values such as arrays and coordinates.
/// Examples include the dimensions, origin and spacing of a structured
/// dataset.
struct Value : public DataModelBase
{
  /// Overridden to handle Value specific items.
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Reads and returns values.
  /// This method should not depend on DataSource::ReadVariable
  /// as actual IO for the variable is done after this method is
  /// called whereas the return value of this method is used
  /// immediately.
  std::vector<vtkm::cont::VariantArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  /// Returns the number of blocks in the underlying variable (if any).
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources);

private:
  std::unique_ptr<ValueBase> ValueImpl = nullptr;
};

/// \brief \c ValueBase subclass that provides values based on dimensions (shape) of a variable
///
/// \c ValueVariableDimensions reads the dimensions (shape) as well as the
/// start of an n-dimensional variable. The first n values are the dimensions
/// and the following are the start indices.
struct ValueVariableDimensions : public ValueBase
{
  /// Reads the dimensions (shape) as well as the
  /// start of an n-dimensional variable. The first n values are the dimensions
  /// and the following are the start indices.
  std::vector<vtkm::cont::VariantArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable.
  size_t GetNumberOfBlocks(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources) override;
};

/// \brief \c ValueBase subclass that provides array of values from json.
///
/// \c ValueArray reads its values from a provided json array. Currently,
/// the values are assumed to be double.
struct ValueArray : public ValueBase
{
  /// These values are assumed to be global so always returns 1.
  size_t GetNumberOfBlocks(
    const std::unordered_map<std::string, std::string>&,
    DataSourcesType&) override {return 1;}

  /// Overridden to parse the value array.
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Returns array values read from json.
  std::vector<vtkm::cont::VariantArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  std::vector<double> Values;
};

/// \brief \c ValueBase subclass that can read and immediately return a
/// scalar value from a data source.
struct ValueScalar : public ValueBase
{
  /// Always a single value, so always returns 1.
  size_t GetNumberOfBlocks(
    const std::unordered_map<std::string, std::string>&,
    DataSourcesType&) override {return 1;}

  /// Reads the variable
  std::vector<vtkm::cont::VariantArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;
};

}
}

#endif
