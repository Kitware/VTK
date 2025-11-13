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
                                             viskores::Vec3f& pCoords) const
  {
    LastCell lastCell;
    viskores::Vec<viskores::Id, 1> cellIdsVec = { -1 };
    viskores::Vec<viskores::Vec3f, 1> pCoordsVec = { viskores::Vec3f() };
    viskores::IdComponent count = 0;
    auto status =
      this->FindCellImpl(IterateMode::FindOne, point, cellIdsVec, pCoordsVec, lastCell, count);

    cellId = cellIdsVec[0];
    pCoords = pCoordsVec[0];
    return status;
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindCell
  VISKORES_EXEC viskores::ErrorCode FindCell(const viskores::Vec3f& point,
                                             viskores::Id& cellId,
                                             viskores::Vec3f& pCoords,
                                             LastCell& lastCell) const
  {
    cellId = -1;

    //Check the last cell.
    if ((lastCell.CellId >= 0) && (lastCell.CellId < this->CellSet.GetNumberOfElements()))
    {
      if (this->PointInCell(point, lastCell.CellId, pCoords))
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
        viskores::IdComponent count = 0;
        viskores::Vec<viskores::Id, 1> cellIdsVec = { -1 };
        viskores::Vec<viskores::Vec3f, 1> pCoordsVec = { viskores::Vec3f() };
        if (this->FindInLeaf(IterateMode::FindOne, point, pCoordsVec, node, cellIdsVec, count))
        {
          lastCell.CellId = cellIdsVec[0];
          pCoords = pCoordsVec[0];
          return viskores::ErrorCode::Success;
        }
      }
    }

    //No fastpath. Do a full search.
    viskores::Vec<viskores::Id, 1> cellIdsVec = { -1 };
    viskores::Vec<viskores::Vec3f, 1> pCoordsVec = { viskores::Vec3f() };

    viskores::IdComponent count = 0;
    auto status =
      this->FindCellImpl(IterateMode::FindOne, point, cellIdsVec, pCoordsVec, lastCell, count);

    cellId = cellIdsVec[0];
    pCoords = pCoordsVec[0];
    return status;
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::CountAllCells
  VISKORES_EXEC viskores::IdComponent CountAllCells(const viskores::Vec3f& point) const
  {
    viskores::Vec<viskores::Id, 1> cellIdsVec = { -1 };
    viskores::Vec<viskores::Vec3f, 1> pCoordsVec = { viskores::Vec3f() };

    viskores::IdComponent count = 0;
    LastCell lastCell;

    auto status =
      this->FindCellImpl(IterateMode::CountAll, point, cellIdsVec, pCoordsVec, lastCell, count);
    if (status == viskores::ErrorCode::Success)
      return count;

    return 0;
  }

  /// @copydoc viskores::exec::CellLocatorUniformGrid::FindAllCells
  template <typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::ErrorCode FindAllCells(
    const viskores::Vec3f& viskoresNotUsed(point),
    CellIdsType& viskoresNotUsed(cellIdsVec),
    ParametricCoordsVecType& viskoresNotUsed(pCoordsVec)) const
  {
    //There is a memory access error on some GPU devices.
    // Disabling for now.
    return viskores::ErrorCode::Unsupported;

#if 0
    viskores::IdComponent n = cellIdsVec.GetNumberOfComponents();
    if (pCoordsVec.GetNumberOfComponents() != n)
      return viskores::ErrorCode::InvalidNumberOfIndices;

    if (n == 0)
      return viskores::ErrorCode::Success;

    for (viskores::IdComponent i = 0; i < n; i++)
      cellIdsVec[i] = -1;

    LastCell lastCell;
    viskores::IdComponent count = 0;
    auto status =
      this->FindCellImpl(IterateMode::FindAll, point, cellIdsVec, pCoordsVec, lastCell, count);

    // More than n cells were found.
    //If the size of cellIdsVec is not big enough to hold the number found, return an error.
    if (count > n)
      return viskores::ErrorCode::InvalidNumberOfIndices;

    return status;
#endif
  }

private:
  enum struct FindCellState
  {
    EnterNode,
    AscendFromNode,
    DescendLeftChild,
    DescendRightChild
  };

  enum struct IterateMode
  {
    FindOne,
    CountAll,
    FindAll
  };

  template <typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC viskores::ErrorCode FindCellImpl(const IterateMode& mode,
                                                 const viskores::Vec3f& point,
                                                 CellIdsType& cellIdsVec,
                                                 ParametricCoordsVecType& pCoordsVec,
                                                 LastCell& lastCell,
                                                 viskores::IdComponent& count) const
  {
    viskores::Id nodeIndex = 0;
    FindCellState state = FindCellState::EnterNode;
    VISKORES_ASSERT(cellIdsVec.GetNumberOfComponents() > 0);

    cellIdsVec[0] = -1;

    VISKORES_ASSERT(this->Nodes.GetNumberOfValues() > 0);
    while (true)
    {
      // 1) If we’ve found a cell (and only looking for one), stop immediately
      if (mode == IterateMode::FindOne && cellIdsVec[0] > viskores::Id(0))
        break;

      // 2) If we’ve returned all the way to the root and just ascended, stop
      if (nodeIndex == 0 && state == FindCellState::AscendFromNode)
        break;

      // 3) Otherwise, do exactly one step of the state machine
      switch (state)
      {
        case FindCellState::EnterNode:
          state = this->EnterNode(mode, point, cellIdsVec, nodeIndex, pCoordsVec, lastCell, count);
          break;

        case FindCellState::AscendFromNode:
          state = this->AscendFromNode(nodeIndex);
          break;

        case FindCellState::DescendLeftChild:
          state = this->DescendLeftChild(point, nodeIndex);
          break;

        case FindCellState::DescendRightChild:
          state = this->DescendRightChild(point, nodeIndex);
          break;
      }
    }

    if (count == 0)
      return viskores::ErrorCode::CellNotFound;

    return viskores::ErrorCode::Success;
  }

  template <typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC FindCellState EnterNode(const IterateMode& mode,
                                        const viskores::Vec3f& point,
                                        CellIdsType& cellIdsVec,
                                        viskores::Id nodeIndex,
                                        ParametricCoordsVecType& pCoordsVec,
                                        LastCell& lastCell,
                                        viskores::IdComponent& count) const
  {
    viskores::IdComponent n = cellIdsVec.GetNumberOfComponents();
    VISKORES_ASSERT(pCoordsVec.GetNumberOfComponents() == n);

    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node =
      this->Nodes.Get(nodeIndex);

    if (node.ChildIndex < 0)
    {
      // In a leaf node. Look for a containing cell.
      if (this->FindInLeaf(mode, point, pCoordsVec, node, cellIdsVec, count))
      {
        lastCell.NodeIdx = nodeIndex;
        lastCell.CellId = cellIdsVec[0];
      }

      return FindCellState::AscendFromNode;
    }
    else
    {
      return FindCellState::DescendLeftChild;
    }
  }

  VISKORES_EXEC
  FindCellState AscendFromNode(viskores::Id& nodeIndex) const
  {
    viskores::Id childNodeIndex = nodeIndex;
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& childNode =
      this->Nodes.Get(childNodeIndex);
    nodeIndex = childNode.ParentIndex;
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& parentNode =
      this->Nodes.Get(nodeIndex);

    if (parentNode.ChildIndex == childNodeIndex)
    {
      // Ascending from left child. Descend into the right child.
      return FindCellState::DescendRightChild;
    }
    else
    {
      VISKORES_ASSERT(parentNode.ChildIndex + 1 == childNodeIndex);
      // Ascending from right child. Ascend again. (Don't need to change state.)
      return FindCellState::AscendFromNode;
    }
  }

  VISKORES_EXEC
  FindCellState DescendLeftChild(const viskores::Vec3f& point, viskores::Id& nodeIndex) const
  {
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node =
      this->Nodes.Get(nodeIndex);
    VISKORES_ASSERT(node.Dimension >= 0 && node.Dimension < 3);

    const viskores::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate <= node.Node.LMax)
    {
      // Left child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex;
      return FindCellState::EnterNode;
    }
    else
    {
      // Left child does not contain the point. Skip to the right child.
      return FindCellState::DescendRightChild;
    }
  }

  VISKORES_EXEC
  FindCellState DescendRightChild(const viskores::Vec3f& point, viskores::Id& nodeIndex) const
  {
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node =
      this->Nodes.Get(nodeIndex);
    VISKORES_ASSERT(node.Dimension >= 0 && node.Dimension < 3);

    const viskores::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate >= node.Node.RMin)
    {
      // Right child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex + 1;
      return FindCellState::EnterNode;
    }
    else
    {
      // Right child does not contain the point. Skip to ascent
      return FindCellState::AscendFromNode;
    }
  }

  template <typename CellIdsType, typename ParametricCoordsVecType>
  VISKORES_EXEC bool FindInLeaf(
    const IterateMode& mode,
    const viskores::Vec3f& point,
    ParametricCoordsVecType& pCoordsVec,
    const viskores::exec::CellLocatorBoundingIntervalHierarchyNode& node,
    CellIdsType& cellIdsVec,
    viskores::IdComponent& count) const
  {
    auto n = cellIdsVec.GetNumberOfComponents();

    bool found = false;

    for (viskores::Id i = node.Leaf.Start; i < node.Leaf.Start + node.Leaf.Size; ++i)
    {
      viskores::Id cid = this->CellIds.Get(i);
      viskores::Vec3f pCoords;
      if (this->PointInCell(point, cid, pCoords))
      {
        found = true;
        if (mode == IterateMode::FindOne || mode == IterateMode::FindAll)
        {
          VISKORES_ASSERT(count < n);
          //Update vecs if there is room.  If the vecs are too small, it will get reported as an error in FindAllCells()
          if (count < n)
          {
            cellIdsVec[count] = cid;
            pCoordsVec[count] = pCoords;
          }
        }
        count++;

        if (mode == IterateMode::FindOne)
          break;
      }
    }
    return found;
  }

  VISKORES_EXEC bool PointInCell(const viskores::Vec3f& point,
                                 viskores::Id& cellId,
                                 viskores::Vec3f& pCoords) const
  {
    using IndicesType = typename CellSetPortal::IndicesType;
    IndicesType cellPointIndices = this->CellSet.GetIndices(cellId);
    viskores::VecFromPortalPermute<IndicesType, CoordsPortal> cellPoints(&cellPointIndices,
                                                                         this->Coords);
    auto cellShape = this->CellSet.GetCellShape(cellId);
    auto status = viskores::exec::WorldCoordinatesToParametricCoordinates(
      cellPoints, point, cellShape, pCoords);

    if (status != viskores::ErrorCode::Success)
      return false;

    if (!viskores::exec::CellInside(pCoords, cellShape))
      return false;

    return true;
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
