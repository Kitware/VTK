//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataSetModel_H_
#define fides_datamodel_DataSetModel_H_

#include <fides/FidesTypes.h>
#include <fides/internal/DataObjectModel.h>

#include <map>
#include <memory>
#include <string>
#include <utility>

namespace fides
{
namespace predefined
{
class InternalMetadataSource;
}

namespace datamodel
{

struct CellSet;
struct CoordinateSystem;
struct Field;

/// \brief Top-level data model for traditional dataset-shaped data.
///
/// Holds the coordinate system, cell set, and fields parsed from a Fides
/// JSON schema and drives reading them through an \c OutputBuilder. Peer of
/// VTK's \c vtkDataSet within the \c DataObjectModel hierarchy.
struct DataSetModel : public DataObjectModel
{
  using FieldsKeyType = std::pair<std::string, fides::FieldAssociation>;

  // Member names intentionally don't match their type names: gcc 14's
  // -Wchanges-meaning rejects e.g. `std::shared_ptr<CoordinateSystem>
  // CoordinateSystem` because the member identifier shadows the type
  // identifier inside the same declaration.
  std::shared_ptr<CoordinateSystem> Coordinates = nullptr;
  std::shared_ptr<CellSet> Cells = nullptr;
  std::map<FieldsKeyType, std::shared_ptr<Field>> Fields;

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

private:
  /// Expands any wildcard fields using \c metadataSource. Updates \c Fields
  /// in place.
  void ExpandWildcardFields(
    std::shared_ptr<fides::predefined::InternalMetadataSource>& metadataSource,
    DataSourcesType& sources);

  void ProcessCoordinateSystem(const rapidjson::Value& coordSys, DataSourcesType& sources);
  void ProcessCellSet(const rapidjson::Value& cellSet, DataSourcesType& sources);
  void ProcessFields(const rapidjson::Value& fields, DataSourcesType& sources);
  std::shared_ptr<Field> ProcessField(const rapidjson::Value& fieldJson, DataSourcesType& sources);
};

}
}

#endif
