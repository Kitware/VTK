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
#ifndef viskores_worklet_spatialstructure_BoundingIntervalHierarchy_h
#define viskores_worklet_spatialstructure_BoundingIntervalHierarchy_h

#include <type_traits>

#include <viskores/Bounds.h>
#include <viskores/Types.h>
#include <viskores/VecFromPortalPermute.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleReverse.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/exec/CellLocatorBoundingIntervalHierarchy.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{
namespace spatialstructure
{

struct TreeNode
{
  viskores::FloatDefault LMax;
  viskores::FloatDefault RMin;
  viskores::IdComponent Dimension;

  VISKORES_EXEC
  TreeNode()
    : LMax()
    , RMin()
    , Dimension()
  {
  }
}; // struct TreeNode

struct SplitProperties
{
  viskores::FloatDefault Plane;
  viskores::Id NumLeftPoints;
  viskores::Id NumRightPoints;
  viskores::FloatDefault LMax;
  viskores::FloatDefault RMin;
  viskores::FloatDefault Cost;

  VISKORES_EXEC
  SplitProperties()
    : Plane()
    , NumLeftPoints()
    , NumRightPoints()
    , LMax()
    , RMin()
    , Cost()
  {
  }
}; // struct SplitProperties

struct CellRangesExtracter : public viskores::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn,
                                WholeArrayIn,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell);
  typedef void ExecutionSignature(_1, PointIndices, _2, _3, _4, _5, _6, _7, _8);

  template <typename CellShape, typename PointIndicesVec, typename PointsPortal>
  VISKORES_EXEC void operator()(CellShape viskoresNotUsed(shape),
                                const PointIndicesVec& pointIndices,
                                const PointsPortal& points,
                                viskores::Range& rangeX,
                                viskores::Range& rangeY,
                                viskores::Range& rangeZ,
                                viskores::FloatDefault& centerX,
                                viskores::FloatDefault& centerY,
                                viskores::FloatDefault& centerZ) const
  {
    viskores::Bounds bounds;
    viskores::VecFromPortalPermute<PointIndicesVec, PointsPortal> cellPoints(&pointIndices, points);
    viskores::IdComponent numPoints = cellPoints.GetNumberOfComponents();
    for (viskores::IdComponent i = 0; i < numPoints; ++i)
    {
      bounds.Include(cellPoints[i]);
    }
    rangeX = bounds.X;
    rangeY = bounds.Y;
    rangeZ = bounds.Z;
    viskores::Vec3f center = bounds.Center();
    centerX = center[0];
    centerY = center[1];
    centerZ = center[2];
  }
}; // struct CellRangesExtracter

struct LEQWorklet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_EXEC
  void operator()(const viskores::FloatDefault& value,
                  const viskores::FloatDefault& planeValue,
                  viskores::Id& leq,
                  viskores::Id& r) const
  {
    leq = value <= planeValue;
    r = !leq;
  }
}; // struct LEQWorklet

template <bool LEQ>
struct FilterRanges;

template <>
struct FilterRanges<true> : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_EXEC
  void operator()(const viskores::FloatDefault& value,
                  const viskores::FloatDefault& planeValue,
                  const viskores::Range& cellBounds,
                  viskores::Range& outBounds) const
  {
    outBounds = (value <= planeValue) ? cellBounds : viskores::Range();
  }
}; // struct FilterRanges

template <>
struct FilterRanges<false> : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_EXEC
  void operator()(const viskores::FloatDefault& value,
                  const viskores::FloatDefault& planeValue,
                  const viskores::Range& cellBounds,
                  viskores::Range& outBounds) const
  {
    outBounds = (value > planeValue) ? cellBounds : viskores::Range();
  }
}; // struct FilterRanges

struct SplitPlaneCalculatorWorklet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  VISKORES_CONT
  SplitPlaneCalculatorWorklet(viskores::IdComponent planeIdx, viskores::IdComponent numPlanes)
    : Scale(static_cast<viskores::FloatDefault>(planeIdx + 1) /
            static_cast<viskores::FloatDefault>(numPlanes + 1))
  {
  }

  VISKORES_EXEC
  void operator()(const viskores::Range& range, viskores::FloatDefault& splitPlane) const
  {
    splitPlane = static_cast<viskores::FloatDefault>(range.Min + Scale * (range.Max - range.Min));
  }

  viskores::FloatDefault Scale;
};

