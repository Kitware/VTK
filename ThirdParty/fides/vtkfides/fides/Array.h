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

#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/UnknownArrayHandle.h>

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
  virtual std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) = 0;

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  /// Has to be implemented by subclasses.
  virtual size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                                   DataSourcesType& sources) = 0;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  virtual void PostRead(std::vector<vtkm::cont::DataSet>&, const fides::metadata::MetaData&) {}

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
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>&,
    DataSourcesType&,
    const fides::metadata::MetaData&) override;

  /// Throws error because this Array class is a placeholder for
  /// arrays belonging to wildcard fields that will eventually
  /// be expanded
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&) override;

private:
  std::string ArrayType;
};

/// \brief Data model object for VTK-m array handles.
///
/// \c fides::datamodel::Array is responsible of creating
/// VTK-m \c ArrayHandles by loading data defined by the Fides
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
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections);

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  /// Handled by the internal ArrayBase subclass.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources);

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
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources) override;

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
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(std::vector<vtkm::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources) override;

private:
  std::unique_ptr<Value> Dimensions = nullptr;
  std::unique_ptr<Value> Origin = nullptr;
  std::unique_ptr<Value> Spacing = nullptr;
  std::vector<vtkm::cont::UnknownArrayHandle> DimensionArrays;
  std::vector<vtkm::cont::UnknownArrayHandle> OriginArrays;
  std::vector<vtkm::cont::UnknownArrayHandle> SpacingArrays;
  bool DefinedFromVariableShape = true;
};

struct ArrayCartesianProduct : public ArrayBase
{
  /// Overridden to handle ArrayCartesianProduct specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles. This class depends on
  /// three separate (basic) array  objects that form the
  /// cartesian product.
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  /// Uses the number of blocks in the first (x) array.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources) override;

private:
  std::unique_ptr<Array> XArray = nullptr;
  std::unique_ptr<Array> YArray = nullptr;
  std::unique_ptr<Array> ZArray = nullptr;
};

/// \brief Class for handling XGC data and contains common functionality
/// for handling both XGC coordinates and fields.
struct ArrayXGC : public ArrayBase
{
  ArrayXGC();

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources) override;

protected:
  std::unique_ptr<XGCCommon> CommonImpl;
  vtkm::Id NumberOfPlanes = -1;
  bool EngineChecked = false;

  /// Ensures that the inline engine isn't being used since it's not
  /// supported for XGC
  void CheckEngineType(const std::unordered_map<std::string, std::string>& paths,
                       DataSourcesType& sources,
                       std::string& dataSourceName);

  /// Gets the shape of the variable
  std::vector<size_t> GetShape(const std::unordered_map<std::string, std::string>& paths,
                               DataSourcesType& sources);
};

/// \brief Class to read \c ArrayXGCCoordinates objects.
struct ArrayXGCCoordinates : public ArrayXGC
{
  /// Overridden to handle ArrayXGCCoordinates specific items.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Reads and returns array handles.
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
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
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Special handling for reading 3D variables. Use instead of the superclass's
  /// ReadSelf()
  vtkm::cont::UnknownArrayHandle Read3DVariable(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  void PostRead(std::vector<vtkm::cont::DataSet>& dataSets,
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
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  /// Uses the number of blocks in the first (x) array.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources) override;

  void PostRead(std::vector<vtkm::cont::DataSet>& dataSets,
                const fides::metadata::MetaData& metaData) override;

private:
  using GTCCoordsType32 = vtkm::cont::ArrayHandleSOA<vtkm::Vec3f_32>;
  using GTCCoordsType64 = vtkm::cont::ArrayHandleSOA<vtkm::Vec3f_64>;

  bool IsCached = false;
  vtkm::cont::UnknownArrayHandle CachedCoords;

  std::unique_ptr<ArrayBasic> XArray = nullptr;
  std::unique_ptr<ArrayBasic> YArray = nullptr;
  std::unique_ptr<ArrayBasic> ZArray = nullptr;

  class PlaneInserter;
};

/// \brief Class to read \c ArrayGTCField objects.
struct ArrayGTCField : public ArrayBase
{
  /// Reads and returns array handles.
  std::vector<vtkm::cont::UnknownArrayHandle> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// Returns the number of blocks in the underlying variable.
  /// Used by the reader to provide meta-data on blocks.
  /// For GTC, there are always only 1 block.
  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>&,
                           DataSourcesType&) override
  {
    return 1;
  }

  void PostRead(std::vector<vtkm::cont::DataSet>& dataSets,
                const fides::metadata::MetaData& metaData) override;

private:
  vtkm::Id NumberOfPlanes = -1;
  vtkm::Id NumberOfPointsPerPlane = -1;
  bool IsCached = false;
};

}
}

#endif
