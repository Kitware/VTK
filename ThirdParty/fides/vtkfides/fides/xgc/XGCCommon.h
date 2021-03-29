//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_XGCCommon_H_
#define fides_datamodel_XGCCommon_H_

#include <fides/DataModel.h>
#include <fides/Value.h>

#include <memory>

namespace fides
{
namespace datamodel
{

/// Provides info that helps to convert from an Fides XGCBlock,
/// into ADIOS blocks for reading. Also useful for setting up the
/// VTK-m ArrayHandles.
struct XGCBlockInfo
{
  size_t BlockId;
  vtkm::Id NumberOfPlanesOwned;
  vtkm::Id PlaneStartId;
};

/// \brief Has common functionality for XGC that can be used
/// by ArrayXGC and its subclasses, as well as CellSetXGC.
/// This class is responsible for determining the number of Fides blocks
/// for the given XGC configuration, and maps the planes to blocks.
struct XGCCommon
{
  XGCCommon();
  ~XGCCommon();

  /// processes the JSON for the number_of_planes object
  static void ProcessNumberOfPlanes(const rapidjson::Value& nPlanes, DataSourcesType& sources);

  /// Reads number of planes from data and returns it immediately
  vtkm::Id GetNumberOfPlanes(const std::unordered_map<std::string, std::string>& paths,
                             DataSourcesType& sources);

  /// Gets the number of Fides blocks. Will throw an error if the
  /// planes have not been mapped to blocks yet.
  size_t GetNumberOfBlocks();

  /// Gets the XGCBlock info for the blocks requested by the user.
  /// If userBlocks is not provided/empty, then it is assumed that all
  /// blocks are being requested.
  std::vector<XGCBlockInfo> GetXGCBlockInfo(const std::vector<size_t>& userBlocks);

  /// Gets the XGCBlock info for the blocks requested by the user, but
  /// also returns a set of plane selections to be used for reading 3D
  /// variables.
  /// If userBlocks is not provided/empty, then it is assumed that all
  /// blocks are being requested.
  std::pair<std::vector<XGCBlockInfo>, fides::metadata::Set<size_t>>
  GetXGCBlockInfoWithPlaneSelection(const std::vector<size_t>& userBlocks);

private:
  class XGCCommonImpl;
  std::unique_ptr<XGCCommonImpl> Impl;
  static std::shared_ptr<Value> NumberOfPlanes;
  static vtkm::Id PlanesPerUserBlock;
};

}
}
#endif
