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

#include <fides/DataContainer.h>
#include <fides/internal/Array.h>
#include <fides/internal/DataModel.h>
#include <fides/internal/Value.h>

#if FIDES_USE_VISKORES
#include <fides/xgc/XGCCommon.h>

#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownCellSet.h>
#endif

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
  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  ///
  /// Legacy single-pass entry point — issues \c DataSource reads inline.
  /// Concrete subclasses are being converted to the two-pass
  /// \c CollectReadRequests / \c EmitTokens flow; until conversion is
  /// complete, the default \c EmitTokens implementation falls back to
  /// calling \c Read.
  virtual std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources,
                                   const fides::metadata::MetaData& selections,
                                   OutputBuilder& builder) = 0;

  /// Two-pass plan phase: append the \c ReadRequest entries this cell
  /// set needs to satisfy a subsequent \c EmitTokens call. Default:
  /// no-op (subclass still uses legacy \c Read via the default
  /// \c EmitTokens).
  virtual void CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources,
                                   const fides::metadata::MetaData& selections,
                                   std::vector<ReadRequest>& out);

  /// Two-pass emit phase: pull \c RawArrays out of \c results and emit
  /// the corresponding \c OutputBuilder tokens. Default: ignores
  /// \c results and delegates to legacy \c Read.
  virtual std::vector<size_t> EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                         DataSourcesType& sources,
                                         const fides::metadata::MetaData& selections,
                                         const ReadResultMap& results,
                                         OutputBuilder& builder);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  virtual void PostRead(fides::DataContainer&, const fides::metadata::MetaData&);

  virtual ~CellSetBase(){};

protected:
#if FIDES_USE_VISKORES
  virtual void ProcessViskores(std::vector<viskores::cont::DataSet>&,
                               const fides::metadata::MetaData&)
  {
  }
#endif

#if FIDES_USE_VTK
  virtual void ProcessVTK(fides::VTKCollection&, const fides::metadata::MetaData&) {}
#endif
};

/// \brief Data model object for cell sets.
///
/// \c fides::datamodel::CellSet is responsible for creating
/// cell sets for each block. Note that this class
/// acts as a variant in that it will create a class of the
/// appropriate type to handle the specific cell set type
/// in use and delegate functionality to that class.
struct CellSet : public DataModelBase
{
  /// Overridden to handle CellSet specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder);

  /// Two-pass plan phase: forwards to the underlying CellSetBase.
  void CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           std::vector<ReadRequest>& out);

  /// Two-pass emit phase: forwards to the underlying CellSetBase.
  std::vector<size_t> EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                 DataSourcesType& sources,
                                 const fides::metadata::MetaData& selections,
                                 const ReadResultMap& results,
                                 OutputBuilder& builder);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(fides::DataContainer& container, const fides::metadata::MetaData& selections);

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

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

  void CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           std::vector<ReadRequest>& out) override;

  std::vector<size_t> EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                 DataSourcesType& sources,
                                 const fides::metadata::MetaData& selections,
                                 const ReadResultMap& results,
                                 OutputBuilder& builder) override;

protected:
#if FIDES_USE_VISKORES
  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, it is necessary to know the number
  /// of points to create the Viskores cellset.
  void ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                       const fides::metadata::MetaData& selections) override;
#endif

  std::pair<unsigned char, int> CellInformation;
  std::vector<size_t> CellSetCache;
  std::vector<fides::RawArray> ConnectivityArrays;
};

/// \brief Class to read unstructured grids of mixed cell types.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetExplicit objects.
struct CellSetExplicit : public CellSetBase
{
  /// Overridden to handle CellSetExplicit specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

  void CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           std::vector<ReadRequest>& out) override;

  std::vector<size_t> EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                 DataSourcesType& sources,
                                 const fides::metadata::MetaData& selections,
                                 const ReadResultMap& results,
                                 OutputBuilder& builder) override;

protected:
#if FIDES_USE_VISKORES
  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, it is necessary to know the number
  /// of points to create the Viskores cellset.
  void ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                       const fides::metadata::MetaData& selections) override;
#endif

  std::vector<size_t> CellSetCache;
  std::unique_ptr<Array> CellTypes = nullptr;
  std::unique_ptr<Array> NumberOfVertices = nullptr;
  std::unique_ptr<Array> Connectivity = nullptr;
  std::vector<fides::RawArray> CellTypesArrays;
  std::vector<fides::RawArray> NumberOfVerticesArrays;
  std::vector<fides::RawArray> ConnectivityArrays;
};

/// \brief Class to read polydata cell sets.
///
/// Polydata partitions are represented as four optional (offsets,
/// connectivity) pairs, one per vtkPolyData cell-array role (verts,
/// lines, polys, strips). Any pair may be absent if the partition has
/// no cells of that kind. Cell-type semantics are implicit per role +
/// vertex count, so no per-cell type byte is stored.
struct CellSetPolyData : public CellSetBase
{
  /// Overridden to handle CellSetPolyData specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns cell set tokens via the OutputBuilder.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

  void CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           std::vector<ReadRequest>& out) override;

  std::vector<size_t> EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                 DataSourcesType& sources,
                                 const fides::metadata::MetaData& selections,
                                 const ReadResultMap& results,
                                 OutputBuilder& builder) override;

#if FIDES_USE_VISKORES
  /// Viskores has no native polydata. Flatten the four cell-array roles
  /// into a single CellSetExplicit so downstream Viskores filters can
  /// still consume the data; strips are expanded to triangles.
  void ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                       const fides::metadata::MetaData& selections) override;
#endif

  /// One vtkPolyData cell-array role. \c Offsets / \c Connectivity are
  /// non-null when the role appears in the JSON schema; the raw vectors
  /// are populated by EmitTokens after the read plan executes. Public so
  /// helpers in CellSet.cxx can populate it during ProcessJSON.
  struct Role
  {
    std::unique_ptr<Array> Offsets;
    std::unique_ptr<Array> Connectivity;
    std::vector<fides::RawArray> OffsetsArrays;
    std::vector<fides::RawArray> ConnArrays;
    bool Present() const { return Offsets != nullptr && Connectivity != nullptr; }
  };

protected:
  Role Verts;
  Role Lines;
  Role Polys;
  Role Strips;
};

/// \brief Class to read structured grids.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetStructured objects.
struct CellSetStructured : public CellSetBase
{
  /// Overridden to handle CellSetSingleType specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

protected:
#if FIDES_USE_VISKORES
  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, the dimensions, origin, and spacing
  /// can be read from file, which is not available at Read() time.
  void ProcessViskores(std::vector<viskores::cont::DataSet>&,
                       const fides::metadata::MetaData&) override;
#endif

private:
  std::unique_ptr<Value> Dimensions = nullptr;
  std::vector<fides::RawArray> DimensionArrays;
};

#if FIDES_USE_VISKORES

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

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

  // cuda doesn't like this class being private...
  class CalcPsi;

protected:
  void ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                       const fides::metadata::MetaData& selections) override;

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

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

protected:
  void ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
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

  /// Reads and returns cell set tokens via the OutputBuilder.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder) override;

protected:
  void ProcessViskores(std::vector<viskores::cont::DataSet>& partitions,
                       const fides::metadata::MetaData& selections) override;

private:
  viskores::Id GetMetaDataValue(const viskores::cont::DataSet& ds,
                                const std::string& fieldNm) const;
};

#endif // FIDES_USE_VISKORES

}
}
#endif
