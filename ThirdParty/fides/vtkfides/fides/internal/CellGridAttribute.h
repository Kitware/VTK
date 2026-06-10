//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_datamodel_CellGridAttribute_H_
#define fides_datamodel_CellGridAttribute_H_

#include <fides/internal/DataModel.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace fides
{
namespace datamodel
{

/// \brief Per-component data model for one cell attribute on a vtkCellGrid.
///
/// One \c CellGridAttribute corresponds to one entry in the schema's
/// \c cell_attributes array. The schema only carries the attribute name and
/// the data source; the structural metadata (space, components, is_shape,
/// per-cell-type function space / basis / order / DOF sharing / array
/// references and their roles) is discovered at read time from the data
/// source.
struct CellGridAttribute : public DataModelBase
{
  void ProcessJSON(const rapidjson::Value& json, DataSourcesType& sources) override;

  /// Returns whether the given array role should be cached across steps.
  /// A per-role override (from \c "static_values" / \c "static_connectivity",
  /// or any future per-role key) wins; otherwise the \c "static" shorthand
  /// applies to every role.
  bool IsRoleStatic(const std::string& role) const
  {
    auto it = this->StaticRoleOverrides.find(role);
    return it != this->StaticRoleOverrides.end() ? it->second : this->AllRolesStatic;
  }

  /// Per-(role, cell type) cache populated on the first
  /// \c CellGridModel::Read call where the role is static, and reused on
  /// subsequent calls. Roles are cached independently so a schema can
  /// freeze the connectivity (structurally fixed by the function space)
  /// while letting the values change every step. Each vector is the
  /// deferred RawArray vector returned by \c DataSource::ReadVariable;
  /// the underlying buffers are kept alive by the \c shared_ptr<void>
  /// inside \c RawArray and survive data-source step transitions, which
  /// is what makes per-step caching safe.
  std::map<std::string, std::map<std::string, std::vector<fides::RawArray>>> CachedArraysByRole;

  /// Roles whose cache has been populated (and is therefore reusable).
  std::set<std::string> ValidCachedRoles;

  /// Schema-driven static policy. \c AllRolesStatic comes from the
  /// \c "static" shorthand; \c StaticRoleOverrides holds explicit
  /// per-role choices (\c "static_values" -> "values",
  /// \c "static_connectivity" -> "connectivity").
  bool AllRolesStatic = false;
  std::map<std::string, bool> StaticRoleOverrides;
};

}
}

#endif
