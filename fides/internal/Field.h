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

#include <fides/DataContainer.h>
#include <fides/FidesTypes.h>
#include <fides/internal/Array.h>
#include <fides/internal/DataModel.h>
#include <fides/internal/predefined/InternalMetadataSource.h>

#if FIDES_USE_VISKORES
#include <viskores/cont/Field.h>
#endif

namespace fides
{
namespace datamodel
{

/// \brief Data model object for fields.
///
/// \c fides::datamodel::Field is responsible for creating
/// fields by loading data defined by the Fides data model.
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

  /// Reads and returns array tokens via the OutputBuilder. The heavy-lifting is
  /// handled by the underlying Array object.
  /// The paths are passed to the \c DataSources to create
  /// file paths. \c selections restrict the data that is loaded.
  std::vector<size_t> Read(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           OutputBuilder& builder);

  /// Two-pass plan phase: forwards to the underlying Array.
  void CollectReadRequests(const std::unordered_map<std::string, std::string>& paths,
                           DataSourcesType& sources,
                           const fides::metadata::MetaData& selections,
                           std::vector<ReadRequest>& out);

  /// Two-pass emit phase: forwards to the underlying Array.
  std::vector<size_t> EmitTokens(const std::unordered_map<std::string, std::string>& paths,
                                 DataSourcesType& sources,
                                 const fides::metadata::MetaData& selections,
                                 const ReadResultMap& results,
                                 OutputBuilder& builder);

  void PostRead(fides::DataContainer& container, const fides::metadata::MetaData& selections);

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

  /// The association of the field.
  fides::FieldAssociation Association;

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
