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