struct SplitPropertiesCalculator : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, InputIndex);
  using InputDomain = _1;

  VISKORES_CONT
  SplitPropertiesCalculator(viskores::IdComponent index, viskores::Id stride)
    : Index(index)
    , Stride(stride)
  {
  }

  template <typename SplitPropertiesPortal>
  VISKORES_EXEC void operator()(const viskores::Id& pointsToLeft,
                                const viskores::Id& pointsToRight,
                                const viskores::Range& lMaxRanges,
                                const viskores::Range& rMinRanges,
                                const viskores::FloatDefault& planeValue,
                                SplitPropertiesPortal& splits,
                                viskores::Id inputIndex) const
  {
    SplitProperties split;
    split.Plane = planeValue;
    split.NumLeftPoints = pointsToLeft;
    split.NumRightPoints = pointsToRight;
    split.LMax = static_cast<viskores::FloatDefault>(lMaxRanges.Max);
    split.RMin = static_cast<viskores::FloatDefault>(rMinRanges.Min);
    if (lMaxRanges.IsNonEmpty() && rMinRanges.IsNonEmpty())
    {
      split.Cost = viskores::Abs(split.LMax * static_cast<viskores::FloatDefault>(pointsToLeft) -
                                 split.RMin * static_cast<viskores::FloatDefault>(pointsToRight));
    }
    else
    {
      split.Cost = viskores::Infinity<viskores::FloatDefault>();
    }
    splits.Set(inputIndex * Stride + Index, split);
    //printf("Plane = %lf, NL = %lld, NR = %lld, LM = %lf, RM = %lf, C = %lf\n", split.Plane, split.NumLeftPoints, split.NumRightPoints, split.LMax, split.RMin, split.Cost);
  }

  viskores::IdComponent Index;
  viskores::Id Stride;
};

struct SplitSelector : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  VISKORES_CONT
  SplitSelector(viskores::IdComponent numPlanes,
                viskores::IdComponent maxLeafSize,
                viskores::IdComponent stride)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Stride(stride)
  {
  }

  template <typename SplitPropertiesPortal>
  VISKORES_EXEC void operator()(viskores::Id index,
                                const SplitPropertiesPortal& xSplits,
                                const SplitPropertiesPortal& ySplits,
                                const SplitPropertiesPortal& zSplits,
                                const viskores::Id& segmentSize,
                                TreeNode& node,
                                viskores::FloatDefault& plane,
                                viskores::Id& choice) const
  {
    if (segmentSize <= MaxLeafSize)
    {
      node.Dimension = -1;
      choice = 0;
      return;
    }
    choice = 1;
    using Split = SplitProperties;
    viskores::FloatDefault minCost = viskores::Infinity<viskores::FloatDefault>();
    const Split& xSplit = xSplits.Get(ArgMin(xSplits, index * Stride, Stride));
    bool found = false;
    if (xSplit.Cost < minCost && xSplit.NumLeftPoints != 0 && xSplit.NumRightPoints != 0)
    {
      minCost = xSplit.Cost;
      node.Dimension = 0;
      node.LMax = xSplit.LMax;
      node.RMin = xSplit.RMin;
      plane = xSplit.Plane;
      found = true;
    }
    const Split& ySplit = ySplits.Get(ArgMin(ySplits, index * Stride, Stride));
    if (ySplit.Cost < minCost && ySplit.NumLeftPoints != 0 && ySplit.NumRightPoints != 0)
    {
      minCost = ySplit.Cost;
      node.Dimension = 1;
      node.LMax = ySplit.LMax;
      node.RMin = ySplit.RMin;
      plane = ySplit.Plane;
      found = true;
    }
    const Split& zSplit = zSplits.Get(ArgMin(zSplits, index * Stride, Stride));
    if (zSplit.Cost < minCost && zSplit.NumLeftPoints != 0 && zSplit.NumRightPoints != 0)
    {
      minCost = zSplit.Cost;
      node.Dimension = 2;
      node.LMax = zSplit.LMax;
      node.RMin = zSplit.RMin;
      plane = zSplit.Plane;
      found = true;
    }
    if (!found)
    {
      const Split& xMSplit = xSplits.Get(NumPlanes);
      minCost = xMSplit.Cost;
      node.Dimension = 0;
      node.LMax = xMSplit.LMax;
      node.RMin = xMSplit.RMin;
      plane = xMSplit.Plane;
      const Split& yMSplit = ySplits.Get(NumPlanes);
      if (yMSplit.Cost < minCost && yMSplit.NumLeftPoints != 0 && yMSplit.NumRightPoints != 0)
      {
        minCost = yMSplit.Cost;
        node.Dimension = 1;
        node.LMax = yMSplit.LMax;
        node.RMin = yMSplit.RMin;
        plane = yMSplit.Plane;
      }
      const Split& zMSplit = zSplits.Get(NumPlanes);
      if (zMSplit.Cost < minCost && zMSplit.NumLeftPoints != 0 && zMSplit.NumRightPoints != 0)
      {
        minCost = zMSplit.Cost;
        node.Dimension = 2;
        node.LMax = zMSplit.LMax;
        node.RMin = zMSplit.RMin;
        plane = zMSplit.Plane;
      }
    }
    //printf("Selected plane %lf, with cost %lf [%d, %lf, %lf]\n", plane, minCost, node.Dimension, node.LMax, node.RMin);
  }

  template <typename ArrayPortal>
  VISKORES_EXEC viskores::Id ArgMin(const ArrayPortal& values,
                                    viskores::Id start,
                                    viskores::Id length) const
  {
    viskores::Id minIdx = start;
    for (viskores::Id i = start; i < (start + length); ++i)
    {
      if (values.Get(i).Cost < values.Get(minIdx).Cost)
      {
        minIdx = i;
      }
    }
    return minIdx;
  }

  viskores::IdComponent NumPlanes;
  viskores::IdComponent MaxLeafSize;
  viskores::Id Stride;
};

