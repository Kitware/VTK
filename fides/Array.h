//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_Array_H_
#define fides_datamodel_Array_H_

#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <fides/DataModel.h>
#include <fides/Value.h>
#include <fides/xgc/XGCCommon.h>

namespace fides
{
namespace datamodel
{

/// \brief Superclass for all specialized array implementations.
struct ArrayBase : public DataModelBase
{
  /// Reads and returns array handles. Has to be implemented
  /// by subclasses.
  virtual std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) = 0;

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  /// Has to be implemented by subclasses.
  virtual size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources,
                                   const std::string& groupName = "") = 0;

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  /// Has to be implemented by subclasses.
  virtual std::set<std::string> GetGroupNames(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources) = 0;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  virtual void PostRead(std::vector<viskores::cont::DataSet>&, const fides::metadata::MetaData&) {}

  virtual ~ArrayBase(){};
};

/// \brief A placeholder for setting up Wildcard Fields.
///
/// When a wildcard field is specified in a data model,
/// Fides first creates \c ArrayPlacholder object for its
/// data. When reading metadata, Fides is able to read ADIOS
/// files (or streams) and expand the wildcard fields.
/// At this point the actual Arrays to be used for the fields
/// can be created. ArrayPlaceholders should no longer exist
/// after metadata is read and thus Read and GetNumberOfBlocks
/// will throw errors if they are used.
struct ArrayPlaceholder : public ArrayBase
{
  /// Overridden to process the JSON for an array belonging
  /// to a wildcard field
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Throws error because this Array class is a placeholder for
  /// arrays belonging to wildcard fields that will eventually
  /// be expanded
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>&,
    DataSourcesType&,
    const fides::metadata::MetaData&) override;

  /// Throws error because this Array class is a placeholder for
  /// arrays belonging to wildcard fields that will eventually
  /// be expanded
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&,
                           const std::string& = "") override;

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

private:
  std::string ArrayType;
};

/// \brief Data model object for Viskores array handles.
///
/// \c fides::datamodel::Array is responsible of creating
/// Viskores \c ArrayHandles by loading data defined by the Fides
/// data model. This class delegates its responsibilities to
/// one of the specialized \c ArrayBase subclasses it creates
/// during JSON parsing.
struct Array : public DataModelBase
{
  /// Overridden to handle Array specific items.
  /// This will create an internal ArrayBase subclass
  /// depending on the array_type value.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Sets up an ArrayPlaceholder for a wildcard field.
  /// Should be used by wildcard fields instead of
  /// ProcessJSON.
  void CreatePlaceholder(const rapidjson::Value& json, DataSourcesType& sources);

  /// Reads and returns array handles. Handled by the
  /// internal ArrayBase subclass.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections);

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  /// Handled by the internal ArrayBase subclass.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "");

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&);

private:
  std::unique_ptr<ArrayBase> ArrayImpl = nullptr;
  // only used when this array belongs to a wildcard field
  std::unique_ptr<ArrayPlaceholder> Placeholder = nullptr;
};

/// \brief Class to read \c ArrayHandle objects.
///
/// \c ArrayBasic reads ArrayHandle objects with basic storage.
struct ArrayBasic : public ArrayBase
{
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles. The heavy-lifting is
  /// handled by the \c DataModelBase \c ReadSelf() method.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

private:
  fides::io::IsVector IsVector = fides::io::IsVector::Auto;
};

/// \brief Class to read \c ArrayHandleUniformPointCoordinates objects.
///
/// \c ArrayUniformPointCoordinates creates ArrayHandleUniformPointCoordinates
/// objects. It depends on a number of Value objects to obtains dimensions
/// (per block), origin and spacing. The origin of each block is computed based
/// on spacing and additional values provided by the Dimensions
/// Value object (start indices for each block).
struct ArrayUniformPointCoordinates : public ArrayBase
{
  /// Overridden to handle ArrayUniformPointCoordinates specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles. This class depends on
  /// a number of Value objects to obtains dimensions (per block),
  /// origin and spacing. The origin of each block is computed based
  /// on spacing and additional values provided by the Dimensions
  /// Value object (start indices for each block).
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

private:
  std::unique_ptr<Value> Dimensions = nullptr;
  std::unique_ptr<Value> Origin = nullptr;
  std::unique_ptr<Value> Spacing = nullptr;
  std::vector<viskores::cont::UnknownArrayHandle> DimensionArrays;
  std::vector<viskores::cont::UnknownArrayHandle> OriginArrays;
  std::vector<viskores::cont::UnknownArrayHandle> SpacingArrays;
  bool DefinedFromVariableShape = true;
};

struct ArrayCartesianProduct : public ArrayBase
{
  /// Overridden to handle ArrayCartesianProduct specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles. This class depends on
  /// three separate (basic) array  objects that form the
  /// cartesian product.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  /// Uses the number of blocks in the first (x) array.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

protected:
  std::unique_ptr<Array> XArray = nullptr;
  std::unique_ptr<Array> YArray = nullptr;
  std::unique_ptr<Array> ZArray = nullptr;
};

struct ArrayComposite : public ArrayCartesianProduct
{
  /// Reads and returns array handles. This class depends on
  /// three separate (basic) array  objects that form the
  /// cartesian product.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;
};

/// \brief Class for handling XGC data and contains common functionality
/// for handling both XGC coordinates and fields.
struct ArrayXGC : public ArrayBase
{
  ArrayXGC();

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override
  {
    /// No groups.
    return {};
  }

protected:
  std::unique_ptr<XGCCommon> CommonImpl;
  viskores::Id NumberOfPlanes = -1;
  bool EngineChecked = false;

