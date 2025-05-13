//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_Field_H_
#define fides_datamodel_Field_H_

#include <fides/Array.h>
#include <fides/DataModel.h>
#include <fides/FidesTypes.h>
#include <fides/FieldData.h>
#include <fides/predefined/InternalMetadataSource.h>

#include <viskores/cont/Field.h>

namespace fides
{
namespace datamodel
{

/// \brief Data model object for Viskores fields.
///
/// \c fides::datamodel::Field is responsible of creating
/// Viskores fields by loading data defined by the Fides
/// data model.
struct Field : public DataModelBase
{
  /// Overridden to handle the underlying Array as well as the
  /// association.
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Used when a wildcard field has been expanded. The new field object should use
  /// this function instead of ProcessJSON in order to be setup correctly and
  /// create the underlying Array.
  /// The json passed should contain the correct DOM for the underlying array, as
  /// that will be passed to Array::ProcessJSON
  void ProcessExpandedField(const std::string& name,
                            const std::string& assoc,
                            const rapidjson::Value& json,
                            DataSourcesType& sources);

  /// Reads and returns fields. The heavy-lifting is
  /// handled by the underlying Array object.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<viskores::cont::Field> Read(const std::unordered_map<std::string, std::string>& paths,
                                          DataSourcesType& sources,
                                          const fides::metadata::MetaData& selections);

  void PostRead(std::vector<viskores::cont::DataSet>& partitions,
                const fides::metadata::MetaData& selections);

  /// Similar to Read() but to be used when reading field data instead of regular
  /// fields. The heavy-lifting is
  /// handled by the underlying Array object.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  FIDES_DEPRECATED_SUPPRESS_BEGIN
  FIDES_DEPRECATED(1.1, "FieldData is no longer used. All data is stored in Viskores DataSet.")
  FieldData ReadFieldData(const std::unordered_map<std::string, std::string>& paths,
                          DataSourcesType& sources,
                          const fides::metadata::MetaData& selections);
  FIDES_DEPRECATED_SUPPRESS_END

  /// Returns true if this is a wildcard field
  bool IsWildcardField() { return this->WildcardField; }

  /// A struct to store info to be used in the expansion of a wildcard field
  struct WildcardFieldInfo
  {
    std::vector<std::string> Names;
    std::vector<std::string> Associations;
    std::vector<std::string> IsVector;
    std::vector<std::string> Sources;
    std::vector<std::string> ArrayTypes;
  };

  /// Reads attributes containing wildcard field info from the metadata source.
  /// The attributes contain info on the variable names, associations, data sources,
  /// and the underlying array type.
  WildcardFieldInfo GetWildcardFieldLists(
    std::shared_ptr<predefined::InternalMetadataSource> source);

  /// Name of the array.
  std::string Name;

  /// The association of the array, either POINTS, CELL_SET, or FIELD_DATA
  viskores::cont::Field::Association Association;

private:
  std::shared_ptr<fides::datamodel::Array> Array;
  std::string VariableAttributeName;
  std::string AssociationAttributeName;
  std::string VectorAttributeName;
  std::string SourcesAttributeName;
  std::string ArrayTypesAttributeName;
  bool WildcardField = false;
};

}
}

#endif
