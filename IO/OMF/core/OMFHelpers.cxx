// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "OMFHelpers.h"

#include "vtkLogger.h"

#include "vtk_jsoncpp.h"

namespace omf
{
namespace helper
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void PrintMemberNames(const Json::Value& root)
{
  vtkLogStartScope(INFO, "print-member-names");
  if (!root.isObject())
  {
    vtkLogEndScope("print-member-names");
    return;
  }
  auto members = root.getMemberNames();
  for (const auto& member : members)
  {
    vtkLog(INFO, << member);
  }
  vtkLogEndScope("print-member-names");
}

//------------------------------------------------------------------------------
bool GetPointFromJSON(const Json::Value& pointJSON, double point[3])
{
  if (pointJSON.isNull() || !pointJSON.isArray())
  {
    return false;
  }
  for (Json::Value::ArrayIndex i = 0; i < pointJSON.size(); ++i)
  {
    point[i] = pointJSON[i].asDouble();
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetIntValue(const Json::Value& root, int& value)
{
  if (root.empty() || !root.isInt())
  {
    return false;
  }
  value = root.asInt();
  return true;
}

//------------------------------------------------------------------------------
bool GetUIntValue(const Json::Value& root, unsigned int& value)
{
  if (root.empty() || !root.isUInt())
  {
    return false;
  }
  value = root.asUInt();
  return true;
}

//------------------------------------------------------------------------------
bool GetDoubleValue(const Json::Value& root, double& value)
{
  if (root.empty() || !root.isDouble())
  {
    return false;
  }
  value = root.asDouble();
  return true;
}

//------------------------------------------------------------------------------
bool GetStringValue(const Json::Value& root, std::string& value)
{
  if (root.empty() || !root.isString())
  {
    return false;
  }
  value = root.asString();
  return true;
}

//------------------------------------------------------------------------------
bool GetBoolValue(const Json::Value& root, bool& value)
{
  if (root.empty() || !root.isBool())
  {
    return false;
  }
  value = root.asBool();
  return true;
}

//------------------------------------------------------------------------------
bool GetIntArray(const Json::Value& root, std::vector<int>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& intValue : root)
  {
    if (intValue.empty() && !intValue.isInt())
    {
      value.clear();
      return false;
    }
    value.push_back(intValue.asInt());
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetUIntArray(const Json::Value& root, std::vector<unsigned int>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& uIntValue : root)
  {
    if (uIntValue.empty() && !uIntValue.isUInt())
    {
      value.clear();
      return false;
    }
    value.push_back(uIntValue.asUInt());
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetFloatArray(const Json::Value& root, std::vector<float>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& floatValue : root)
  {
    if (floatValue.empty() && !floatValue.isDouble())
    {
      value.clear();
      return false;
    }
    value.push_back(floatValue.asDouble());
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetDoubleArray(const Json::Value& root, std::vector<double>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& doubleValue : root)
  {
    if (doubleValue.empty() && !doubleValue.isDouble())
    {
      value.clear();
      return false;
    }
    value.push_back(doubleValue.asDouble());
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

VTK_ABI_NAMESPACE_END
} // end namespace helper
} // end namespace omf
