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
#ifndef viskores_worklet_RemoveUnusedPoints_h
#define viskores_worklet_RemoveUnusedPoints_h

#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{

/// A collection of worklets used to identify which points are used by at least
/// one cell and then remove the points that are not used by any cells. The
/// class containing these worklets can be used to manage running these
/// worklets, building new cell sets, and redefine field arrays.
///
class RemoveUnusedPoints
{
public:
  /// A worklet that creates a mask of used points (the first step in removing
  /// unused points). Given an array of point indices (taken from the
  /// connectivity of a CellSetExplicit) and an array mask initialized to 0,
  /// writes a 1 at the index of every point referenced by a cell.
  ///
  struct GeneratePointMask : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn pointIndices, WholeArrayInOut pointMask);
    using ExecutionSignature = void(_1, _2);

    template <typename PointMaskPortalType>
    VISKORES_EXEC void operator()(viskores::Id pointIndex,
                                  const PointMaskPortalType& pointMask) const
    {
      pointMask.Set(pointIndex, 1);
    }
  };

  /// A worklet that takes an array of point indices (taken from the
  /// connectivity of a CellSetExplicit) and an array that functions as a map
  /// from the original indices to new indices, creates a new array with the
  /// new mapped indices.
  ///
  struct TransformPointIndices : public viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn pointIndex, WholeArrayIn indexMap, FieldOut mappedPoints);
    using ExecutionSignature = _3(_1, _2);

    template <typename IndexMapPortalType>
    VISKORES_EXEC viskores::Id operator()(viskores::Id pointIndex,
                                          const IndexMapPortalType& indexPortal) const
    {
      return indexPortal.Get(pointIndex);
    }
  };

public:
  VISKORES_CONT
  RemoveUnusedPoints() = default;

  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VISKORES_CONT explicit RemoveUnusedPoints(
    const viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>&
      inCellSet)
  {
    this->FindPointsStart();
    this->FindPoints(inCellSet);
    this->FindPointsEnd();
  }

  /// Get this class ready for identifying the points used by cell sets.
  ///
  VISKORES_CONT void FindPointsStart() { this->MaskArray.ReleaseResources(); }

  /// Analyze the given cell set to find all points that are used. Unused
  /// points are those that are not found in any cell sets passed to this
  /// method.
  ///
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VISKORES_CONT void FindPoints(
    const viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>&
      inCellSet)
  {
    if (this->MaskArray.GetNumberOfValues() < 1)
    {
      // Initialize mask array to 0.
      this->MaskArray.AllocateAndFill(inCellSet.GetNumberOfPoints(), 0);
    }
    VISKORES_ASSERT(this->MaskArray.GetNumberOfValues() == inCellSet.GetNumberOfPoints());

    viskores::worklet::DispatcherMapField<GeneratePointMask> dispatcher;
    dispatcher.Invoke(inCellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                     viskores::TopologyElementTagPoint()),
                      this->MaskArray);
  }

  /// Compile the information collected from calls to \c FindPointsInCellSet to
  /// ready this class for mapping cell sets and fields.
  ///
  VISKORES_CONT void FindPointsEnd()
  {
    this->PointScatter.reset(new viskores::worklet::ScatterCounting(this->MaskArray, true));

    this->MaskArray.ReleaseResources();
  }

  /// \brief Map cell indices
  ///
  /// Given a cell set (typically the same one passed to the constructor)
  /// returns a new cell set with cell points transformed to use the indices of
  /// the new reduced point arrays.
  ///
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VISKORES_CONT viskores::cont::
    CellSetExplicit<ShapeStorage, VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG, OffsetsStorage>
    MapCellSet(
      const viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>&
        inCellSet) const
  {
    VISKORES_ASSERT(this->PointScatter);

    return MapCellSet(inCellSet,
                      this->PointScatter->GetInputToOutputMap(),
                      this->PointScatter->GetOutputToInputMap().GetNumberOfValues());
  }

  /// \brief Map cell indices
  ///
  /// Given a cell set (typically the same one passed to the constructor) and
  /// an array that maps point indices from an old set of indices to a new set,
  /// returns a new cell set with cell points transformed to use the indices of
  /// the new reduced point arrays.
  ///
  /// This helper method can be used by external items that do similar operations
  /// that remove points or otherwise rearange points in a cell set. If points
  /// were removed by calling \c FindPoints, then you should use the other form
  /// of \c MapCellSet.
  ///
  template <typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename MapStorage>
  VISKORES_CONT static viskores::cont::
    CellSetExplicit<ShapeStorage, VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG, OffsetsStorage>
    MapCellSet(
      const viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>&
        inCellSet,
      const viskores::cont::ArrayHandle<viskores::Id, MapStorage>& inputToOutputPointMap,
      viskores::Id numberOfPoints)
  {
    using VisitTopology = viskores::TopologyElementTagCell;
    using IncidentTopology = viskores::TopologyElementTagPoint;

    using NewConnectivityStorage = VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG;

    viskores::cont::ArrayHandle<viskores::Id, NewConnectivityStorage> newConnectivityArray;

    viskores::worklet::DispatcherMapField<TransformPointIndices> dispatcher;
    dispatcher.Invoke(inCellSet.GetConnectivityArray(VisitTopology(), IncidentTopology()),
                      inputToOutputPointMap,
                      newConnectivityArray);

    viskores::cont::CellSetExplicit<ShapeStorage, NewConnectivityStorage, OffsetsStorage>
      outCellSet;
    outCellSet.Fill(numberOfPoints,
                    inCellSet.GetShapesArray(VisitTopology(), IncidentTopology()),
                    newConnectivityArray,
                    inCellSet.GetOffsetsArray(VisitTopology(), IncidentTopology()));

    return outCellSet;
  }

  /// \brief Returns a permutation map that maps new points to old points.
  ///
  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id> GetPermutationArray() const
  {
    return this->PointScatter->GetOutputToInputMap();
  }

private:
  viskores::cont::ArrayHandle<viskores::IdComponent> MaskArray;

  /// Manages how the original point indices map to the new point indices.
  ///
  std::shared_ptr<viskores::worklet::ScatterCounting> PointScatter;
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_RemoveUnusedPoints_h
