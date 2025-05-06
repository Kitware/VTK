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
#include <viskores/cont/CellLocatorBoundingIntervalHierarchy.h>

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
#include <viskores/cont/ErrorBadDevice.h>
#include <viskores/exec/CellLocatorBoundingIntervalHierarchy.h>

#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/worklet/spatialstructure/BoundingIntervalHierarchy.h>

namespace viskores
{
namespace cont
{

using IdArrayHandle = viskores::cont::ArrayHandle<viskores::Id>;
using IdPermutationArrayHandle =
  viskores::cont::ArrayHandlePermutation<IdArrayHandle, IdArrayHandle>;
using CoordsArrayHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
using CoordsPermutationArrayHandle =
  viskores::cont::ArrayHandlePermutation<IdArrayHandle, CoordsArrayHandle>;
using CountingIdArrayHandle = viskores::cont::ArrayHandleCounting<viskores::Id>;
using RangeArrayHandle = viskores::cont::ArrayHandle<viskores::Range>;
using RangePermutationArrayHandle =
  viskores::cont::ArrayHandlePermutation<IdArrayHandle, RangeArrayHandle>;
using SplitArrayHandle = viskores::cont::ArrayHandle<viskores::worklet::spatialstructure::TreeNode>;
using SplitPermutationArrayHandle =
  viskores::cont::ArrayHandlePermutation<IdArrayHandle, SplitArrayHandle>;
using SplitPropertiesArrayHandle =
  viskores::cont::ArrayHandle<viskores::worklet::spatialstructure::SplitProperties>;

namespace
{

IdArrayHandle CalculateSegmentSizes(const IdArrayHandle& segmentIds, viskores::Id numCells)
{
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  viskores::cont::Algorithm::ReduceByKey(
    segmentIds,
    viskores::cont::ArrayHandleConstant<viskores::Id>(1, numCells),
    discardKeys,
    segmentSizes,
    viskores::Add());
  return segmentSizes;
}

IdArrayHandle GenerateSegmentIds(const IdArrayHandle& segmentSizes, viskores::Id numCells)
{
  // Compact segment ids, removing non-contiguous values.

  // 1. Perform ScanInclusive to calculate the end positions of each segment
  IdArrayHandle segmentEnds;
  viskores::cont::Algorithm::ScanInclusive(segmentSizes, segmentEnds);
  // 2. Perform UpperBounds to perform the final compaction.
  IdArrayHandle segmentIds;
  viskores::cont::Algorithm::UpperBounds(
    segmentEnds, viskores::cont::ArrayHandleCounting<viskores::Id>(0, 1, numCells), segmentIds);
  return segmentIds;
}

void CalculatePlaneSplitCost(viskores::IdComponent planeIndex,
                             viskores::IdComponent numPlanes,
                             RangePermutationArrayHandle& segmentRanges,
                             RangeArrayHandle& ranges,
                             CoordsArrayHandle& coords,
                             IdArrayHandle& segmentIds,
                             SplitPropertiesArrayHandle& splits,
                             viskores::IdComponent index,
                             viskores::IdComponent numTotalPlanes)
{
  viskores::cont::Invoker invoker;

  // Make candidate split plane array
  viskores::cont::ArrayHandle<viskores::FloatDefault> splitPlanes;
  viskores::worklet::spatialstructure::SplitPlaneCalculatorWorklet splitPlaneCalcWorklet(planeIndex,
                                                                                         numPlanes);
  invoker(splitPlaneCalcWorklet, segmentRanges, splitPlanes);

  // Check if a point is to the left of the split plane or right
  viskores::cont::ArrayHandle<viskores::Id> isLEQOfSplitPlane, isROfSplitPlane;
  invoker(viskores::worklet::spatialstructure::LEQWorklet{},
          coords,
          splitPlanes,
          isLEQOfSplitPlane,
          isROfSplitPlane);

  // Count of points to the left
  viskores::cont::ArrayHandle<viskores::Id> pointsToLeft;
  IdArrayHandle discardKeys;
  viskores::cont::Algorithm::ReduceByKey(
    segmentIds, isLEQOfSplitPlane, discardKeys, pointsToLeft, viskores::Add());

  // Count of points to the right
  viskores::cont::ArrayHandle<viskores::Id> pointsToRight;
  viskores::cont::Algorithm::ReduceByKey(
    segmentIds, isROfSplitPlane, discardKeys, pointsToRight, viskores::Add());

  isLEQOfSplitPlane.ReleaseResourcesExecution();
  isROfSplitPlane.ReleaseResourcesExecution();

  // Calculate Lmax and Rmin
  viskores::cont::ArrayHandle<viskores::Range> lMaxRanges;
  {
    viskores::cont::ArrayHandle<viskores::Range> leqRanges;
    viskores::worklet::spatialstructure::FilterRanges<true> worklet;
    invoker(worklet, coords, splitPlanes, ranges, leqRanges);

    viskores::cont::Algorithm::ReduceByKey(segmentIds,
                                           leqRanges,
                                           discardKeys,
                                           lMaxRanges,
                                           viskores::worklet::spatialstructure::RangeAdd());
  }

  viskores::cont::ArrayHandle<viskores::Range> rMinRanges;
  {
    viskores::cont::ArrayHandle<viskores::Range> rRanges;
    viskores::worklet::spatialstructure::FilterRanges<false> worklet;
    invoker(worklet, coords, splitPlanes, ranges, rRanges);

    viskores::cont::Algorithm::ReduceByKey(segmentIds,
                                           rRanges,
                                           discardKeys,
                                           rMinRanges,
                                           viskores::worklet::spatialstructure::RangeAdd());
  }

  viskores::cont::ArrayHandle<viskores::FloatDefault> segmentedSplitPlanes;
  viskores::cont::Algorithm::ReduceByKey(
    segmentIds, splitPlanes, discardKeys, segmentedSplitPlanes, viskores::Minimum());

  // Calculate costs
  viskores::worklet::spatialstructure::SplitPropertiesCalculator splitPropertiesCalculator(
    index, numTotalPlanes + 1);
  invoker(splitPropertiesCalculator,
          pointsToLeft,
          pointsToRight,
          lMaxRanges,
          rMinRanges,
          segmentedSplitPlanes,
          splits);
}

void CalculateSplitCosts(viskores::IdComponent numPlanes,
                         RangePermutationArrayHandle& segmentRanges,
                         RangeArrayHandle& ranges,
                         CoordsArrayHandle& coords,
                         IdArrayHandle& segmentIds,
                         SplitPropertiesArrayHandle& splits)
{
  for (viskores::IdComponent planeIndex = 0; planeIndex < numPlanes; ++planeIndex)
  {
    CalculatePlaneSplitCost(planeIndex,
                            numPlanes,
                            segmentRanges,
                            ranges,
                            coords,
                            segmentIds,
                            splits,
                            planeIndex,
                            numPlanes);
  }
  // Calculate median costs
  CalculatePlaneSplitCost(
    0, 1, segmentRanges, ranges, coords, segmentIds, splits, numPlanes, numPlanes);
}

IdArrayHandle CalculateSplitScatterIndices(const IdArrayHandle& cellIds,
                                           const IdArrayHandle& leqFlags,
                                           const IdArrayHandle& segmentIds)
{
  viskores::cont::Invoker invoker;

  // Count total number of true flags preceding in segment
  IdArrayHandle trueFlagCounts;
  viskores::cont::Algorithm::ScanExclusiveByKey(segmentIds, leqFlags, trueFlagCounts);

  // Make a counting iterator.
  CountingIdArrayHandle counts(0, 1, cellIds.GetNumberOfValues());

  // Total number of elements in previous segment
  viskores::cont::ArrayHandle<viskores::Id> countPreviousSegments;
  viskores::cont::Algorithm::ScanInclusiveByKey(
    segmentIds, counts, countPreviousSegments, viskores::Minimum());

  // Total number of false flags so far in segment
  viskores::cont::ArrayHandleTransform<IdArrayHandle, viskores::worklet::spatialstructure::Invert>
    flagsInverse(leqFlags, viskores::worklet::spatialstructure::Invert());
  viskores::cont::ArrayHandle<viskores::Id> runningFalseFlagCount;
  viskores::cont::Algorithm::ScanInclusiveByKey(
    segmentIds, flagsInverse, runningFalseFlagCount, viskores::Add());

  // Total number of false flags in segment
  IdArrayHandle totalFalseFlagSegmentCount =
    viskores::worklet::spatialstructure::ReverseScanInclusiveByKey(
      segmentIds, runningFalseFlagCount, viskores::Maximum());

  // if point is to the left,
  //    index = total number in  previous segments + total number of false flags in this segment + total number of trues in previous segment
  // else
  //    index = total number in previous segments + number of falses preceding it in the segment.
  IdArrayHandle scatterIndices;
  invoker(viskores::worklet::spatialstructure::SplitIndicesCalculator{},
          leqFlags,
          trueFlagCounts,
          countPreviousSegments,
          runningFalseFlagCount,
          totalFalseFlagSegmentCount,
          scatterIndices);
  return scatterIndices;
}

} // anonymous namespace


void CellLocatorBoundingIntervalHierarchy::Build()
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "CellLocatorBoundingIntervalHierarchy::Build");

