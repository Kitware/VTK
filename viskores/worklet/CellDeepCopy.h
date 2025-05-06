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
#ifndef viskores_worklet_CellDeepCopy_h
#define viskores_worklet_CellDeepCopy_h

#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

/// Container for worklets and helper methods to copy a cell set to a new
/// \c CellSetExplicit structure
///
struct CellDeepCopy
{
  struct CountCellPoints : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut numPointsInCell);
    using ExecutionSignature = _2(PointCount);

    VISKORES_EXEC
    viskores::IdComponent operator()(viskores::IdComponent numPoints) const { return numPoints; }
  };

  struct PassCellStructure : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut shapes, FieldOut pointIndices);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    VISKORES_EXEC void operator()(const CellShape& inShape,
                                  const InPointIndexType& inPoints,
                                  viskores::UInt8& outShape,
                                  OutPointIndexType& outPoints) const
    {
      (void)inShape; //C4100 false positive workaround
      outShape = inShape.Id;

      viskores::IdComponent numPoints = inPoints.GetNumberOfComponents();
      VISKORES_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  VISKORES_CONT static void Run(
    const InCellSetType& inCellSet,
    viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet,
    viskores::Id numberOfPoints)
  {
    VISKORES_IS_KNOWN_OR_UNKNOWN_CELL_SET(InCellSetType);

    viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;

    viskores::worklet::DispatcherMapTopology<CountCellPoints> countDispatcher;
    countDispatcher.Invoke(inCellSet, numIndices);

    viskores::cont::ArrayHandle<viskores::UInt8, ShapeStorage> shapes;
    viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorage> connectivity;

    viskores::cont::ArrayHandle<viskores::Id, OffsetsStorage> offsets;
    viskores::Id connectivitySize;
    viskores::cont::ConvertNumComponentsToOffsets(numIndices, offsets, connectivitySize);
    connectivity.Allocate(connectivitySize);

    viskores::worklet::DispatcherMapTopology<PassCellStructure> passDispatcher;
    passDispatcher.Invoke(
      inCellSet, shapes, viskores::cont::make_ArrayHandleGroupVecVariable(connectivity, offsets));

    viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage> newCellSet;
    newCellSet.Fill(numberOfPoints, shapes, connectivity, offsets);
    outCellSet = newCellSet;
  }

  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  VISKORES_CONT static void Run(
    const InCellSetType& inCellSet,
    viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    Run(inCellSet, outCellSet, inCellSet.GetNumberOfPoints());
  }

  template <typename InCellSetType>
  VISKORES_CONT static viskores::cont::CellSetExplicit<> Run(const InCellSetType& inCellSet)
  {
    VISKORES_IS_KNOWN_OR_UNKNOWN_CELL_SET(InCellSetType);

    viskores::cont::CellSetExplicit<> outCellSet;
    Run(inCellSet, outCellSet);

    return outCellSet;
  }
};
}
} // namespace viskores::worklet

#endif //viskores_worklet_CellDeepCopy_h
