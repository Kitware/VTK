//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_CellGridModel_H_
#define fides_datamodel_CellGridModel_H_

#include <fides/internal/DataObjectModel.h>

#include <memory>
#include <string>
#include <vector>

namespace fides
{
namespace datamodel
{

struct CellGridAttribute;

/// \brief Top-level data model for vtkCellGrid (DG finite element) data.
///
/// Schema lists only attribute names; cell types and per-cell-type metadata
/// are discovered from attributes at read time. Output goes through
/// the VTK backend; the resulting partition is a \c vtkCellGrid placed in
/// the existing \c vtkPartitionedDataSet container. Peer of \c DataSetModel
/// in the \c DataObjectModel hierarchy.
struct CellGridModel : public DataObjectModel
{
  CellGridModel();
  ~CellGridModel() override;

  std::vector<std::unique_ptr<CellGridAttribute>> Attributes;

  /// Optional per-schema-entry filter restricting which cell types are built.
  /// This list comes from a "cell_types" string array in the cell_grid body.
  /// When non-empty, any "cell_types" in the source that aren't also in this
  /// array are omitted. If the "cell_types" string array is missing or empty
  /// from the schema, all cells of any type in the source are included.
  std::vector<std::string> CellTypeFilter;

  void ProcessJSON(const rapidjson::Value& root, DataSourcesType& sources) override;

  size_t GetNumberOfBlocks(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const std::string& groupName) override;

  std::set<std::string> GetGroupNames(const std::unordered_map<std::string, std::string>& paths,
                                      DataSourcesType& sources) override;

  std::vector<fides::metadata::FieldInformation> CollectFieldInformation(
    std::shared_ptr<fides::predefined::InternalMetadataSource>& metadataSource,
    DataSourcesType& sources) override;

  void Read(const std::unordered_map<std::string, std::string>& paths,
            DataSourcesType& sources,
            const fides::metadata::MetaData& selections,
            fides::OutputBuilder& builder) override;

  void PostRead(fides::DataContainer& container,
                const fides::metadata::MetaData& selections) override;

  bool RequiresVTK() const override { return true; }
};

}
}

#endif
