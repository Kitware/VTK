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

#ifndef viskores_cont_CellLocatorBoundingIntervalHierarchy_h
#define viskores_cont_CellLocatorBoundingIntervalHierarchy_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/cont/CellLocatorBase.h>

#include <viskores/exec/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/exec/CellLocatorMultiplexer.h>

namespace viskores
{
namespace cont
{

/// @brief A cell locator that performs a recursive division of space.
///
/// `CellLocatorBoundingIntervalHierarchy` creates a search structure by recursively
/// dividing the space in which data lives.
/// It starts by choosing an axis to split and then defines a number of splitting planes
/// (set with `SetNumberOfSplittingPlanes()`).
/// These splitting planes divide the physical region into partitions, and the cells are
/// divided among these partitions.
/// The algorithm then recurses into each region and repeats the process until the regions
/// are divided to the point where the contain no more than a maximum number of cells
/// (specified with `SetMaxLeafSize()`).
class VISKORES_CONT_EXPORT CellLocatorBoundingIntervalHierarchy
  : public viskores::cont::CellLocatorBase
{
public:
  using SupportedCellSets = VISKORES_DEFAULT_CELL_SET_LIST;

  using CellLocatorExecList =
    viskores::ListTransform<SupportedCellSets,
                            viskores::exec::CellLocatorBoundingIntervalHierarchy>;

  using ExecObjType =
    viskores::ListApply<CellLocatorExecList, viskores::exec::CellLocatorMultiplexer>;
  using LastCell = typename ExecObjType::LastCell;

  /// Construct a `CellLocatorBoundingIntervalHierarchy` while optionally specifying the
  /// number of splitting planes and number of cells in each leaf.
  VISKORES_CONT
  CellLocatorBoundingIntervalHierarchy(viskores::IdComponent numPlanes = 4,
                                       viskores::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  /// @brief Specify the number of splitting planes to use each time a region is divided.
  ///
  /// Larger numbers of splitting planes result in a shallower tree (which is good because
  /// it means fewer memory lookups to find a cell), but too many splitting planes could lead
  /// to poorly shaped regions that inefficiently partition cells.
  ///
  /// The default value is 4.
  VISKORES_CONT void SetNumberOfSplittingPlanes(viskores::IdComponent numPlanes)
  {
    this->NumPlanes = numPlanes;
    this->SetModified();
  }
  /// @copydoc SetNumberOfSplittingPlanes
  VISKORES_CONT viskores::IdComponent GetNumberOfSplittingPlanes() { return this->NumPlanes; }

  /// @brief Specify the number of cells in each leaf.
  ///
  /// Larger numbers for the maximum leaf size result in a shallower tree (which is good
  /// because it means fewer memory lookups to find a cell), but it also means there will
  /// be more cells to check in each leaf (which is bad as checking a cell is slower
  /// than decending a tree level).
  ///
  /// The default value is 5.
  VISKORES_CONT void SetMaxLeafSize(viskores::IdComponent maxLeafSize)
  {
    this->MaxLeafSize = maxLeafSize;
    this->SetModified();
  }
  /// @copydoc SetMaxLeafSize
  VISKORES_CONT viskores::Id GetMaxLeafSize() { return this->MaxLeafSize; }

  VISKORES_CONT ExecObjType PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                viskores::cont::Token& token) const;

private:
  viskores::IdComponent NumPlanes;
  viskores::IdComponent MaxLeafSize;
  viskores::cont::ArrayHandle<viskores::exec::CellLocatorBoundingIntervalHierarchyNode> Nodes;
  viskores::cont::ArrayHandle<viskores::Id> ProcessedCellIds;

  VISKORES_CONT void Build() override;

  struct MakeExecObject;
};

} // namespace cont
} // namespace viskores

#endif // viskores_cont_CellLocatorBoundingIntervalHierarchy_h
