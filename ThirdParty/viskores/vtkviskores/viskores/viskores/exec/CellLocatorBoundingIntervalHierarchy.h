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
#ifndef viskores_exec_CellLocatorBoundingIntervalHierarchy_h
#define viskores_exec_CellLocatorBoundingIntervalHierarchy_h

#include <viskores/TopologyElementTag.h>
#include <viskores/VecFromPortalPermute.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/exec/CellInside.h>
#include <viskores/exec/ParametricCoordinates.h>

namespace viskores
{
namespace exec
{




struct CellLocatorBoundingIntervalHierarchyNode
{
#if defined(VISKORES_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif // gcc || clang
  viskores::IdComponent Dimension;
  viskores::Id ParentIndex;
  viskores::Id ChildIndex;
  union
  {
    struct
    {
      viskores::FloatDefault LMax;
      viskores::FloatDefault RMin;
    } Node;
    struct
    {
      viskores::Id Start;
      viskores::Id Size;
    } Leaf;
  };
#if defined(VISKORES_CLANG)
#pragma GCC diagnostic pop
#endif // gcc || clang

  VISKORES_EXEC_CONT
  CellLocatorBoundingIntervalHierarchyNode()
    : Dimension()
    , ParentIndex()
    , ChildIndex()
    , Node{ 0, 0 }
  {
  }
}; // struct CellLocatorBoundingIntervalHierarchyNode

/// @brief Structure for locating cells.
///
/// Use the `FindCell()` method to identify which cell contains a point in space.
/// The `FindCell()` method optionally takes a `LastCell` object, which is a
/// structure nested in this class. The `LastCell` object can help speed locating
/// cells for successive finds at nearby points.
///
/// This class is provided by `viskores::cont::CellLocatorBoundingIntervalHierarchy`
/// when passed to a worklet.
template <typename CellSetType>
class VISKORES_ALWAYS_EXPORT CellLocatorBoundingIntervalHierarchy
{
  using NodeArrayHandle =
    viskores::cont::ArrayHandle<viskores::exec::CellLocatorBoundingIntervalHierarchyNode>;
  using CellIdArrayHandle = viskores::cont::ArrayHandle<viskores::Id>;

public:
  VISKORES_CONT
  CellLocatorBoundingIntervalHierarchy(
    const NodeArrayHandle& nodes,
    const CellIdArrayHandle& cellIds,
    const CellSetType& cellSet,
    const viskores::cont::CoordinateSystem::MultiplexerArrayType& coords,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
    : Nodes(nodes.PrepareForInput(device, token))
    , CellIds(cellIds.PrepareForInput(device, token))
    , CellSet(cellSet.PrepareForInput(device, VisitType(), IncidentType(), token))
    , Coords(coords.PrepareForInput(device, token))
  {
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::LastCell
  struct LastCell
  {
    viskores::Id CellId = -1;
    viskores::Id NodeIdx = -1;
  };

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& parametric) const
  {
    LastCell lastCell;
    return this->FindCellImpl(point, cellId, parametric, lastCell);
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& parametric,
                                             LastCell& lastCell) const
  {
    cellId = -1;

    //Check the last cell.
    if ((lastCell.CellId >= 0) && (lastCell.CellId < this->CellSet.GetNumberOfElements()))
    {
      if (this->PointInCell(point, lastCell.CellId, parametric) == viskores::ErrorCode::Success)
      {
        cellId = lastCell.CellId;
        return viskores::ErrorCode::Success;
      }
    }

    //Check the last leaf node.
    if ((lastCell.NodeIdx >= 0) && (lastCell.NodeIdx < this->Nodes.GetNumberOfValues()))
    {
      const auto& node = this->Nodes.Get(lastCell.NodeIdx);

      if (node.ChildIndex < 0)
      {
        VISKORES_RETURN_ON_ERROR(this->FindInLeaf(point, parametric, node, cellId));
        if (cellId != -1)
        {
          lastCell.CellId = cellId;
          return viskores::ErrorCode::Success;
        }
      }
    }

    //No fastpath. Do a full search.
    return this->FindCellImpl(point, cellId, parametric, lastCell);
  }

  VISKORES_EXEC
  viskores::ErrorCode FindCellImpl(const viskores::Vec3f& point,
                                   viskores::Id& cellId,
                                   viskores::Vec3f& parametric,
                                   LastCell& lastCell) const
  {
    cellId = -1;
    viskores::Id nodeIndex = 0;
    FindCellState state = FindCellState::EnterNode;

    while ((cellId < 0) && !((nodeIndex == 0) && (state == FindCellState::AscendFromNode)))
    {
      switch (state)
      {
        case FindCellState::EnterNode:
          VISKORES_RETURN_ON_ERROR(
            this->EnterNode(state, point, cellId, nodeIndex, parametric, lastCell));
          break;
        case FindCellState::AscendFromNode:
          this->AscendFromNode(state, nodeIndex);
          break;
        case FindCellState::DescendLeftChild:
          this->DescendLeftChild(state, point, nodeIndex);
          break;
        case FindCellState::DescendRightChild:
          this->DescendRightChild(state, point, nodeIndex);
          break;
      }
    }

    if (cellId >= 0)
    {
      return viskores::ErrorCode::Success;
    }
    else
    {
      return viskores::ErrorCode::CellNotFound;
    }
  }

private:
  enum struct FindCellState
  {
    EnterNode,
    AscendFromNode,
    DescendLeftChild,
    DescendRightChild
  };

  VISKORES_EXEC
  viskores::ErrorCode EnterNode(FindCellState& state,
                                const viskores::Vec3f& point,
                                viskores::Id& cellId,
                                viskores::Id nodeIndex,
                                viskores::Vec3f& parametric,
                                LastCell& lastCell) const
  {
    VISKORES_ASSERT(state == FindCellState::EnterNode);

    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node =
      this->Nodes.Get(nodeIndex);

    if (node.ChildIndex < 0)
    {
      // In a leaf node. Look for a containing cell.
      VISKORES_RETURN_ON_ERROR(this->FindInLeaf(point, parametric, node, cellId));
      state = FindCellState::AscendFromNode;
      if (cellId != -1)
      {
        lastCell.CellId = cellId;
        lastCell.NodeIdx = nodeIndex;
      }
    }
    else
    {
      state = FindCellState::DescendLeftChild;
    }
    return viskores::ErrorCode::Success;
  }

  VISKORES_EXEC
  void AscendFromNode(FindCellState& state, viskores::Id& nodeIndex) const
  {
    VISKORES_ASSERT(state == FindCellState::AscendFromNode);

    viskores::Id childNodeIndex = nodeIndex;
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& childNode =
      this->Nodes.Get(childNodeIndex);
    nodeIndex = childNode.ParentIndex;
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& parentNode =
      this->Nodes.Get(nodeIndex);

    if (parentNode.ChildIndex == childNodeIndex)
    {
      // Ascending from left child. Descend into the right child.
      state = FindCellState::DescendRightChild;
    }
    else
    {
      VISKORES_ASSERT(parentNode.ChildIndex + 1 == childNodeIndex);
      // Ascending from right child. Ascend again. (Don't need to change state.)
    }
  }

  VISKORES_EXEC
  void DescendLeftChild(FindCellState& state,
                        const viskores::Vec3f& point,
                        viskores::Id& nodeIndex) const
  {
    VISKORES_ASSERT(state == FindCellState::DescendLeftChild);

    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node =
      this->Nodes.Get(nodeIndex);
    const viskores::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate <= node.Node.LMax)
    {
      // Left child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex;
      state = FindCellState::EnterNode;
    }
    else
    {
      // Left child does not contain the point. Skip to the right child.
      state = FindCellState::DescendRightChild;
    }
  }

