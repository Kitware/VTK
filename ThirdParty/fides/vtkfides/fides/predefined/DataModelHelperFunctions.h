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
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
// clang-format on

#include <string>

namespace fides
{
namespace predefined
{

/// Set a string in a rapidjson::Value object
rapidjson::Value SetString(rapidjson::Document::AllocatorType& allocator, const std::string& str);

/// Creates DOM for an ArrayBasic
void CreateArrayBasic(rapidjson::Document::AllocatorType& allocator,
                      rapidjson::Value& parent,
                      const std::string& dataSource,
                      const std::string& variable,
                      bool isStatic = false,
                      const std::string& arrayType = "basic",
                      const std::string& isVector = "");

/// Creates DOM for an ArrayCartesianProduct
void CreateArrayCartesianProduct(rapidjson::Document::AllocatorType& allocator,
                                 rapidjson::Value& parent,
                                 std::shared_ptr<InternalMetadataSource> source,
                                 const std::string& dataSource);

/// Creates DOM for an ArrayXGCCoordinates
void CreateArrayXGCCoordinates(rapidjson::Document::AllocatorType& allocator,
                               rapidjson::Value& parent,
                               const std::string& dataSource,
                               const std::string& variable);

/// Creates DOM for an ArrayXGCField
void CreateArrayXGCField(rapidjson::Document::AllocatorType& allocator,
                         rapidjson::Value& parent,
                         const std::string& dataSource,
                         const std::string& variable);

/// Creates DOM for an ValueVariableDimensions
void CreateValueVariableDimensions(rapidjson::Document::AllocatorType& allocator,
                                   rapidjson::Value& parent,
                                   const std::string& source,
                                   const std::string& dataSource,
                                   const std::string& variable);

/// Creates DOM for an ValueScalar
void CreateValueScalar(rapidjson::Document::AllocatorType& allocator,
                       rapidjson::Value& parent,
                       const std::string& memberName,
                       const std::string& source,
                       const std::string& dataSource,
                       const std::string& variable);

/// Creates DOM for an ValueArray
void CreateValueArray(rapidjson::Document::AllocatorType& allocator,
                      rapidjson::Value& parent,
                      std::shared_ptr<InternalMetadataSource> source,
                      const std::string& attrName,
                      const std::string& memberName,
                      const std::string& dataSourceName);


void CreateValueArrayVariable(rapidjson::Document::AllocatorType& allocator,
                              rapidjson::Value& parent,
                              const std::string& variableName,
                              const std::string& dataSourceName,
                              const std::string& memberName);

/// Creates DOM for an ValueArray when the vector is already known
template <typename ValueType>
inline void CreateValueArray(rapidjson::Document::AllocatorType& allocator,
                             rapidjson::Value& parent,
                             const std::string& memberName,
                             std::vector<ValueType> values)
{
  rapidjson::Value obj(rapidjson::kObjectType);
  obj.AddMember("source", "array", allocator);
  rapidjson::Value vals(rapidjson::kArrayType);
  for (size_t i = 0; i < values.size(); ++i)
  {
    vals.PushBack(values[i], allocator);
  }
  obj.AddMember("values", vals, allocator);
  auto name = SetString(allocator, memberName);
  parent.AddMember(name, obj, allocator);
}

/// Creates DOM for ArrayUniformPointCoordinates
template <typename OriginType, typename SpacingType>
inline void CreateArrayUniformPointCoordinates(rapidjson::Document::AllocatorType& allocator,
                                               rapidjson::Value& parent,
                                               const std::string& dimFieldName,
                                               const std::vector<OriginType>& origin,
                                               const std::vector<SpacingType>& spacing)
{
  rapidjson::Value coordObj(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);

  arrObj.AddMember("array_type", "uniform_point_coordinates", allocator);

  CreateValueVariableDimensions(allocator, arrObj, "variable_dimensions", "source", dimFieldName);
  CreateValueArray(allocator, arrObj, "origin", origin);
  CreateValueArray(allocator, arrObj, "spacing", spacing);

  coordObj.AddMember("array", arrObj, allocator);
  parent.AddMember("coordinate_system", coordObj, allocator);
}

inline void CreateArrayUniformPointCoordinates(rapidjson::Document::AllocatorType& allocator,
                                               rapidjson::Value& parent,
                                               const std::string& dimFieldName,
                                               const std::string& originFieldName,
                                               const std::string& spacingFieldName)
{
  rapidjson::Value coordObj(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);

  arrObj.AddMember("array_type", "uniform_point_coordinates", allocator);
  CreateValueArrayVariable(allocator, arrObj, dimFieldName, "source", "dimensions");
  CreateValueArrayVariable(allocator, arrObj, originFieldName, "source", "origin");
  CreateValueArrayVariable(allocator, arrObj, spacingFieldName, "source", "spacing");

  coordObj.AddMember("array", arrObj, allocator);
  parent.AddMember("coordinate_system", coordObj, allocator);
}

inline void CreateArrayRectilinearPointCoordinates(rapidjson::Document::AllocatorType& allocator,
                                                   rapidjson::Value& parent,
                                                   const std::string& xCoordsName,
                                                   const std::string& yCoordsName,
                                                   const std::string& zCoordsName)
{
  rapidjson::Value coordObj(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);

  arrObj.AddMember("array_type", "cartesian_product", allocator);

  rapidjson::Value xcObj(rapidjson::kObjectType);
  CreateArrayBasic(allocator, xcObj, "source", xCoordsName);
  arrObj.AddMember("x_array", xcObj, allocator);


  rapidjson::Value ycObj(rapidjson::kObjectType);
  CreateArrayBasic(allocator, ycObj, "source", yCoordsName);
  arrObj.AddMember("y_array", ycObj, allocator);

  rapidjson::Value zcObj(rapidjson::kObjectType);
  CreateArrayBasic(allocator, zcObj, "source", zCoordsName);
  arrObj.AddMember("z_array", zcObj, allocator);

  coordObj.AddMember("array", arrObj, allocator);
  parent.AddMember("coordinate_system", coordObj, allocator);
}

inline void CreateArrayUnstructuredPointCoordinates(rapidjson::Document::AllocatorType& allocator,
                                                    rapidjson::Value& parent,
                                                    const std::string& CoordsName)
{
  rapidjson::Value coordObj(rapidjson::kObjectType);
  rapidjson::Value arrObj(rapidjson::kObjectType);

  arrObj.AddMember("array_type", "basic", allocator);
  arrObj.AddMember("data_source", "source", allocator);
  arrObj.AddMember("variable", SetString(allocator, CoordsName), allocator);

  coordObj.AddMember("array", arrObj, allocator);
  parent.AddMember("coordinate_system", coordObj, allocator);
}

inline void CreateStructuredCellset(rapidjson::Document::AllocatorType& allocator,
                                    rapidjson::Value& parent,
                                    const std::string& dimFieldName)
{
  rapidjson::Value csObj(rapidjson::kObjectType);

  csObj.AddMember("cell_set_type", "structured", allocator);

  CreateValueScalar(allocator, csObj, "dimensions", "array_variable", "source", dimFieldName);
  parent.AddMember("cell_set", csObj, allocator);
}

inline void CreateUnstructuredSingleTypeCellset(rapidjson::Document::AllocatorType& allocator,
                                                rapidjson::Value& parent,
                                                const std::string& connectivityName,
                                                const std::string& cellType)
{
  rapidjson::Value cellSetObj(rapidjson::kObjectType);

  cellSetObj.AddMember("cell_set_type", "single_type", allocator);
  cellSetObj.AddMember("cell_type", SetString(allocator, cellType), allocator);
  cellSetObj.AddMember("data_source", "source", allocator);
  cellSetObj.AddMember("variable", SetString(allocator, connectivityName), allocator);

  parent.AddMember("cell_set", cellSetObj, allocator);
}

/// Creates DOM for the underlying array for a wildcard field
/// that is being expanded
rapidjson::Document CreateFieldArrayDoc(const std::string& variable,
                                        const std::string& source = "source",
                                        const std::string& arrayType = "basic",
                                        const std::string& isVector = "auto");
}
}

#endif
