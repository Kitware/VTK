//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_filter_mesh_info_GhostCellClassify_h
#define viskores_filter_mesh_info_GhostCellClassify_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Determines which cells should be considered ghost cells in a structured data set.
///
/// The ghost cells are expected to be on the border. The outer layer of cells are marked
/// as ghost cells and the remainder marked as normal.
///
/// This filter generates a new cell-centered field marking the status of each cell.
/// Each entry is set to either `viskores::CellClassification::Normal` or
/// `viskores::CellClassification::Ghost`.
///
class VISKORES_FILTER_MESH_INFO_EXPORT GhostCellClassify : public viskores::filter::Filter
{
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inData) override;
  std::string GhostCellName;

public:
  VISKORES_CONT GhostCellClassify()
    : Filter()
    , GhostCellName(viskores::cont::GetGlobalGhostCellFieldName())
  {
  }

  /// @brief Set the name of the output field name.
  ///
  /// The output field is also marked as the ghost cell field in the output
  /// `viskores::cont::DataSet`.
  VISKORES_CONT void SetGhostCellName(const std::string& fieldName)
  {
    this->GhostCellName = fieldName;
  }
  /// @copydoc SetGhostCellName
  VISKORES_CONT const std::string& GetGhostCellName() { return this->GhostCellName; }
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_GhostCellClassify_h
