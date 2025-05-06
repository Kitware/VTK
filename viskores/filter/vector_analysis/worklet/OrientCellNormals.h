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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_m_worklet_OrientCellNormals_h
#define viskores_m_worklet_OrientCellNormals_h

#include <viskores/Range.h>
#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleBitField.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Logging.h>

#include <viskores/worklet/MaskIndices.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/VecTraits.h>

namespace viskores
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientCellNormals
{
  static constexpr viskores::Id INVALID_ID = -1;

  // Returns true if v1 and v2 are pointing in the same hemisphere.
  template <typename T>
  VISKORES_EXEC static bool SameDirection(const viskores::Vec<T, 3>& v1,
                                          const viskores::Vec<T, 3>& v2)
  {
    return viskores::Dot(v1, v2) >= 0;
  }

  // Ensure that the normal is pointing in the same hemisphere as ref.
  // Returns true if normal is modified.
  template <typename T>
  VISKORES_EXEC static bool Align(viskores::Vec<T, 3>& normal, const viskores::Vec<T, 3>& ref)
  {
    if (!SameDirection(normal, ref))
    {
      normal = -normal;
      return true;
    }
    return false;
  }

public:
  // Locates starting points for BFS traversal of dataset by finding points
  // on the dataset boundaries. These points are marked as active.
  // Initializes the ActivePoints array.
  class WorkletMarkSourcePoints : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn coords, WholeArrayIn ranges, FieldOut activePoints);
    using ExecutionSignature = _3(_1 coord, _2 ranges);

    template <typename CoordT, typename RangePortal>
    VISKORES_EXEC bool operator()(const viskores::Vec<CoordT, 3>& point,
                                  const RangePortal& ranges) const
    {
      bool isActive = false;
      for (viskores::IdComponent dim = 0; dim < 3; ++dim)
      {
        const auto& range = ranges.Get(dim);
        const auto val = static_cast<decltype(range.Min)>(point[dim]);
        if (val <= range.Min || val >= range.Max)
        {
          isActive = true;
        }
      }
      return isActive;
    }
  };

  // For each of the source points, determine the boundaries it lies on. Align
  // each incident cell's normal to point out of the boundary, marking each cell
  // as both visited and active.
  // Clears the active flags for points, and marks the current point as visited.
  class WorkletProcessSourceCells : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  FieldInPoint coords,
                                  WholeArrayIn ranges,
                                  WholeArrayInOut cellNormals,
                                  // InOut for preserve data on masked indices
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints,
                                  FieldInOutPoint visitedPoints);
    using ExecutionSignature = void(CellIndices cellIds,
                                    _2 coords,
                                    _3 ranges,
                                    _4 cellNormals,
                                    _5 activeCells,
                                    _6 visitedCells,
                                    _7 activePoints,
                                    _8 visitedPoints);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename CellList,
              typename CoordComp,
              typename RangePortal,
              typename CellNormalPortal,
              typename ActiveCellsBitPortal,
              typename VisitedCellsBitPortal>
    VISKORES_EXEC void operator()(const CellList& cellIds,
                                  const viskores::Vec<CoordComp, 3>& coord,
                                  const RangePortal& ranges,
                                  CellNormalPortal& cellNormals,
                                  ActiveCellsBitPortal& activeCells,
                                  VisitedCellsBitPortal& visitedCells,
                                  bool& pointIsActive,
                                  bool& pointIsVisited) const
    {
      using NormalType = typename CellNormalPortal::ValueType;
      VISKORES_STATIC_ASSERT_MSG(viskores::VecTraits<NormalType>::NUM_COMPONENTS == 3,
                                 "Cell Normals expected to have 3 components.");
      using NormalCompType = typename NormalType::ComponentType;

      // Find the vector that points out of the dataset from the current point:
      const NormalType refNormal = [&]() -> NormalType
      {
        NormalType normal{ NormalCompType{ 0 } };
        NormalCompType numNormals{ 0 };
        for (viskores::IdComponent dim = 0; dim < 3; ++dim)
        {
          const auto range = ranges.Get(dim);
          const auto val = static_cast<decltype(range.Min)>(coord[dim]);
          if (val <= range.Min)
          {
            NormalType compNormal{ NormalCompType{ 0 } };
            compNormal[dim] = NormalCompType{ -1 };
            normal += compNormal;
            ++numNormals;
          }
          else if (val >= range.Max)
          {
            NormalType compNormal{ NormalCompType{ 0 } };
            compNormal[dim] = NormalCompType{ 1 };
            normal += compNormal;
            ++numNormals;
          }
        }

        VISKORES_ASSERT("Source point is not on a boundary?" && numNormals > 0.5);
        return normal / numNormals;
      }();

      // Align all cell normals to the reference, marking them as active and
      // visited.
      const viskores::IdComponent numCells = cellIds.GetNumberOfComponents();
      for (viskores::IdComponent c = 0; c < numCells; ++c)
      {
        const viskores::Id cellId = cellIds[c];

        if (!visitedCells.OrBitAtomic(cellId, true))
        { // This thread is the first to touch this cell.
          activeCells.SetBitAtomic(cellId, true);

          NormalType cellNormal = cellNormals.Get(cellId);
          if (Align(cellNormal, refNormal))
          {
            cellNormals.Set(cellId, cellNormal);
          }
        }
      }

      // Mark current point as inactive but visited:
      pointIsActive = false;
      pointIsVisited = true;
    }
  };

  // Mark each incident point as active and visited.
  // Marks the current cell as inactive.
  class WorkletMarkActivePoints : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cell,
                                  BitFieldInOut activePoints,
                                  BitFieldInOut visitedPoints,
                                  FieldInOutCell activeCells);
    using ExecutionSignature = _4(PointIndices pointIds, _2 activePoints, _3 visitedPoints);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename PointList, typename ActivePointsBitPortal, typename VisitedPointsBitPortal>
    VISKORES_EXEC bool operator()(const PointList& pointIds,
                                  ActivePointsBitPortal& activePoints,
                                  VisitedPointsBitPortal& visitedPoints) const
    {
      const viskores::IdComponent numPoints = pointIds.GetNumberOfComponents();
      for (viskores::IdComponent p = 0; p < numPoints; ++p)
      {
        const viskores::Id pointId = pointIds[p];
        if (!visitedPoints.OrBitAtomic(pointId, true))
        { // This thread owns this point.
          activePoints.SetBitAtomic(pointId, true);
        }
      }

      // Mark current cell as inactive:
      return false;
    }
  };

  // Mark each incident cell as active, setting a visited neighbor
  // cell as its reference for alignment.
  // Marks the current point as inactive.
  class WorkletMarkActiveCells : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayOut refCells,
                                  BitFieldInOut activeCells,
                                  BitFieldIn visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _5(CellIndices cellIds,
                                  _2 refCells,
                                  _3 activeCells,
                                  _4 visitedCells);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename CellList,
              typename RefCellPortal,
              typename ActiveCellBitPortal,
              typename VisitedCellBitPortal>
    VISKORES_EXEC bool operator()(const CellList& cellIds,
                                  RefCellPortal& refCells,
                                  ActiveCellBitPortal& activeCells,
                                  const VisitedCellBitPortal& visitedCells) const
    {
      // One of the cells must be marked visited already. Find it and use it as
      // an alignment reference for the others:
      const viskores::IdComponent numCells = cellIds.GetNumberOfComponents();
      viskores::Id refCellId = INVALID_ID;
      for (viskores::IdComponent c = 0; c < numCells; ++c)
      {
        const viskores::Id cellId = cellIds[c];
        if (visitedCells.GetBit(cellId))
        {
          refCellId = cellId;
          break;
        }
      }

      VISKORES_ASSERT("No reference cell found." && refCellId != INVALID_ID);

      for (viskores::IdComponent c = 0; c < numCells; ++c)
      {
        const viskores::Id cellId = cellIds[c];
        if (!visitedCells.GetBit(cellId))
        {
          if (!activeCells.OrBitAtomic(cellId, true))
          { // This thread owns this cell.
            refCells.Set(cellId, refCellId);
          }
        }
      }

      // Mark current point as inactive:
      return false;
    }
  };

  // Align the normal of each active cell, to its reference cell normal.
  // The cell is marked visited.
  class WorkletProcessCellNormals : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn refCells,
                                  WholeArrayInOut cellNormals,
                                  FieldInOut visitedCells);
    using ExecutionSignature = _3(InputIndex cellId, _1 refCellId, _2 cellNormals);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename CellNormalsPortal>
    VISKORES_EXEC bool operator()(const viskores::Id cellId,
                                  const viskores::Id refCellId,
                                  CellNormalsPortal& cellNormals) const
    {
      const auto refNormal = cellNormals.Get(refCellId);
      auto normal = cellNormals.Get(cellId);
      if (Align(normal, refNormal))
      {
        cellNormals.Set(cellId, normal);
      }

      // Mark cell as visited:
      return true;
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VISKORES_CONT static void Run(
    const CellSetType& cells,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<CellNormalCompType, 3>, CellNormalStorageType>&
      cellNormals)
  {
    using RangeType = viskores::cont::ArrayHandle<viskores::Range>;

    const viskores::Id numPoints = coords.GetNumberOfValues();
    const viskores::Id numCells = cells.GetNumberOfCells();

    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                       "OrientCellNormals worklet (%lld points, %lld cells)",
                       static_cast<viskores::Int64>(numPoints),
                       static_cast<viskores::Int64>(numCells));

    // active = cells / point to be used in the next worklet invocation mask.
    viskores::cont::BitField activePointBits; // Initialized by MarkSourcePoints
    auto activePoints = viskores::cont::make_ArrayHandleBitField(activePointBits);

    viskores::cont::BitField activeCellBits;
    activeCellBits.AllocateAndFill(numCells, false);
    auto activeCells = viskores::cont::make_ArrayHandleBitField(activeCellBits);

    // visited = cells / points that have been corrected.
    viskores::cont::BitField visitedPointBits;
    visitedPointBits.AllocateAndFill(numPoints, false);
    auto visitedPoints = viskores::cont::make_ArrayHandleBitField(visitedPointBits);

    viskores::cont::BitField visitedCellBits;
    visitedCellBits.AllocateAndFill(numCells, false);
    auto visitedCells = viskores::cont::make_ArrayHandleBitField(visitedCellBits);

    viskores::cont::Invoker invoke;
    viskores::cont::ArrayHandle<viskores::Id> mask; // Allocated as needed

    // For each cell, store a reference alignment cell.
    viskores::cont::ArrayHandle<viskores::Id> refCells;
    {
      viskores::cont::Algorithm::Copy(
        viskores::cont::make_ArrayHandleConstant<viskores::Id>(INVALID_ID, numCells), refCells);
    }

    // 1) Compute range of coords.
    const RangeType ranges = viskores::cont::ArrayRangeCompute(coords);

    // 2) Locate points on a boundary, since their normal alignment direction
    //    is known.
    invoke(WorkletMarkSourcePoints{}, coords, ranges, activePoints);

    // 3) For each source point, align the normals of the adjacent cells.
    {
      viskores::Id numActive =
        viskores::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
      (void)numActive;
      VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                     "ProcessSourceCells from " << numActive << " source points.");
      invoke(WorkletProcessSourceCells{},
             viskores::worklet::MaskIndices{ mask },
             cells,
             coords,
             ranges,
             cellNormals,
             activeCellBits,
             visitedCellBits,
             activePoints,
             visitedPoints);
    }

    for (size_t iter = 1;; ++iter)
    {
      // 4) Mark unvisited points adjacent to active cells
      {
        viskores::Id numActive =
          viskores::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "MarkActivePoints from " << numActive << " active cells.");
        invoke(WorkletMarkActivePoints{},
               viskores::worklet::MaskIndices{ mask },
               cells,
               activePointBits,
               visitedPointBits,
               activeCells);
      }

      // 5) Mark unvisited cells adjacent to active points
      {
        viskores::Id numActive =
          viskores::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
        (void)numActive;
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "MarkActiveCells from " << numActive << " active points.");
        invoke(WorkletMarkActiveCells{},
               viskores::worklet::MaskIndices{ mask },
               cells,
               refCells,
               activeCellBits,
               visitedCellBits,
               activePoints);
      }

      viskores::Id numActiveCells =
        viskores::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);

      if (numActiveCells == 0)
      { // Done!
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "Iteration " << iter << ": Traversal complete.");
        break;
      }

      VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                     "Iteration " << iter << ": Processing " << numActiveCells << " normals.");

      // 5) Correct normals for active cells.
      {
        invoke(WorkletProcessCellNormals{},
               viskores::worklet::MaskIndices{ mask },
               refCells,
               cellNormals,
               visitedCells);
      }
    }
  }
};
}
} // end namespace viskores::worklet


#endif // viskores_m_worklet_OrientCellNormals_h
