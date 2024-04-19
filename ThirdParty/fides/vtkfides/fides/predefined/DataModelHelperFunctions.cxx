//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/FidesTypes.h>
#include <fides/predefined/DataModelHelperFunctions.h>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/document.h)
#include FIDES_RAPIDJSON(rapidjson/error/en.h)
#include FIDES_RAPIDJSON(rapidjson/filereadstream.h)
#include FIDES_RAPIDJSON(rapidjson/prettywriter.h)
#include FIDES_RAPIDJSON(rapidjson/stringbuffer.h)
// clang-format on

#include <iostream>

namespace fides
{
namespace predefined
{

namespace detail
{
template <typename ValueType>
void SetValueArray(std::shared_ptr<InternalMetadataSource> source,
                   rapidjson::Document::AllocatorType& allocator,
                   rapidjson::Value& parent,
                   const std::string& attrName,
                   const std::string& memberName)
{
  auto vec = source->GetAttribute<ValueType>(attrName);
  if (vec.empty())
  {
    throw std::runtime_error(memberName + " vector should not be empty. Check " + attrName +
                             " attribute.");
  }
  CreateValueArray(allocator, parent, memberName, vec);
}


} // end namespace detail

rapidjson::Value SetString(rapidjson::Document::AllocatorType& allocator, const std::string& str)
{
  rapidjson::Value s;
  s.SetString(str.c_str(), static_cast<rapidjson::SizeType>(str.length()), allocator);
  return s;
}

void CreateArrayBasic(rapidjson::Document::AllocatorType& allocator,
                      rapidjson::Value& parent,
                      const std::string& dataSource,
                      const std::string& variable,
                      bool isStatic /* = false*/,
                      const std::string& arrayType /* = "basic"*/,
                      const std::string& isVector /* = "" */)
{
  rapidjson::Value at = SetString(allocator, arrayType);
  parent.AddMember("array_type", at, allocator);

  rapidjson::Value ds = SetString(allocator, dataSource);
  parent.AddMember("data_source", ds, allocator);

  rapidjson::Value var = SetString(allocator, variable);
  parent.AddMember("variable", var, allocator);

  if (!isVector.empty())
  {
    rapidjson::Value vec = SetString(allocator, isVector);
    parent.AddMember("is_vector", vec, allocator);
  }

  if (isStatic)
  {
    parent.AddMember("static", isStatic, allocator);
  }
}

void CreateArrayCartesianProduct(rapidjson::Document::AllocatorType& allocator,
                                 rapidjson::Value& parent,
                                 std::shared_ptr<InternalMetadataSource> source,
                                 const std::string& dataSource)
{
  parent.AddMember("array_type", "cartesian_product", allocator);

  std::string xName = "x";
  auto vec = source->GetAttribute<std::string>("Fides_X_Variable");
  if (!vec.empty())
  {
    xName = vec[0];
  }
  rapidjson::Value xArr(rapidjson::kObjectType);
  CreateArrayBasic(allocator, xArr, dataSource, xName, false);
  parent.AddMember("x_array", xArr, allocator);

  std::string yName = "y";
  vec = source->GetAttribute<std::string>("Fides_Y_Variable");
  if (!vec.empty())
  {
    yName = vec[0];
  }
  rapidjson::Value yArr(rapidjson::kObjectType);
  CreateArrayBasic(allocator, yArr, dataSource, yName, false);
  parent.AddMember("y_array", yArr, allocator);

  std::string zName = "z";
  vec = source->GetAttribute<std::string>("Fides_Z_Variable");
  if (!vec.empty())
  {
    zName = vec[0];
  }
  rapidjson::Value zArr(rapidjson::kObjectType);
  CreateArrayBasic(allocator, zArr, dataSource, zName, false);
  parent.AddMember("z_array", zArr, allocator);
}

void CreateArrayXGCCoordinates(rapidjson::Document::AllocatorType& allocator,
                               rapidjson::Value& parent,
                               const std::string& dataSource,
                               const std::string& variable)
{
  parent.AddMember("array_type", "xgc_coordinates", allocator);

  rapidjson::Value ds = SetString(allocator, dataSource);
  parent.AddMember("data_source", ds, allocator);

  rapidjson::Value var = SetString(allocator, variable);
  parent.AddMember("variable", var, allocator);

  parent.AddMember("static", true, allocator);
  parent.AddMember("is_cylindrical", false, allocator);
}

void CreateArrayXGCField(rapidjson::Document::AllocatorType& allocator,
                         rapidjson::Value& parent,
                         const std::string& dataSource,
                         const std::string& variable)
{
  parent.AddMember("array_type", "xgc_field", allocator);

  rapidjson::Value ds = SetString(allocator, dataSource);
  parent.AddMember("data_source", ds, allocator);

  rapidjson::Value var = SetString(allocator, variable);
  parent.AddMember("variable", var, allocator);
}

void CreateValueVariableDimensions(rapidjson::Document::AllocatorType& allocator,
                                   rapidjson::Value& parent,
                                   const std::string& source,
                                   const std::string& dataSource,
                                   const std::string& variable)
{
  // ValueScalar and ValueVariableDimensions basically look the same in JSON
  CreateValueScalar(allocator, parent, "dimensions", source, dataSource, variable);
}

void CreateValueScalar(rapidjson::Document::AllocatorType& allocator,
                       rapidjson::Value& parent,
                       const std::string& memberName,
                       const std::string& source,
                       const std::string& dataSource,
                       const std::string& variable)
{
  rapidjson::Value obj(rapidjson::kObjectType);

  rapidjson::Value src = SetString(allocator, source);
  obj.AddMember("source", src, allocator);

  rapidjson::Value ds = SetString(allocator, dataSource);
  obj.AddMember("data_source", ds, allocator);

  rapidjson::Value var = SetString(allocator, variable);
  obj.AddMember("variable", var, allocator);
  auto name = SetString(allocator, memberName);
  parent.AddMember(name, obj, allocator);
}

void CreateValueArray(rapidjson::Document::AllocatorType& allocator,
                      rapidjson::Value& parent,
                      std::shared_ptr<InternalMetadataSource> source,
                      const std::string& attrName,
                      const std::string& memberName,
                      const std::string& dataSourceName)
{
  auto typeStr = source->GetAttributeType(attrName);
  if (typeStr.empty())
  {
    throw std::runtime_error(attrName + " could not be found.");
  }
  if (typeStr == GetType<std::string>())
  {
    auto varName = source->GetAttribute<std::string>(attrName);
    if (varName.size() != 1)
    {
      throw std::runtime_error(memberName + " should have a single value. Check " + attrName +
                               " attribute.");
    }
    CreateValueScalar(allocator, parent, memberName, "array_variable", dataSourceName, varName[0]);
  }
#define declare_type(T)                                                        \
  if (typeStr == GetType<T>())                                                 \
  {                                                                            \
    detail::SetValueArray<T>(source, allocator, parent, attrName, memberName); \
  }
  FIDES_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
}

void CreateValueArrayVariable(rapidjson::Document::AllocatorType& allocator,
                              rapidjson::Value& parent,
                              const std::string& variableName,
                              const std::string& dataSourceName,
                              const std::string& memberName)
{
  rapidjson::Value obj(rapidjson::kObjectType);
  obj.AddMember("source", "array_variable", allocator);
  obj.AddMember("data_source", SetString(allocator, dataSourceName), allocator);
  obj.AddMember("variable", SetString(allocator, variableName), allocator);
  parent.AddMember(SetString(allocator, memberName), obj, allocator);
}

rapidjson::Document CreateFieldArrayDoc(const std::string& variable,
                                        const std::string& source,
                                        const std::string& arrayType,
                                        const std::string& isVector)
{
  rapidjson::Document d;
  d.SetObject();
  rapidjson::Value arrObj(rapidjson::kObjectType);
  CreateArrayBasic(d.GetAllocator(), arrObj, source, variable, false, arrayType, isVector);
  d.AddMember("array", arrObj, d.GetAllocator());

  return d;
}

}
}