  /// Ensures that the inline engine isn't being used since it's not
  /// supported for XGC
  void CheckEngineType(const std::unordered_map<std::string, std::string>& paths,
                       DataSourcesType& sources,
                       std::string& dataSourceName);

  /// Gets the shape of the variable
  std::vector<size_t> GetShape(const std::unordered_map<std::string, std::string>& paths,
                               DataSourcesType& sources,
                               const std::string& groupName = "");
};

/// \brief Class to read \c ArrayXGCCoordinates objects.
struct ArrayXGCCoordinates : public ArrayXGC
{
  /// Overridden to handle ArrayXGCCoordinates specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

private:
  bool IsCylindrical = true;
  struct AddToVectorFunctor;
};

/// \brief Class to read \c ArrayXGCField objects.
struct ArrayXGCField : public ArrayXGC
{
  /// Reads and returns array handles.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Special handling for reading 3D variables. Use instead of the superclass's
  /// ReadSelf()
  viskores::cont::UnknownArrayHandle Read3DVariable(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  void PostRead(std::vector<viskores::cont::DataSet>& dataSets,
                const fides::metadata::MetaData& metaData) override;

private:
  /// If Is2DField, then that means the variable is duplicated for each plane
  /// If false, then each plane has its own set of values.
  bool Is2DField = true;
  bool FieldDimsChecked = false;
};

/// \brief Class to read \c ArrayGTCCoordinates objects.
struct ArrayGTCCoordinates : public ArrayBase
{
  /// Overridden to handle ArrayXGCCoordinates specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  /// Uses the number of blocks in the first (x) array.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName = "") override;

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

  void PostRead(std::vector<viskores::cont::DataSet>& dataSets,
                const fides::metadata::MetaData& metaData) override;

  // cuda does not like this being private, so moved it to public...
  class PlaneInserter;

private:
  using GTCCoordsType32 = viskores::cont::ArrayHandleSOA<viskores::Vec3f_32>;
  using GTCCoordsType64 = viskores::cont::ArrayHandleSOA<viskores::Vec3f_64>;

  bool IsCached = false;
  viskores::cont::UnknownArrayHandle CachedCoords;

  std::unique_ptr<ArrayBasic> XArray = nullptr;
  std::unique_ptr<ArrayBasic> YArray = nullptr;
  std::unique_ptr<ArrayBasic> ZArray = nullptr;
};

/// \brief Class to read \c ArrayGTCField objects.
struct ArrayGTCField : public ArrayBase
{
  /// Reads and returns array handles.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable inside the given group
  /// Used by the reader to provide meta-data on blocks.
  /// For GTC, there are always only 1 block.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&,
                           const std::string& = "") override
  {
    return 1;
  }

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override;

  void PostRead(std::vector<viskores::cont::DataSet>& dataSets,
                const fides::metadata::MetaData& metaData) override;

private:
  viskores::Id NumberOfPlanes = -1;
  viskores::Id NumberOfPointsPerPlane = -1;
  bool IsCached = false;
};


/// \brief Class to read \c ArrayGXCoordinates objects.
struct ArrayGXCoordinates : public ArrayBase
{
  /// Overridden to handle ArrayXGCCoordinates specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles.
  std::vector<viskores::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable inside the given group.
  /// Used by the reader to provide meta-data on blocks.
  /// Uses the number of blocks in the first (x) array.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&,
                           const std::string&) override
  {
    return 1;
  }

  /// Returns the groups that have the underlying Array variable.
  /// Used by the reader to provide group names
  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>&,
                                      DataSourcesType&) override
  {
    return std::set<std::string>();
  }

  void PostRead(std::vector<viskores::cont::DataSet>& dataSets,
                const fides::metadata::MetaData& metaData) override;

  // cuda does not like this being private, so moved it to public...
  class PlaneInserter;

private:
  void ProcessJSONHelper(const rapidjson::Value& json,
                         DataSourcesType& sources,
                         const std::string& varName,
                         std::unique_ptr<ArrayBasic>& array);

  viskores::Id NumTheta = 10;
  viskores::Id NumZeta = 10;
  viskores::Id NFP = 1;
  bool FullTorus = true;
  bool ThetaZeroMid = false;
  bool ZetaZeroMid = false;
  bool SurfaceMaxIdxSet = false;
  bool SurfaceMinIdxSet = false;
  viskores::Id SurfaceMaxIdx = -1;
  viskores::Id SurfaceMinIdx = -1;

  std::unique_ptr<ArrayBasic> CoordPoints = nullptr;
  std::unique_ptr<ArrayBasic> RMNC = nullptr;
  std::unique_ptr<ArrayBasic> ZMNS = nullptr;
  std::unique_ptr<ArrayBasic> LMNS = nullptr;
  std::unique_ptr<ArrayBasic> XM = nullptr;
  std::unique_ptr<ArrayBasic> XN = nullptr;
  std::unique_ptr<ArrayBasic> nfp = nullptr;
  std::unique_ptr<ArrayBasic> phi = nullptr;

  viskores::cont::UnknownArrayHandle XMArrayHandle;
  viskores::cont::UnknownArrayHandle XNArrayHandle;
  viskores::cont::UnknownArrayHandle NFPArrayHandle;
  viskores::cont::UnknownArrayHandle RMNCArrayHandle;
  viskores::cont::UnknownArrayHandle ZMNSArrayHandle;
  viskores::cont::UnknownArrayHandle LMNSArrayHandle;
  viskores::cont::UnknownArrayHandle PhiArrayHandle;
};

}
}

#endif
