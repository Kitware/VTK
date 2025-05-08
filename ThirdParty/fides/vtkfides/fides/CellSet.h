//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_CellSet_H_
#define fides_datamodel_CellSet_H_

#include <fides/Array.h>
#include <fides/DataModel.h>
#include <fides/Value.h>
#include <fides/xgc/XGCCommon.h>

#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownCellSet.h>

namespace fides
{
namespace datamodel
{

/// \brief Superclass for all specific cellset implementations.
///
/// \c CellSetBase and its subclasses are internal to the \c CellSet
/// class. They handle specific cellset cases. \c CellSetBase is
/// an abstract class that establishes the API.
struct CellSetBase : public DataModelBase
{
  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  virtual std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) = 0;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  virtual void PostRead(std::vector<viskores::cont::DataSet>&, const fides::metadata::MetaData&) {}

  virtual ~CellSetBase(){};
};

/// \brief Data model object for Viskores cell sets.
///
/// \c fides::datamodel::CellSet is responsible of creating
/// a Viskores cell set for each block. Note that this class
/// acts as a variant in that it will create a class of the
/// appropriate type to handle the specific cell set type
/// in use and delegate functionality to that class.
struct CellSet : public DataModelBase
{
  /// Overridden to handle CellSet specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections);

private:
  std::unique_ptr<CellSetBase> CellSetImpl = nullptr;
};

/// \brief Class to read unstructured grids of single cell type.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetSingleType objects.
struct CellSetSingleType : public CellSetBase
{
  /// Overridden to handle CellSetSingleType specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, it is necessary to know the number
  /// of points to create the Viskores cellset.
  virtual void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                        const fides::metadata::MetaData& selections) override;

protected:
  std::pair<unsigned char, int> CellInformation;
  std::vector<viskores::cont::UnknownCellSet> CellSetCache;
  std::vector<viskores::cont::UnknownArrayHandle> ConnectivityArrays;
};

/// \brief Class to read unstructured grids of mixed cell types.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetExplicit objects.
struct CellSetExplicit : public CellSetBase
{
  /// Overridden to handle CellSetExplicit specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, it is necessary to know the number
  /// of points to create the Viskores cellset.
  virtual void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                        const fides::metadata::MetaData& selections) override;

protected:
  std::vector<viskores::cont::UnknownCellSet> CellSetCache;
  std::unique_ptr<Array> CellTypes = nullptr;
  std::unique_ptr<Array> NumberOfVertices = nullptr;
  std::unique_ptr<Array> Connectivity = nullptr;
  std::vector<viskores::cont::UnknownArrayHandle> CellTypesArrays;
  std::vector<viskores::cont::UnknownArrayHandle> NumberOfVerticesArrays;
  std::vector<viskores::cont::UnknownArrayHandle> ConnectivityArrays;
};

/// \brief Class to read structured grids.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetStructured objects.
struct CellSetStructured : public CellSetBase
{
  /// Overridden to handle CellSetSingleType specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, the dimensions, origin, and spacing
  /// can be read from file, which is not available at Read() time.
  virtual void PostRead(std::vector<viskores::cont::DataSet>&,
                        const fides::metadata::MetaData&) override;

private:
  std::unique_ptr<Value> Dimensions = nullptr;
  std::vector<viskores::cont::UnknownArrayHandle> DimensionArrays;
};


/// \brief Class to read XGC 2.5D cell set.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetXGC objects.
struct CellSetXGC : public CellSetBase
{
  CellSetXGC()
    : CommonImpl(new XGCCommon())
  {
  }

  /// Overridden to handle CellSetXGC specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  virtual void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                        const fides::metadata::MetaData& selections) override;

  // cuda doesn't like this class being private...
  class CalcPsi;

private:
  std::vector<viskores::cont::UnknownCellSet> CellSetCache;
  std::unique_ptr<Array> CellConnectivity = nullptr;
  std::unique_ptr<Array> PlaneConnectivity = nullptr;
  viskores::Id NumberOfPlanes = -1;
  bool IsPeriodic = true;
  std::unique_ptr<XGCCommon> CommonImpl;
};

/// \brief Class to read GTC cell set.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetGTC objects.
struct CellSetGTC : public CellSetBase
{
  /// Overridden to handle CellSetGTC specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections) override;

private:
  using GTCCoordsType32 = viskores::cont::ArrayHandleSOA<viskores::Vec3f_32>;
  using GTCCoordsType64 = viskores::cont::ArrayHandleSOA<viskores::Vec3f_64>;

  bool IsCached = false;
  viskores::cont::UnknownCellSet CachedCellSet;

  void ComputeCellSet(viskores::cont::DataSet& dataSet);

  template <typename T, typename C>
  std::vector<viskores::Id> ComputeConnectivity(
    const viskores::cont::ArrayHandle<viskores::Vec<T, 3>, C>& coords,
    const viskores::cont::ArrayHandle<int>& igrid,
    const viskores::cont::ArrayHandle<int>& indexShift);

  std::vector<viskores::cont::UnknownArrayHandle> IGridArrays;
  std::vector<viskores::cont::UnknownArrayHandle> IndexShiftArrays;
  std::unique_ptr<Array> IGrid = nullptr;
  std::unique_ptr<Array> IndexShift = nullptr;
  viskores::Id NumberOfPlanes = -1;
  viskores::Id NumberOfPointsPerPlane = -1;

  bool RArrayCached = false;
  bool PhiArrayCached = false;
  viskores::cont::ArrayHandle<viskores::Float32> RArray;
  viskores::cont::ArrayHandle<viskores::Float32> PhiArray;
  bool PeriodicCellSet = true;
};

/// \brief Class to read GX cell set.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetBoutpp objects.
struct CellSetGX : public CellSetBase
{
  CellSetGX()
    : CellSetBase()
  {
  }

  /// Overridden to handle CellSetGX specific items.
  void ProcessJSON(const rapidjson::Value&, DataSourcesType&) override {}

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  virtual void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                        const fides::metadata::MetaData& selections) override;

private:
  viskores::Id GetMetaDataValue(const viskores::cont::DataSet& ds,
                                const std::string& fieldNm) const;
};


}
}
#endif
