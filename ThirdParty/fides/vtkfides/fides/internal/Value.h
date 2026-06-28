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

#include <fides/internal/DataModel.h>

namespace fides
{
namespace datamodel
{
/// \brief Superclass for all specialized value implementations.
struct ValueBase : public DataModelBase
{
  /// Reads and returns raw arrays. Has to be implemented
  /// by subclasses. The \c mode parameter is forwarded to
  /// \c DataSource::ReadVariable by subclasses that go through the data
  /// source (currently only \c ValueArrayVariable); others ignore it.
  /// Defaults to \c Deferred to match the \c DataSource convention, but
  /// callers that dereference the returned \c RawArray inline (before
  /// the next \c EndStep() / \c PerformGets() flush) must pass \c Sync.
  virtual std::vector<fides::RawArray> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections,
    fides::io::ReadMode mode = fides::io::ReadMode::Deferred) = 0;

  virtual size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources,
                                   const std::string& groupName = "") = 0;

  /// Returns the groups that have the underlying value
  /// Used by the reader to provide group names
  /// Has to be implemented by subclasses.
  virtual std::set<std::string> GetGroupNames(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources) = 0;

  virtual ~ValueBase(){};
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
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns values as RawArrays. The \c mode parameter is
  /// forwarded to the underlying \c ValueBase implementation; callers
  /// that dereference the returned \c RawArray inline (before the next
  /// \c EndStep() / \c PerformGets() flush) must pass \c Sync.
  std::vector<fides::RawArray> Read(const std::unordered_map<std::string, std::string>& paths,
                                    DataSourcesType& sources,
                                    const fides::metadata::MetaData& selections,
                                    fides::io::ReadMode mode = fides::io::ReadMode::Deferred);

  /// Returns the number of blocks in the underlying variable inside the group (if any).
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "");

  /// Returns the groups that have the underlying value
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&);

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
  /// Overridden to parse the variable source and optional association.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads the dimensions (shape) as well as the
  /// start of an n-dimensional variable. The first n values are the dimensions
  /// and the following are the start indices. The \c mode parameter is
  /// ignored (this path goes through \c GetVariableDimensions, not
  /// \c ReadVariable).
  std::vector<fides::RawArray> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections,
    fides::io::ReadMode mode = fides::io::ReadMode::Deferred) override;

  /// Returns the number of blocks in the underlying variable inside the group
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Returns the groups that have the underlying value
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

private:
  fides::FieldAssociation Association = fides::FieldAssociation::Points;
};

/// \brief \c ValueBase subclass that provides values from an array
///
/// \c ValueArrayVariable reads its values from the provided array name.
/// Currently, the values are assumed to be double, or std::size_t and are
/// used for the metadata describing uniform grids.
struct ValueArrayVariable : public ValueBase
{
  std::vector<fides::RawArray> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections,
    fides::io::ReadMode mode = fides::io::ReadMode::Deferred) override;

  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Returns the groups that have the underlying value
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;
};

/// \brief \c ValueBase subclass that provides array of values from json.
///
/// \c ValueArray reads its values from a provided json array. Currently,
/// the values are assumed to be double.
struct ValueArray : public ValueBase
{
  /// These values are assumed to be global so always returns 1.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&,
                           const std::string& = "") override
  {
    return 1;
  }

  /// Returns the groups that have the underlying value
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override
  {
    return {};
  }

  /// Overridden to parse the value array.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Returns array values read from json. The \c mode parameter is
  /// ignored (values come from JSON, not a data source).
  std::vector<fides::RawArray> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections,
    fides::io::ReadMode mode = fides::io::ReadMode::Deferred) override;

  std::vector<double> Values;
};

/// \brief \c ValueBase subclass that can read and immediately return a
/// scalar value from a data source.
struct ValueScalar : public ValueBase
{
  /// Always a single value, so always returns 1.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&,
                           const std::string& = "") override
  {
    return 1;
  }

  /// Returns the groups that have the underlying value
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

  /// Reads the variable. The \c mode parameter is ignored
  /// (\c GetScalarVariable is always synchronous internally).
  std::vector<fides::RawArray> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections,
    fides::io::ReadMode mode = fides::io::ReadMode::Deferred) override;
};

}
}

#endif
