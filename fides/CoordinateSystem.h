//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_CoordinateSystem_H_
#define fides_datamodel_CoordinateSystem_H_

#include <fides/Array.h>
#include <fides/DataModel.h>
#include <fides/MetaData.h>

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/PartitionedDataSet.h>

namespace fides
{
namespace datamodel
{

/// \brief Data model object for Viskores coordinate systems.
///
/// \c fides::datamodel::CoordinateSystem is responsible of creating
/// Viskores coordinate systems by loading data defined by the Fides
/// data model.
struct CoordinateSystem : public DataModelBase
{
  /// Overridden to handle the undelying Array. The Array
  /// object determines the actual type of the coordinate system.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns coordinate systems. The heavy-lifting is
  /// handled by the underlying Array object.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::CoordinateSystem> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections);

  /// Returns the number of blocks in the underlying Array variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "");

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>& paths,
                                      DataSourcesType& sources);

private:
  std::shared_ptr<fides::datamodel::Array> Array;
  size_t NumberOfBlocks = 0;
};

}
}

#endif
