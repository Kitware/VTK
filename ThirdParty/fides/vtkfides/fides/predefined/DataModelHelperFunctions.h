//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_DataModelHelperFunctions_H
#define fides_datamodel_DataModelHelperFunctions_H

#include <fides/predefined/InternalMetadataSource.h>

#include <fides_rapidjson.h>
#include FIDES_RAPIDJSON(rapidjson/document.h)

#include <string>

namespace fides
{
namespace predefined
{

  /// Set a string in a rapidjson::Value object
  rapidjson::Value SetString(
    rapidjson::Document::AllocatorType& allocator,
    const std::string& str);

  /// Creates DOM for an ArrayBasic
  void CreateArrayBasic(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    const std::string& dataSource,
    const std::string& variable,
    bool isStatic = false,
    const std::string& arrayType = "basic");

  /// Creates DOM for an ArrayCartesianProduct
  void CreateArrayCartesianProduct(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    std::shared_ptr<InternalMetadataSource> source,
    const std::string& dataSource);

  /// Creates DOM for an ArrayXGCCoordinates
  void CreateArrayXGCCoordinates(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    const std::string& dataSource,
    const std::string& variable);

  /// Creates DOM for an ArrayXGCField
  void CreateArrayXGCField(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    const std::string& dataSource,
    const std::string& variable);

  /// Creates DOM for an ValueVariableDimensions
  void CreateValueVariableDimensions(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    const std::string& source,
    const std::string& dataSource,
    const std::string& variable);

  /// Creates DOM for an ValueScalar
  void CreateValueScalar(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    const std::string& memberName,
    const std::string& source,
    const std::string& dataSource,
    const std::string& variable);

  /// Creates DOM for an ValueArray
  void CreateValueArray(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    std::shared_ptr<InternalMetadataSource> source,
    const std::string& attrName,
    const std::string& memberName);

  /// Creates DOM for ArrayUniformPointCoordinates
  template <typename OriginType, typename SpacingType>
  void CreateArrayUniformPointCoordinates(
    rapidjson::Document::AllocatorType& allocator,
    rapidjson::Value& parent,
    std::vector<OriginType> origin,
    std::vector<SpacingType> spacing)
  {
    parent.AddMember("array_type", "uniform_point_coordinates", allocator);
    CreateValueVariableDimensions(allocator, parent, "variable_dimensions", "source", "density");
    CreateValueArray(allocator, parent, "origin", origin);
    CreateValueArray(allocator, parent, "spacing", spacing);
  }

  /// Creates DOM for the underlying array for a wildcard field
  /// that is being expanded
  rapidjson::Document CreateFieldArrayDoc(
    const std::string& variable,
    const std::string& source = "source",
    const std::string& arrayType = "basic");

}
}

#endif
