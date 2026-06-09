//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/CellGridAttribute.h>

#include <stdexcept>

namespace fides
{
namespace datamodel
{

void CellGridAttribute::ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources)
{
  // Cell attributes only carry name + data_source in the schema. The structural
  // metadata (variable names, function space, basis, order, etc.) is discovered
  // from ADIOS2 attributes at read time, so we deliberately do not call the
  // base ProcessJSON which would require a "variable" field.
  if (!json.HasMember("name") || !json["name"].IsString())
  {
    throw std::runtime_error("cell_attribute requires a string 'name' member.");
  }
  this->ObjectName = json["name"].GetString();
  this->DataSourceName = this->FindDataSource(json, sources);

  // "static" is a shorthand that marks every role static. Per-role keys
  // override individual roles; explicit per-role wins over the shorthand
  // when both are present.
  if (json.HasMember("static") && json["static"].IsBool() && json["static"].GetBool())
  {
    this->AllRolesStatic = true;
  }
  // "static_roles" freezes an arbitrary set of roles by name (e.g.
  // ["connectivity", "ghost"]). Listed roles are marked static; the
  // named-key forms below can still override an individual entry (e.g.
  // to set it false), so they are parsed after this.
  if (json.HasMember("static_roles") && json["static_roles"].IsArray())
  {
    for (const auto& role : json["static_roles"].GetArray())
    {
      if (role.IsString())
      {
        this->StaticRoleOverrides[role.GetString()] = true;
      }
    }
  }
  if (json.HasMember("static_values") && json["static_values"].IsBool())
  {
    this->StaticRoleOverrides["values"] = json["static_values"].GetBool();
  }
  if (json.HasMember("static_connectivity") && json["static_connectivity"].IsBool())
  {
    this->StaticRoleOverrides["connectivity"] = json["static_connectivity"].GetBool();
  }
}

}
}
