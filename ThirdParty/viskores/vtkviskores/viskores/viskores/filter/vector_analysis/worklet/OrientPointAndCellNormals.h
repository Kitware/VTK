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
#ifndef viskores_m_worklet_OrientPointAndCellNormals_h
#define viskores_m_worklet_OrientPointAndCellNormals_h

#include <viskores/Range.h>
#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleBitField.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/BitField.h>
#include <viskores/cont/Logging.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
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
class OrientPointAndCellNormals
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
  // on the dataset boundaries. The normals for these points are corrected by
  // making them point outside of the dataset, and they are marked as both
  // active and visited.
  class WorkletMarkSourcePoints : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn coords,
                                  FieldInOut pointNormals,
                                  WholeArrayIn ranges,
                                  FieldOut activePoints,
                                  FieldOut visitedPoints);
    using ExecutionSignature =
      void(_1 coord, _2 pointNormal, _3 ranges, _4 activePoints, _5 visitedPoints);

    template <typename CoordT, typename NormalT, typename RangePortal>
    VISKORES_EXEC void operator()(const viskores::Vec<CoordT, 3>& point,
                                  viskores::Vec<NormalT, 3>& pointNormal,
                                  const RangePortal& ranges,
                                  bool& isActive,
                                  bool& isVisited) const
    {
      for (viskores::IdComponent dim = 0; dim < 3; ++dim)
      {
        const auto& range = ranges.Get(dim);
        const auto val = static_cast<decltype(range.Min)>(point[dim]);
        if (val <= range.Min)
        {
          viskores::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(-1);
          Align(pointNormal, ref);
          isActive = true;
          isVisited = true;
          return;
        }
        else if (val >= range.Max)
        {
          viskores::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(1);
          Align(pointNormal, ref);
          isActive = true;
          isVisited = true;
          return;
        }
      }

      isActive = false;
      isVisited = false;
    }
  };

  // Mark each incident cell as active and visited.
  // Marks the current point as inactive.
  class WorkletMarkActiveCells : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cell,
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _4(CellIndices cellIds, _2 activeCells, _3 visitedCells);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename CellList, typename ActiveCellsBitPortal, typename VisitedCellsBitPortal>
    VISKORES_EXEC bool operator()(const CellList& cellIds,
                                  ActiveCellsBitPortal& activeCells,
                                  VisitedCellsBitPortal& visitedCells) const
    {
      const viskores::IdComponent numCells = cellIds.GetNumberOfComponents();
      for (viskores::IdComponent c = 0; c < numCells; ++c)
      {
        const viskores::Id cellId = cellIds[c];
        if (!visitedCells.OrBitAtomic(cellId, true))
        { // This thread owns this cell.
          activeCells.SetBitAtomic(cellId, true);
        }
      }

      // Mark current point as inactive:
      return false;
    }
  };

  // Align the current cell's normals to an adjacent visited point's normal.
  class WorkletProcessCellNormals : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayIn pointNormals,
                                  WholeArrayInOut cellNormals,
                                  BitFieldIn visitedPoints);
    using ExecutionSignature = void(PointIndices pointIds,
                                    InputIndex cellId,
                                    _2 pointNormals,
                                    _3 cellNormals,
                                    _4 visitedPoints);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename PointList,
              typename PointNormalsPortal,
              typename CellNormalsPortal,
              typename VisitedPointsBitPortal>
    VISKORES_EXEC void operator()(const PointList& pointIds,
                                  const viskores::Id cellId,
                                  const PointNormalsPortal& pointNormals,
                                  CellNormalsPortal& cellNormals,
                                  const VisitedPointsBitPortal& visitedPoints) const
    {
      // Use the normal of a visited point as a reference:
      const viskores::Id refPointId = [&]() -> viskores::Id
      {
        const viskores::IdComponent numPoints = pointIds.GetNumberOfComponents();
        for (viskores::IdComponent p = 0; p < numPoints; ++p)
        {
          const viskores::Id pointId = pointIds[p];
          if (visitedPoints.GetBit(pointId))
          {
            return pointId;
          }
        }

        return INVALID_ID;
      }();

      VISKORES_ASSERT("No reference point found." && refPointId != INVALID_ID);

      const auto refNormal = pointNormals.Get(refPointId);
      auto normal = cellNormals.Get(cellId);
      if (Align(normal, refNormal))
      {
        cellNormals.Set(cellId, normal);
      }
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
    using ExecutionSignature = _4(PointIndices pointIds, _2 activePoint, _3 visitedPoint);

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

  // Align the current point's normals to an adjacent visited cell's normal.
  class WorkletProcessPointNormals : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cells,
                                  WholeArrayInOut pointNormals,
                                  WholeArrayIn cellNormals,
                                  BitFieldIn visitedCells);
    using ExecutionSignature = void(CellIndices cellIds,
                                    InputIndex pointId,
                                    _2 pointNormals,
                                    _3 cellNormals,
                                    _4 visitedCells);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename CellList,
              typename CellNormalsPortal,
              typename PointNormalsPortal,
              typename VisitedCellsBitPortal>
    VISKORES_EXEC void operator()(const CellList& cellIds,
                                  const viskores::Id pointId,
                                  PointNormalsPortal& pointNormals,
                                  const CellNormalsPortal& cellNormals,
                                  const VisitedCellsBitPortal& visitedCells) const
    {
      // Use the normal of a visited cell as a reference:
      const viskores::Id refCellId = [&]() -> viskores::Id
      {
        const viskores::IdComponent numCells = cellIds.GetNumberOfComponents();
        for (viskores::IdComponent c = 0; c < numCells; ++c)
        {
          const viskores::Id cellId = cellIds[c];
          if (visitedCells.GetBit(cellId))
          {
            return cellId;
          }
        }

        return INVALID_ID;
      }();

      VISKORES_ASSERT("No reference cell found." && refCellId != INVALID_ID);

      const auto refNormal = cellNormals.Get(refCellId);
      auto normal = pointNormals.Get(pointId);
      if (Align(normal, refNormal))
      {
        pointNormals.Set(pointId, normal);
      }
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType,
            typename CellNormalCompType,
            typename CellNormalStorageType>
  VISKORES_CONT static void Run(
    const CellSetType& cells,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals,
    viskores::cont::ArrayHandle<viskores::Vec<CellNormalCompType, 3>, CellNormalStorageType>&
      cellNormals)
  {
    using RangeType = viskores::cont::ArrayHandle<viskores::Range>;

    using MarkSourcePoints = viskores::worklet::DispatcherMapField<WorkletMarkSourcePoints>;
    using MarkActiveCells = viskores::worklet::DispatcherMapTopology<WorkletMarkActiveCells>;
    using ProcessCellNormals = viskores::worklet::DispatcherMapTopology<WorkletProcessCellNormals>;
    using MarkActivePoints = viskores::worklet::DispatcherMapTopology<WorkletMarkActivePoints>;
    using ProcessPointNormals =
      viskores::worklet::DispatcherMapTopology<WorkletProcessPointNormals>;

    const viskores::Id numCells = cells.GetNumberOfCells();

    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                       "OrientPointAndCellNormals worklet (%lld points, %lld cells)",
                       static_cast<viskores::Int64>(coords.GetNumberOfValues()),
                       static_cast<viskores::Int64>(numCells));

    // active = cells / point to be used in the next worklet invocation mask.
    viskores::cont::BitField activePointBits; // Initialized by MarkSourcePoints
    auto activePoints = viskores::cont::make_ArrayHandleBitField(activePointBits);

    viskores::cont::BitField activeCellBits;
    activeCellBits.AllocateAndFill(numCells, false);
    auto activeCells = viskores::cont::make_ArrayHandleBitField(activeCellBits);

    // visited = cells / points that have been corrected.
    viskores::cont::BitField visitedPointBits; // Initialized by MarkSourcePoints
    auto visitedPoints = viskores::cont::make_ArrayHandleBitField(visitedPointBits);

    viskores::cont::BitField visitedCellBits;
    visitedCellBits.AllocateAndFill(numCells, false);
    auto visitedCells = viskores::cont::make_ArrayHandleBitField(visitedCellBits);

    viskores::cont::ArrayHandle<viskores::Id> mask; // Allocated as needed

    // 1) Compute range of coords.
    const RangeType ranges = viskores::cont::ArrayRangeCompute(coords);

    // 2) Locate points on a boundary and align their normal to point out of the
    //    dataset:
    {
      MarkSourcePoints dispatcher;
      dispatcher.Invoke(coords, pointNormals, ranges, activePoints, visitedPoints);
    }

    for (size_t iter = 1;; ++iter)
    {
      // 3) Mark unvisited cells adjacent to active points
      {
        viskores::Id numActive =
          viskores::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);
        (void)numActive;
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "MarkActiveCells from " << numActive << " active points.");
        MarkActiveCells dispatcher{ viskores::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activeCellBits, visitedCellBits, activePoints);
      }

      viskores::Id numActiveCells =
        viskores::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);

      if (numActiveCells == 0)
      { // Done!
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "Iteration " << iter << ": Traversal complete; no more cells");
        break;
      }

      VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                     "Iteration " << iter << ": Processing " << numActiveCells << " cell normals.");

      // 4) Correct normals for active cells.
      {
        ProcessCellNormals dispatcher{ viskores::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, pointNormals, cellNormals, visitedPointBits);
      }

      // 5) Mark unvisited points adjacent to active cells
      {
        viskores::Id numActive =
          viskores::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "MarkActivePoints from " << numActive << " active cells.");
        MarkActivePoints dispatcher{ viskores::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activePointBits, visitedPointBits, activeCells);
      }

      viskores::Id numActivePoints =
        viskores::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);

      if (numActivePoints == 0)
      { // Done!
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "Iteration " << iter << ": Traversal complete; no more points");
        break;
      }

      VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                     "Iteration " << iter << ": Processing " << numActivePoints
                                  << " point normals.");

      // 4) Correct normals for active points.
      {
        ProcessPointNormals dispatcher{ viskores::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, pointNormals, cellNormals, visitedCellBits);
      }
    }
  }
};
}
} // end namespace viskores::worklet


#endif // viskores_m_worklet_OrientPointAndCellNormals_h
