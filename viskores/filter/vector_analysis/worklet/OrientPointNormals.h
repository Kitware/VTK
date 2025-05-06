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
#ifndef viskores_m_worklet_OrientPointNormals_h
#define viskores_m_worklet_OrientPointNormals_h

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

namespace viskores
{
namespace worklet
{

///
/// Orients normals to point outside of the dataset. This requires a closed
/// manifold surface or else the behavior is undefined. This requires an
/// unstructured cellset as input.
///
class OrientPointNormals
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
                                  FieldInOut normals,
                                  WholeArrayIn ranges,
                                  FieldOut activePoints,
                                  FieldOut visitedPoints,
                                  FieldOut refPoints);
    using ExecutionSignature =
      _6(InputIndex pointId, _1 coord, _2 normal, _3 ranges, _4 activePoints, _5 visitedPoints);

    template <typename CoordT, typename NormalT, typename RangePortal>
    VISKORES_EXEC viskores::Id operator()(const viskores::Id pointId,
                                          const viskores::Vec<CoordT, 3>& point,
                                          viskores::Vec<NormalT, 3>& normal,
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
          Align(normal, ref);
          isActive = true;
          isVisited = true;
          return pointId;
        }
        else if (val >= range.Max)
        {
          viskores::Vec<NormalT, 3> ref{ static_cast<NormalT>(0) };
          ref[dim] = static_cast<NormalT>(1);
          Align(normal, ref);
          isActive = true;
          isVisited = true;
          return pointId;
        }
      }

      isActive = false;
      isVisited = false;
      return INVALID_ID;
    }
  };

  // Traverses the active points (via mask) and marks the connected cells as
  // active. Set the reference point for all adjacent cells to the current
  // point.
  class WorkletMarkActiveCells : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellSet,
                                  // InOut to preserve data on masked indices
                                  BitFieldInOut activeCells,
                                  BitFieldInOut visitedCells,
                                  FieldInOutPoint activePoints);
    using ExecutionSignature = _4(CellIndices cells, _2 activeCells, _3 visitedCells);

    using MaskType = viskores::worklet::MaskIndices;

    // Mark all unvisited cells as active:
    template <typename CellListT, typename ActiveCellsT, typename VisitedCellsT>
    VISKORES_EXEC bool operator()(const CellListT& cells,
                                  ActiveCellsT& activeCells,
                                  VisitedCellsT& visitedCells) const
    {
      for (viskores::IdComponent c = 0; c < cells.GetNumberOfComponents(); ++c)
      {
        const viskores::Id cellId = cells[c];
        bool checkNotVisited = false;
        if (visitedCells.CompareExchangeBitAtomic(cellId, &checkNotVisited, true))
        { // This thread is first to visit cell
          activeCells.SetBitAtomic(cellId, true);
        }
      }

      // Mark the current point as inactive:
      return false;
    }
  };

  // Traverses the active cells and mark the connected points as active,
  // propogating the reference pointId.
  class WorkletMarkActivePoints : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellSet,
                                  BitFieldInOut activePoints,
                                  BitFieldIn visitedPoints,
                                  WholeArrayInOut refPoints,
                                  FieldInOutCell activeCells);
    using ExecutionSignature = _5(PointIndices points,
                                  _2 activePoints,
                                  _3 visitedPoints,
                                  _4 refPoints);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename PointListT,
              typename ActivePointsT,
              typename VisitedPointsT,
              typename RefPointsT>
    VISKORES_EXEC bool operator()(const PointListT& points,
                                  ActivePointsT& activePoints,
                                  VisitedPointsT& visitedPoints,
                                  RefPointsT& refPoints) const
    {
      // Find any point in the cell that has already been visited, and take
      // its id as the reference for this cell.
      viskores::Id refPtId = INVALID_ID;
      for (viskores::IdComponent p = 0; p < points.GetNumberOfComponents(); ++p)
      {
        const viskores::Id pointId = points[p];
        const bool alreadyVisited = visitedPoints.GetBit(pointId);
        if (alreadyVisited)
        {
          refPtId = pointId;
          break;
        }
      }

      // There must be one valid point in each cell:
      VISKORES_ASSERT("Reference point not found." && refPtId != INVALID_ID);

      // Propogate the reference point to other cell members
      for (viskores::IdComponent p = 0; p < points.GetNumberOfComponents(); ++p)
      {
        const viskores::Id pointId = points[p];

        // Mark this point as active
        const bool alreadyVisited = visitedPoints.GetBit(pointId);
        if (!alreadyVisited)
        {
          bool checkNotActive = false;
          if (activePoints.CompareExchangeBitAtomic(pointId, &checkNotActive, true))
          { // If we're the first thread to mark point active, set ref point:
            refPoints.Set(pointId, refPtId);
          }
        }
      }

      // Mark current cell as inactive:
      return false;
    }
  };

  // For each point with a refPtId set, ensure that the associated normal is
  // in the same hemisphere as the reference normal.
  // This must be done in a separate step from MarkActivePoints since modifying
  // visitedPoints in that worklet would create race conditions.
  class WorkletProcessNormals : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn refIds,
                                  WholeArrayInOut normals,
                                  // InOut to preserve data on masked indices
                                  BitFieldInOut visitedPoints);
    using ExecutionSignature = void(InputIndex ptId, _1 refPtId, _2 normals, _3 visitedPoints);

    using MaskType = viskores::worklet::MaskIndices;

    template <typename NormalsPortal, typename VisitedPointsT>
    VISKORES_EXEC void operator()(const viskores::Id ptId,
                                  const viskores::Id refPtId,
                                  NormalsPortal& normals,
                                  VisitedPointsT& visitedPoints) const
    {
      visitedPoints.SetBitAtomic(ptId, true);

      using Normal = typename NormalsPortal::ValueType;
      Normal normal = normals.Get(ptId);
      const Normal ref = normals.Get(refPtId);
      if (Align(normal, ref))
      {
        normals.Set(ptId, normal);
      }
    }
  };

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename PointNormalCompType,
            typename PointNormalStorageType>
  VISKORES_CONT static void Run(
    const CellSetType& cells,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsCompType, 3>, CoordsStorageType>& coords,
    viskores::cont::ArrayHandle<viskores::Vec<PointNormalCompType, 3>, PointNormalStorageType>&
      pointNormals)
  {
    using RangeType = viskores::cont::ArrayHandle<viskores::Range>;

    using MarkSourcePoints = viskores::worklet::DispatcherMapField<WorkletMarkSourcePoints>;
    using MarkActiveCells = viskores::worklet::DispatcherMapTopology<WorkletMarkActiveCells>;
    using MarkActivePoints = viskores::worklet::DispatcherMapTopology<WorkletMarkActivePoints>;
    using ProcessNormals = viskores::worklet::DispatcherMapField<WorkletProcessNormals>;

    const viskores::Id numCells = cells.GetNumberOfCells();

    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf,
                       "OrientPointNormals worklet (%lld points, %lld cells)",
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

    // For each point, store a reference alignment point. Allocated by
    // MarkSourcePoints.
    viskores::cont::ArrayHandle<viskores::Id> refPoints;

    // 1) Compute range of coords.
    const RangeType ranges = viskores::cont::ArrayRangeCompute(coords);

    // 2) Label source points for traversal (use those on a boundary).
    //    Correct the normals for these points by making them point towards the
    //    boundary.
    {
      MarkSourcePoints dispatcher;
      dispatcher.Invoke(coords, pointNormals, ranges, activePoints, visitedPoints, refPoints);
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

      // 4) Mark unvisited points in active cells, using ref point from cell.
      {
        viskores::Id numActive =
          viskores::cont::Algorithm::BitFieldToUnorderedSet(activeCellBits, mask);
        (void)numActive;
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "MarkActivePoints from " << numActive << " active cells.");
        MarkActivePoints dispatcher{ viskores::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(cells, activePointBits, visitedPointBits, refPoints, activeCells);
      }

      viskores::Id numActivePoints =
        viskores::cont::Algorithm::BitFieldToUnorderedSet(activePointBits, mask);

      if (numActivePoints == 0)
      { // Done!
        VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                       "Iteration " << iter << ": Traversal complete.");
        break;
      }

      VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                     "Iteration " << iter << ": Processing " << numActivePoints << " normals.");

      // 5) Correct normals for active points.
      {
        ProcessNormals dispatcher{ viskores::worklet::MaskIndices{ mask } };
        dispatcher.Invoke(refPoints, pointNormals, visitedPointBits);
      }
    }
  }
};
}
} // end namespace viskores::worklet


#endif // viskores_m_worklet_OrientPointNormals_h