struct CalculateSplitDirectionFlag : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VISKORES_EXEC
  void operator()(const viskores::FloatDefault& x,
                  const viskores::FloatDefault& y,
                  const viskores::FloatDefault& z,
                  const TreeNode& split,
                  const viskores::FloatDefault& plane,
                  viskores::Id& flag) const
  {
    if (split.Dimension >= 0)
    {
      const viskores::Vec3f point(x, y, z);
      const viskores::FloatDefault& c = point[split.Dimension];
      // We use 0 to signify left child, 1 for right child
      flag = 1 - static_cast<viskores::Id>(c <= plane);
    }
    else
    {
      flag = 0;
    }
  }
}; // struct CalculateSplitDirectionFlag

struct SegmentSplitter : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  VISKORES_CONT
  SegmentSplitter(viskores::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  VISKORES_EXEC
  void operator()(const viskores::Id& segmentId,
                  const viskores::Id& leqFlag,
                  const viskores::Id& segmentSize,
                  viskores::Id& newSegmentId) const
  {
    if (segmentSize <= MaxLeafSize)
    {
      // We do not split the segments which have cells fewer than MaxLeafSize, moving them to left
      newSegmentId = 2 * segmentId;
    }
    else
    {
      newSegmentId = 2 * segmentId + leqFlag;
    }
  }

  viskores::IdComponent MaxLeafSize;
}; // struct SegmentSplitter

struct SplitIndicesCalculator : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  VISKORES_EXEC
  void operator()(const viskores::Id& leqFlag,
                  const viskores::Id& trueFlagCount,
                  const viskores::Id& countPreviousSegment,
                  const viskores::Id& runningFalseFlagCount,
                  const viskores::Id& totalFalseFlagCount,
                  viskores::Id& scatterIndex) const
  {
    if (leqFlag)
    {
      scatterIndex = countPreviousSegment + totalFalseFlagCount + trueFlagCount;
    }
    else
    {
      scatterIndex = countPreviousSegment + runningFalseFlagCount - 1;
    }
  }
}; // struct SplitIndicesCalculator

struct Scatter : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldIn, WholeArrayOut);
  typedef void ExecutionSignature(_1, _2, _3);
  using InputDomain = _1;

  template <typename InputType, typename OutputPortalType>
  VISKORES_EXEC void operator()(const InputType& in,
                                const viskores::Id& idx,
                                OutputPortalType& out) const
  {
    out.Set(idx, in);
  }
}; // struct Scatter

template <typename ValueArrayHandle, typename IndexArrayHandle>
ValueArrayHandle ScatterArray(const ValueArrayHandle& input, const IndexArrayHandle& indices)
{
  ValueArrayHandle output;
  output.Allocate(input.GetNumberOfValues());
  viskores::worklet::DispatcherMapField<Scatter>().Invoke(input, indices, output);
  return output;
}