  viskores::cont::Invoker invoker;

  viskores::cont::UnknownCellSet cellSet = this->GetCellSet();
  viskores::Id numCells = cellSet.GetNumberOfCells();
  viskores::cont::CoordinateSystem coords = this->GetCoordinates();
  auto points = coords.GetDataAsMultiplexer();

  //std::cout << "No of cells: " << numCells << "\n";
  //std::cout.precision(3);
  //START_TIMER(s11);
  IdArrayHandle cellIds;
  viskores::cont::Algorithm::Copy(CountingIdArrayHandle(0, 1, numCells), cellIds);
  IdArrayHandle segmentIds;
  viskores::cont::Algorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, numCells),
                                  segmentIds);
  //PRINT_TIMER("1.1", s11);

  //START_TIMER(s12);
  CoordsArrayHandle centerXs, centerYs, centerZs;
  RangeArrayHandle xRanges, yRanges, zRanges;
  invoker(viskores::worklet::spatialstructure::CellRangesExtracter{},
          cellSet,
          points,
          xRanges,
          yRanges,
          zRanges,
          centerXs,
          centerYs,
          centerZs);
  //PRINT_TIMER("1.2", s12);

  bool done = false;
  //viskores::IdComponent iteration = 0;
  viskores::Id nodesIndexOffset = 0;
  viskores::Id numSegments = 1;
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  segmentSizes.Allocate(1);
  segmentSizes.WritePortal().Set(0, numCells);
  this->ProcessedCellIds.Allocate(numCells);
  viskores::Id cellIdsOffset = 0;

  IdArrayHandle parentIndices;
  parentIndices.Allocate(1);
  parentIndices.WritePortal().Set(0, -1);

  while (!done)
  {
    //std::cout << "**** Iteration " << (++iteration) << " ****\n";
    //Output(segmentSizes);
    //START_TIMER(s21);
    // Calculate the X, Y, Z bounding ranges for each segment
    RangeArrayHandle perSegmentXRanges, perSegmentYRanges, perSegmentZRanges;
    viskores::cont::Algorithm::ReduceByKey(
      segmentIds, xRanges, discardKeys, perSegmentXRanges, viskores::Add());
    viskores::cont::Algorithm::ReduceByKey(
      segmentIds, yRanges, discardKeys, perSegmentYRanges, viskores::Add());
    viskores::cont::Algorithm::ReduceByKey(
      segmentIds, zRanges, discardKeys, perSegmentZRanges, viskores::Add());
    //PRINT_TIMER("2.1", s21);

    // Expand the per segment bounding ranges, to per cell;
    RangePermutationArrayHandle segmentXRanges(segmentIds, perSegmentXRanges);
    RangePermutationArrayHandle segmentYRanges(segmentIds, perSegmentYRanges);
    RangePermutationArrayHandle segmentZRanges(segmentIds, perSegmentZRanges);

    //START_TIMER(s22);
    // Calculate split costs for NumPlanes split planes, across X, Y and Z dimensions
    viskores::Id numSplitPlanes = numSegments * (this->NumPlanes + 1);
    viskores::cont::ArrayHandle<viskores::worklet::spatialstructure::SplitProperties> xSplits,
      ySplits, zSplits;
    xSplits.Allocate(numSplitPlanes);
    ySplits.Allocate(numSplitPlanes);
    zSplits.Allocate(numSplitPlanes);
    CalculateSplitCosts(this->NumPlanes, segmentXRanges, xRanges, centerXs, segmentIds, xSplits);
    CalculateSplitCosts(this->NumPlanes, segmentYRanges, yRanges, centerYs, segmentIds, ySplits);
    CalculateSplitCosts(this->NumPlanes, segmentZRanges, zRanges, centerZs, segmentIds, zSplits);
    //PRINT_TIMER("2.2", s22);

    segmentXRanges.ReleaseResourcesExecution();
    segmentYRanges.ReleaseResourcesExecution();
    segmentZRanges.ReleaseResourcesExecution();

    //START_TIMER(s23);
    // Select best split plane and dimension across X, Y, Z dimension, per segment
    SplitArrayHandle segmentSplits;
    viskores::cont::ArrayHandle<viskores::FloatDefault> segmentPlanes;
    viskores::cont::ArrayHandle<viskores::Id> splitChoices;
    CountingIdArrayHandle indices(0, 1, numSegments);

    viskores::worklet::spatialstructure::SplitSelector worklet(
      this->NumPlanes, this->MaxLeafSize, this->NumPlanes + 1);
    invoker(worklet,
            indices,
            xSplits,
            ySplits,
            zSplits,
            segmentSizes,
            segmentSplits,
            segmentPlanes,
            splitChoices);
    //PRINT_TIMER("2.3", s23);

    // Expand the per segment split plane to per cell
    SplitPermutationArrayHandle splits(segmentIds, segmentSplits);
    CoordsPermutationArrayHandle planes(segmentIds, segmentPlanes);

    //START_TIMER(s31);
    IdArrayHandle leqFlags;
    invoker(viskores::worklet::spatialstructure::CalculateSplitDirectionFlag{},
            centerXs,
            centerYs,
            centerZs,
            splits,
            planes,
            leqFlags);
    //PRINT_TIMER("3.1", s31);

    //START_TIMER(s32);
    IdArrayHandle scatterIndices = CalculateSplitScatterIndices(cellIds, leqFlags, segmentIds);
    IdArrayHandle newSegmentIds;
    IdPermutationArrayHandle sizes(segmentIds, segmentSizes);
    invoker(viskores::worklet::spatialstructure::SegmentSplitter{ this->MaxLeafSize },
            segmentIds,
            leqFlags,
            sizes,
            newSegmentIds);
    //PRINT_TIMER("3.2", s32);

    //START_TIMER(s33);
    viskores::cont::ArrayHandle<viskores::Id> choices;
    viskores::cont::Algorithm::Copy(IdPermutationArrayHandle(segmentIds, splitChoices), choices);
    cellIds = viskores::worklet::spatialstructure::ScatterArray(cellIds, scatterIndices);
    segmentIds = viskores::worklet::spatialstructure::ScatterArray(segmentIds, scatterIndices);
    newSegmentIds =
      viskores::worklet::spatialstructure::ScatterArray(newSegmentIds, scatterIndices);
    xRanges = viskores::worklet::spatialstructure::ScatterArray(xRanges, scatterIndices);
    yRanges = viskores::worklet::spatialstructure::ScatterArray(yRanges, scatterIndices);
    zRanges = viskores::worklet::spatialstructure::ScatterArray(zRanges, scatterIndices);
    centerXs = viskores::worklet::spatialstructure::ScatterArray(centerXs, scatterIndices);
    centerYs = viskores::worklet::spatialstructure::ScatterArray(centerYs, scatterIndices);
    centerZs = viskores::worklet::spatialstructure::ScatterArray(centerZs, scatterIndices);
    choices = viskores::worklet::spatialstructure::ScatterArray(choices, scatterIndices);
    //PRINT_TIMER("3.3", s33);

    // Move the cell ids at leafs to the processed cellids list
    //START_TIMER(s41);
    IdArrayHandle nonSplitSegmentSizes;
    invoker(viskores::worklet::spatialstructure::NonSplitIndexCalculator{ this->MaxLeafSize },
            segmentSizes,
            nonSplitSegmentSizes);
    IdArrayHandle nonSplitSegmentIndices;
    viskores::cont::Algorithm::ScanExclusive(nonSplitSegmentSizes, nonSplitSegmentIndices);
    IdArrayHandle runningSplitSegmentCounts;
    viskores::Id numNewSegments =
      viskores::cont::Algorithm::ScanExclusive(splitChoices, runningSplitSegmentCounts);
    //PRINT_TIMER("4.1", s41);

    //START_TIMER(s42);
    IdArrayHandle doneCellIds;
    viskores::cont::Algorithm::CopyIf(
      cellIds, choices, doneCellIds, viskores::worklet::spatialstructure::Invert());
    viskores::cont::Algorithm::CopySubRange(
      doneCellIds, 0, doneCellIds.GetNumberOfValues(), this->ProcessedCellIds, cellIdsOffset);

    cellIds = viskores::worklet::spatialstructure::CopyIfArray(cellIds, choices);
    newSegmentIds = viskores::worklet::spatialstructure::CopyIfArray(newSegmentIds, choices);
    xRanges = viskores::worklet::spatialstructure::CopyIfArray(xRanges, choices);
    yRanges = viskores::worklet::spatialstructure::CopyIfArray(yRanges, choices);
    zRanges = viskores::worklet::spatialstructure::CopyIfArray(zRanges, choices);
    centerXs = viskores::worklet::spatialstructure::CopyIfArray(centerXs, choices);
    centerYs = viskores::worklet::spatialstructure::CopyIfArray(centerYs, choices);
    centerZs = viskores::worklet::spatialstructure::CopyIfArray(centerZs, choices);
    //PRINT_TIMER("4.2", s42);

    //START_TIMER(s43);
    // Make a new nodes with enough nodes for the current level, copying over the old one
    viskores::Id nodesSize = this->Nodes.GetNumberOfValues() + numSegments;
    viskores::cont::ArrayHandle<viskores::exec::CellLocatorBoundingIntervalHierarchyNode> newTree;
    newTree.Allocate(nodesSize);
    viskores::cont::Algorithm::CopySubRange(
      this->Nodes, 0, this->Nodes.GetNumberOfValues(), newTree);

    IdArrayHandle nextParentIndices;
    nextParentIndices.Allocate(2 * numNewSegments);

    CountingIdArrayHandle nodesIndices(nodesIndexOffset, 1, numSegments);
    viskores::worklet::spatialstructure::TreeLevelAdder nodesAdder(
      cellIdsOffset, nodesSize, this->MaxLeafSize);
    invoker(nodesAdder,
            nodesIndices,
            segmentSplits,
            nonSplitSegmentIndices,
            segmentSizes,
            runningSplitSegmentCounts,
            parentIndices,
            newTree,
            nextParentIndices);
    nodesIndexOffset = nodesSize;
    cellIdsOffset += doneCellIds.GetNumberOfValues();
    this->Nodes = newTree;
    //PRINT_TIMER("4.3", s43);
    //START_TIMER(s51);
    segmentIds = newSegmentIds;
    segmentSizes = CalculateSegmentSizes(segmentIds, segmentIds.GetNumberOfValues());
    segmentIds = GenerateSegmentIds(segmentSizes, segmentIds.GetNumberOfValues());
    IdArrayHandle uniqueSegmentIds;
    viskores::cont::Algorithm::Copy(segmentIds, uniqueSegmentIds);
    viskores::cont::Algorithm::Unique(uniqueSegmentIds);
    numSegments = uniqueSegmentIds.GetNumberOfValues();
    done = segmentIds.GetNumberOfValues() == 0;
    parentIndices = nextParentIndices;
    //PRINT_TIMER("5.1", s51);
    //std::cout << "Iteration time: " << iterationTimer.GetElapsedTime() << "\n";
  }
  //std::cout << "Total time: " << totalTimer.GetElapsedTime() << "\n";
}

struct CellLocatorBoundingIntervalHierarchy::MakeExecObject
{
  template <typename CellSetType>
  VISKORES_CONT void operator()(const CellSetType& cellSet,
                                viskores::cont::DeviceAdapterId device,
                                viskores::cont::Token& token,
                                const CellLocatorBoundingIntervalHierarchy& self,
                                ExecObjType& execObject) const
  {
    execObject = viskores::exec::CellLocatorBoundingIntervalHierarchy<CellSetType>(
      self.Nodes,
      self.ProcessedCellIds,
      cellSet,
      self.GetCoordinates().GetDataAsMultiplexer(),
      device,
      token);
  }
};

CellLocatorBoundingIntervalHierarchy::ExecObjType
CellLocatorBoundingIntervalHierarchy::PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                          viskores::cont::Token& token) const
{
  ExecObjType execObject;
  viskores::cont::CastAndCall(
    this->GetCellSet(), MakeExecObject{}, device, token, *this, execObject);
  return execObject;
}

} //namespace cont
} //namespace viskores
