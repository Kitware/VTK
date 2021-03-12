//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_InternalMetadataSource_H
#define fides_datamodel_InternalMetadataSource_H

#include <fides/DataSource.h>
#include <fides/predefined/SupportedDataModels.h>

#include <memory>
#include <string>
#include <vector>

namespace fides
{
namespace predefined
{

/// \brief InternalMetadataSource is a DataSource that has attributes containing
/// info that can be used to generate a data model (instead of providing Fides
/// with a user-created data model).
class InternalMetadataSource
{
public:
  /// Constructor. filename is a path to the file containing the attribute
  /// information
  InternalMetadataSource(const std::string& filename);

  ~InternalMetadataSource();

  /// Get the name of the data model for Fides to generate
  std::string GetDataModelName(const std::string& attrName = "Fides_Data_Model");

  /// Same as GetDataModelName, except the string is converted to an DataModelTypes enum.
  DataModelTypes GetDataModelType(const std::string& attrName = "Fides_Data_Model");

  /// Gets the cell type. Not used by all data models
  std::string GetDataModelCellType(const std::string& attrName = "Fides_Cell_Type");

  /// Reads the attribute specified by attrName
  template <typename AttributeType>
  std::vector<AttributeType> GetAttribute(const std::string& attrName);

  /// Gets the type of the attribute specified by attrName
  std::string GetAttributeType(const std::string& attrName);

private:
  std::shared_ptr<fides::io::DataSource> Source = nullptr;
};

template <typename AttributeType>
std::vector<AttributeType> InternalMetadataSource::GetAttribute(const std::string& attrName)
{
  auto attr = this->Source->ReadAttribute<AttributeType>(attrName);
  return attr;
}

}
}

#endif