struct NonSplitIndexCalculator : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  VISKORES_CONT
  NonSplitIndexCalculator(viskores::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  VISKORES_EXEC void operator()(const viskores::Id& inSegmentSize,
                                viskores::Id& outSegmentSize) const
  {
    if (inSegmentSize <= MaxLeafSize)
    {
      outSegmentSize = inSegmentSize;
    }
    else
    {
      outSegmentSize = 0;
    }
  }

  viskores::Id MaxLeafSize;
}; // struct NonSplitIndexCalculator

struct TreeLevelAdder : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn nodeIndices,
                                FieldIn segmentSplits,
                                FieldIn nonSplitSegmentIndices,
                                FieldIn segmentSizes,
                                FieldIn runningSplitSegmentCounts,
                                FieldIn parentIndices,
                                WholeArrayInOut newTree,
                                WholeArrayOut nextParentIndices);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  VISKORES_CONT
  TreeLevelAdder(viskores::Id cellIdsOffset,
                 viskores::Id treeOffset,
                 viskores::IdComponent maxLeafSize)
    : CellIdsOffset(cellIdsOffset)
    , TreeOffset(treeOffset)
    , MaxLeafSize(maxLeafSize)
  {
  }

  template <typename BoundingIntervalHierarchyPortal, typename NextParentPortal>
  VISKORES_EXEC void operator()(viskores::Id index,
                                const TreeNode& split,
                                viskores::Id start,
                                viskores::Id count,
                                viskores::Id numPreviousSplits,
                                viskores::Id parentIndex,
                                BoundingIntervalHierarchyPortal& treePortal,
                                NextParentPortal& nextParentPortal) const
  {
    viskores::exec::CellLocatorBoundingIntervalHierarchyNode node;
    node.ParentIndex = parentIndex;
    if (count > this->MaxLeafSize)
    {
      node.Dimension = split.Dimension;
      node.ChildIndex = this->TreeOffset + 2 * numPreviousSplits;
      node.Node.LMax = split.LMax;
      node.Node.RMin = split.RMin;
      nextParentPortal.Set(2 * numPreviousSplits, index);
      nextParentPortal.Set(2 * numPreviousSplits + 1, index);
    }
    else
    {
      node.ChildIndex = -1;
      node.Leaf.Start = this->CellIdsOffset + start;
      node.Leaf.Size = count;
    }
    treePortal.Set(index, node);
  }

  viskores::Id CellIdsOffset;
  viskores::Id TreeOffset;
  viskores::IdComponent MaxLeafSize;
}; // struct TreeLevelAdder

template <typename T, class BinaryFunctor>
viskores::cont::ArrayHandle<T> ReverseScanInclusiveByKey(
  const viskores::cont::ArrayHandle<T>& keys,
  const viskores::cont::ArrayHandle<T>& values,
  BinaryFunctor binaryFunctor)
{
  viskores::cont::ArrayHandle<T> result;
  auto reversedResult = viskores::cont::make_ArrayHandleReverse(result);

  viskores::cont::Algorithm::ScanInclusiveByKey(viskores::cont::make_ArrayHandleReverse(keys),
                                                viskores::cont::make_ArrayHandleReverse(values),
                                                reversedResult,
                                                binaryFunctor);

  return result;
}

template <typename T, typename U>
viskores::cont::ArrayHandle<T> CopyIfArray(const viskores::cont::ArrayHandle<T>& input,
                                           const viskores::cont::ArrayHandle<U>& stencil)
{
  viskores::cont::ArrayHandle<T> result;
  viskores::cont::Algorithm::CopyIf(input, stencil, result);

  return result;
}

VISKORES_CONT
struct Invert
{
  VISKORES_EXEC
  viskores::Id operator()(const viskores::Id& value) const { return 1 - value; }
}; // struct Invert

VISKORES_CONT
struct RangeAdd
{
  VISKORES_EXEC
  viskores::Range operator()(const viskores::Range& accumulator, const viskores::Range& value) const
  {
    if (value.IsNonEmpty())
    {
      return accumulator.Union(value);
    }
    else
    {
      return accumulator;
    }
  }
}; // struct RangeAdd

} // namespace spatialstructure
} // namespace worklet
} // namespace viskores

#endif //viskores_worklet_spatialstructure_BoundingIntervalHierarchy_h
