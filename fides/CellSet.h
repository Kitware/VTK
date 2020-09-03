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

#include <fides/DataModel.h>
#include <fides/Value.h>
#include <fides/Array.h>
#include <fides/xgc/XGCCommon.h>

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/PartitionedDataSet.h>

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
  virtual std::vector<vtkm::cont::DynamicCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) = 0;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  virtual void PostRead(
    vtkm::cont::PartitionedDataSet&,
    const fides::metadata::MetaData&)
  {}

  virtual ~CellSetBase() {};
};

/// \brief Data model object for VTK-m cell sets.
///
/// \c fides::datamodel::CellSet is responsible of creating
/// a VTK-m cell set for each block. Note that this class
/// acts as a variant in that it will create a class of the
/// appropriate type to handle the specific cell set type
/// in use and delegate functionality to that class.
struct CellSet : public DataModelBase
{
  /// Overridden to handle CellSet specific items.
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<vtkm::cont::DynamicCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections);

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data.
  void PostRead(
    vtkm::cont::PartitionedDataSet& partitions,
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
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<vtkm::cont::DynamicCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, it is necessary to know the number
  /// of points to create the VTK-m cellset.
  virtual void PostRead(
    vtkm::cont::PartitionedDataSet& partitions,
    const fides::metadata::MetaData& selections) override;

protected:
  std::pair<unsigned char, int> CellInformation;
  std::vector<vtkm::cont::DynamicCellSet> CellSetCache;
  std::vector<vtkm::cont::VariantArrayHandle> ConnectivityArrays;
};

/// \brief Class to read unstructured grids of mixed cell types.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetExplicit objects.
struct CellSetExplicit : public CellSetBase
{
  /// Overridden to handle CellSetExplicit specific items.
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<vtkm::cont::DynamicCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

  /// This is called after all data is read from disk/buffers,
  /// enabling any work that needs to access array values and other
  /// dataset data. In this case, it is necessary to know the number
  /// of points to create the VTK-m cellset.
  virtual void PostRead(
    vtkm::cont::PartitionedDataSet& partitions,
    const fides::metadata::MetaData& selections) override;

protected:
  std::vector<vtkm::cont::DynamicCellSet> CellSetCache;
  std::unique_ptr<Array> CellTypes = nullptr;
  std::unique_ptr<Array> NumberOfVertices = nullptr;
  std::unique_ptr<Array> Connectivity = nullptr;
  std::vector<vtkm::cont::VariantArrayHandle> CellTypesArrays;
  std::vector<vtkm::cont::VariantArrayHandle> NumberOfVerticesArrays;
  std::vector<vtkm::cont::VariantArrayHandle> ConnectivityArrays;
};

/// \brief Class to read structured grids.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetStructured objects.
struct CellSetStructured : public CellSetBase
{
  /// Overridden to handle CellSetSingleType specific items.
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<vtkm::cont::DynamicCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

private:
  std::unique_ptr<Value> Dimensions = nullptr;
};


/// \brief Class to read XGC 2.5D cell set.
///
/// This class implements the \c CellSetBase API for reading
/// \c CellSetXGC objects.
struct CellSetXGC : public CellSetBase
{
  CellSetXGC() : CommonImpl(new XGCCommon()) {}

  /// Overridden to handle CellSetXGC specific items.
  void ProcessJSON(const rapidjson::Value& json,
                   DataSourcesType& sources) override;

  /// Reads and returns the cell sets.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<vtkm::cont::DynamicCellSet> Read(
    const std::unordered_map<std::string, std::string>& paths,
    DataSourcesType& sources,
    const fides::metadata::MetaData& selections) override;

private:
  std::vector<vtkm::cont::DynamicCellSet> CellSetCache;
  std::unique_ptr<Array> CellConnectivity = nullptr;
  std::unique_ptr<Array> PlaneConnectivity = nullptr;
  vtkm::Id NumberOfPlanes = -1;
  bool IsPeriodic = true;
  std::unique_ptr<XGCCommon> CommonImpl;

};

}
}

#endif