  VISKORES_EXEC
  void DescendRightChild(FindCellState& state,
                         const viskores::Vec3f& point,
                         viskores::Id& nodeIndex) const
  {
    VISKORES_ASSERT(state == FindCellState::DescendRightChild);

    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node =
      this->Nodes.Get(nodeIndex);
    const viskores::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate >= node.Node.RMin)
    {
      // Right child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex + 1;
      state = FindCellState::EnterNode;
    }
    else
    {
      // Right child does not contain the point. Skip to ascent
      state = FindCellState::AscendFromNode;
    }
  }

  VISKORES_EXEC viskores::ErrorCode FindInLeaf(
    const viskores::Vec3f& point,
    viskores::Vec3f& parametric,
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node,
    viskores::Id& containingCellId) const
  {
    for (viskores::Id i = node.Leaf.Start; i < node.Leaf.Start + node.Leaf.Size; ++i)
    {
      viskores::Id cellId = this->CellIds.Get(i);

      if (this->PointInCell(point, cellId, parametric) == viskores::ErrorCode::Success)
      {
        containingCellId = cellId;
        return viskores::ErrorCode::Success;
      }
    }

    containingCellId = -1;
    return viskores::ErrorCode::Success;
  }

  //  template <typename CoordsType, typename CellShapeTag>
  VISKORES_EXEC viskores::ErrorCode PointInCell(const viskores::Vec3f& point,
                                                viskores::Id& cellId,
                                                viskores::Vec3f& parametric) const
  {
    using IndicesType = typename CellSetPortal::IndicesType;
    IndicesType cellPointIndices = this->CellSet.GetIndices(cellId);
    viskores::VecFromPortalPermute<IndicesType, CoordsPortal> cellPoints(&cellPointIndices,
                                                                         this->Coords);
    auto cellShape = this->CellSet.GetCellShape(cellId);
    bool isInside;
    VISKORES_RETURN_ON_ERROR(IsPointInCell(point, parametric, cellShape, cellPoints, isInside));

    if (isInside && viskores::exec::CellInside(parametric, cellShape))
      return viskores::ErrorCode::Success;

    return viskores::ErrorCode::CellNotFound;
  }

  template <typename CoordsType, typename CellShapeTag>
  VISKORES_EXEC static viskores::ErrorCode IsPointInCell(const viskores::Vec3f& point,
                                                         viskores::Vec3f& parametric,
                                                         CellShapeTag cellShape,
                                                         const CoordsType& cellPoints,
                                                         bool& isInside)
  {
    isInside = false;
    VISKORES_RETURN_ON_ERROR(viskores::exec::WorldCoordinatesToParametricCoordinates(
      cellPoints, point, cellShape, parametric));
    isInside = viskores::exec::CellInside(parametric, cellShape);
    return viskores::ErrorCode::Success;
  }

  using VisitType = viskores::TopologyElementTagCell;
  using IncidentType = viskores::TopologyElementTagPoint;
  using NodePortal = typename NodeArrayHandle::ReadPortalType;
  using CellIdPortal = typename CellIdArrayHandle::ReadPortalType;
  using CellSetPortal =
    typename CellSetType::template ExecConnectivityType<VisitType, IncidentType>;
  using CoordsPortal =
    typename viskores::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;

  NodePortal Nodes;
  CellIdPortal CellIds;
  CellSetPortal CellSet;
  CoordsPortal Coords;
}; // class CellLocatorBoundingIntervalHierarchy

} // namespace exec

} // namespace viskores

#endif //viskores_exec_CellLocatorBoundingIntervalHierarchy_h
