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
#ifndef viskores_worklet_SplitSharpEdges_h
#define viskores_worklet_SplitSharpEdges_h

#include <viskores/worklet/CellDeepCopy.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/Invoker.h>
#include <viskores/exec/CellEdge.h>

#include <viskores/Bitset.h>
#include <viskores/CellTraits.h>
#include <viskores/TypeTraits.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace worklet
{

namespace internal
{
// Given a cell and a point on the cell, find the two edges that are
// associated with this point in canonical index
template <typename PointFromCellSetType>
VISKORES_EXEC viskores::ErrorCode FindRelatedEdges(const viskores::Id& pointIndex,
                                                   const viskores::Id& cellIndexG,
                                                   const PointFromCellSetType& pFromCellSet,
                                                   viskores::Id2& edge0G,
                                                   viskores::Id2& edge1G)
{
  typename PointFromCellSetType::CellShapeTag cellShape = pFromCellSet.GetCellShape(cellIndexG);
  typename PointFromCellSetType::IndicesType cellConnections = pFromCellSet.GetIndices(cellIndexG);
  viskores::IdComponent numPointsInCell = pFromCellSet.GetNumberOfIndices(cellIndexG);
  viskores::IdComponent numEdges;
  VISKORES_RETURN_ON_ERROR(
    viskores::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges));
  viskores::IdComponent edgeIndex = -1;
  // Find the two edges with the pointIndex
  while (true)
  {
    ++edgeIndex;
    if (edgeIndex >= numEdges)
    {
      // Bad cell. Could not find two incident edges.
      return viskores::ErrorCode::MalformedCellDetected;
    }
    viskores::IdComponent2 localEdgeIndices;
    VISKORES_RETURN_ON_ERROR(viskores::exec::CellEdgeLocalIndex(
      numPointsInCell, 0, edgeIndex, cellShape, localEdgeIndices[0]));
    VISKORES_RETURN_ON_ERROR(viskores::exec::CellEdgeLocalIndex(
      numPointsInCell, 1, edgeIndex, cellShape, localEdgeIndices[1]));
    viskores::Id2 canonicalEdgeId(cellConnections[localEdgeIndices[0]],
                                  cellConnections[localEdgeIndices[1]]);
    if (canonicalEdgeId[0] == pointIndex || canonicalEdgeId[1] == pointIndex)
    { // Assign value to edge0 first
      if ((edge0G[0] == -1) && (edge0G[1] == -1))
      {
        edge0G = canonicalEdgeId;
      }
      else
      {
        edge1G = canonicalEdgeId;
        break;
      }
    }
  }
  return viskores::ErrorCode::Success;
}

// TODO: We should replace this expensive lookup with a WholeCellSetIn<Edge, Cell> map.
// Given an edge on a cell, it would find the neighboring
// cell of this edge in local index. If it's a non manifold edge, -1 would be returned.
template <typename PointFromCellSetType, typename IncidentCellVecType>
VISKORES_EXEC int FindNeighborCellInLocalIndex(const viskores::Id2& eOI,
                                               const PointFromCellSetType& pFromCellSet,
                                               const IncidentCellVecType& incidentCells,
                                               const viskores::Id currentCellLocalIndex)
{
  int neighboringCellIndex = -1;
  viskores::IdComponent numberOfIncidentCells = incidentCells.GetNumberOfComponents();
  for (viskores::IdComponent incidentCellIndex = 0; incidentCellIndex < numberOfIncidentCells;
       incidentCellIndex++)
  {
    if (currentCellLocalIndex == incidentCellIndex)
    {
      continue; // No need to check the current interested cell
    }
    viskores::Id cellIndexG = incidentCells[incidentCellIndex]; // Global cell index
    typename PointFromCellSetType::CellShapeTag cellShape = pFromCellSet.GetCellShape(cellIndexG);
    typename PointFromCellSetType::IndicesType cellConnections =
      pFromCellSet.GetIndices(cellIndexG);
    viskores::IdComponent numPointsInCell = pFromCellSet.GetNumberOfIndices(cellIndexG);
    viskores::IdComponent numEdges;
    viskores::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
    viskores::IdComponent edgeIndex = -1;
    // Check if this cell has edge of interest
    while (true)
    {
      ++edgeIndex;
      if (edgeIndex >= numEdges)
      {
        break;
      }
      viskores::IdComponent2 localEdgeIndices;
      viskores::exec::CellEdgeLocalIndex(
        numPointsInCell, 0, edgeIndex, cellShape, localEdgeIndices[0]);
      viskores::exec::CellEdgeLocalIndex(
        numPointsInCell, 1, edgeIndex, cellShape, localEdgeIndices[1]);
      viskores::Id2 canonicalEdgeId(cellConnections[localEdgeIndices[0]],
                                    cellConnections[localEdgeIndices[1]]);
      if ((canonicalEdgeId[0] == eOI[0] && canonicalEdgeId[1] == eOI[1]) ||
          (canonicalEdgeId[0] == eOI[1] && canonicalEdgeId[1] == eOI[0]))
      {
        neighboringCellIndex = incidentCellIndex;
        break;
      }
    }
  }
  return neighboringCellIndex;
}

// Generalized logic for finding what 'regions' own the connected cells.
template <typename IncidentCellVecType, typename PointFromCellSetType, typename FaceNormalVecType>
VISKORES_EXEC bool FindConnectedCellOwnerships(viskores::FloatDefault cosFeatureAngle,
                                               const IncidentCellVecType& incidentCells,
                                               viskores::Id pointIndex,
                                               const PointFromCellSetType& pFromCellSet,
                                               const FaceNormalVecType& faceNormals,
                                               viskores::Id visitedCellsRegionIndex[64],
                                               viskores::Id& regionIndex)
{
  const viskores::IdComponent numberOfIncidentCells = incidentCells.GetNumberOfComponents();
  VISKORES_ASSERT(numberOfIncidentCells < 64);
  if (numberOfIncidentCells <= 1)
  {
    return false; // Not enough cells to compare
  }
  // Initialize a global cell mask to avoid confusion. globalCellIndex->status
  // 0 means not visited yet 1 means visited.
  viskores::Bitset<viskores::UInt64> visitedCells;
  // Reallocate memory for visitedCellsGroup if needed

  // Loop through each cell
  for (viskores::IdComponent incidentCellIndex = 0; incidentCellIndex < numberOfIncidentCells;
       incidentCellIndex++)
  {
    viskores::Id cellIndexG = incidentCells[incidentCellIndex]; // cell index in global order
    // If not visited
    if (!visitedCells.test(incidentCellIndex))
    {
      // Mark the cell and track the region
      visitedCells.set(incidentCellIndex);
      visitedCellsRegionIndex[incidentCellIndex] = regionIndex;

      // Find two edges containing the current point in canonial index
      viskores::Id2 edge0G(-1, -1), edge1G(-1, -1);
      internal::FindRelatedEdges(pointIndex, cellIndexG, pFromCellSet, edge0G, edge1G);
      // Grow the area along each edge
      for (size_t i = 0; i < 2; i++)
      { // Reset these two values for each grow operation
        viskores::Id2 currentEdgeG = i == 0 ? edge0G : edge1G;
        viskores::IdComponent currentTestingCellIndex = incidentCellIndex;
        while (currentTestingCellIndex >= 0)
        {
          // Find the neighbor cell of the current cell edge in local index
          int neighboringCellIndexQuery = internal::FindNeighborCellInLocalIndex(
            currentEdgeG, pFromCellSet, incidentCells, currentTestingCellIndex);
          // The edge should be manifold and the neighboring cell should
          // have not been visited
          if (neighboringCellIndexQuery != -1 && !visitedCells.test(neighboringCellIndexQuery))
          {
            viskores::IdComponent neighborCellIndex =
              static_cast<viskores::IdComponent>(neighboringCellIndexQuery);
            // Try to grow the area if the feature angle between current neighbor
            auto thisNormal = faceNormals[currentTestingCellIndex];
            //neighborNormal
            auto neighborNormal = faceNormals[neighborCellIndex];
            // Try to grow the area
            if (viskores::dot(thisNormal, neighborNormal) > cosFeatureAngle)
            { // No need to split.
              visitedCells.set(neighborCellIndex);

              // Mark the region visited
              visitedCellsRegionIndex[neighborCellIndex] = regionIndex;

              // Move to examine next cell
              currentTestingCellIndex = neighborCellIndex;
              viskores::Id2 neighborCellEdge0G(-1, -1), neighborCellEdge1G(-1, -1);
              internal::FindRelatedEdges(pointIndex,
                                         incidentCells[currentTestingCellIndex],
                                         pFromCellSet,
                                         neighborCellEdge0G,
                                         neighborCellEdge1G);
              // Update currentEdgeG
              if ((currentEdgeG == neighborCellEdge0G) ||
                  currentEdgeG == viskores::Id2(neighborCellEdge0G[1], neighborCellEdge0G[0]))
              {
                currentEdgeG = neighborCellEdge1G;
              }
              else
              {
                currentEdgeG = neighborCellEdge0G;
              }
            }
            else
            {
              currentTestingCellIndex = -1;
            }
          }
          else
          {
            currentTestingCellIndex =
              -1; // Either seperated by previous visit, boundary or non-manifold
          }
          // cells is smaller than the thresold and the nighboring cell has not been visited
        }
      }
      regionIndex++;
    }
  }
  return true;
}

} // internal namespace

// Split sharp manifold edges where the feature angle between the
// adjacent surfaces are larger than the threshold value
class SplitSharpEdges
{
public:
  // This worklet would calculate the needed space for splitting sharp edges.
  // For each point, it would have two values as numberOfNewPoint(how many
  // times this point needs to be duplicated) and numberOfCellsNeedsUpdate
  // (how many neighboring cells need to update connectivity).
  // For example, Given a unit cube and feature angle
  // as 89 degree, each point would be duplicated twice and there are two cells
  // need connectivity update. There is no guarantee on which cell would get which
  // new point.
  class ClassifyPoint : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    ClassifyPoint(viskores::FloatDefault cosfeatureAngle)
      : CosFeatureAngle(cosfeatureAngle)
    {
    }
    using ControlSignature = void(CellSetIn intputCells,
                                  WholeCellSetIn<Cell, Point>, // Query points from cell
                                  FieldInCell faceNormals,
                                  FieldOutPoint newPointNum,
                                  FieldOutPoint cellNum);
    using ExecutionSignature = void(CellIndices incidentCells,
                                    InputIndex pointIndex,
                                    _2 pFromCellSet,
                                    _3 faceNormals,
                                    _4 newPointNum,
                                    _5 cellNum);
    using InputDomain = _1;

    template <typename IncidentCellVecType,
              typename PointFromCellSetType,
              typename FaceNormalVecType>
    VISKORES_EXEC void operator()(const IncidentCellVecType& incidentCells,
                                  viskores::Id pointIndex,
                                  const PointFromCellSetType& pFromCellSet,
                                  const FaceNormalVecType& faceNormals,
                                  viskores::Id& newPointNum,
                                  viskores::Id& cellNum) const
    {
      viskores::Id regionIndex = 0;
      viskores::Id visitedCellsRegionIndex[64] = { 0 };
      const bool foundConnections = internal::FindConnectedCellOwnerships(this->CosFeatureAngle,
                                                                          incidentCells,
                                                                          pointIndex,
                                                                          pFromCellSet,
                                                                          faceNormals,
                                                                          visitedCellsRegionIndex,
                                                                          regionIndex);
      if (!foundConnections)
      {
        newPointNum = 0;
        cellNum = 0;
      }
      else
      {
        // For each new region you need a new point
        viskores::Id numberOfCellsNeedUpdate = 0;
        const viskores::IdComponent size = incidentCells.GetNumberOfComponents();
        for (viskores::IdComponent i = 0; i < size; i++)
        {
          if (visitedCellsRegionIndex[i] > 0)
          {
            numberOfCellsNeedUpdate++;
          }
        }
        newPointNum = regionIndex - 1;
        cellNum = numberOfCellsNeedUpdate;
      }
    }

  private:
    viskores::FloatDefault CosFeatureAngle; // Cos value of the feature angle
  };

  // This worklet split the sharp edges and populate the
  // cellTopologyUpdateTuples as (cellGlobalId, oldPointId, newPointId).
  class SplitSharpEdge : public viskores::worklet::WorkletVisitPointsWithCells
  {
  public:
    SplitSharpEdge(viskores::FloatDefault cosfeatureAngle, viskores::Id numberOfOldPoints)
      : CosFeatureAngle(cosfeatureAngle)
      , NumberOfOldPoints(numberOfOldPoints)
    {
    }
    using ControlSignature = void(CellSetIn intputCells,
                                  WholeCellSetIn<Cell, Point>, // Query points from cell
                                  FieldInCell faceNormals,
                                  FieldInPoint newPointStartingIndex,
                                  FieldInPoint pointCellsStartingIndex,
                                  WholeArrayOut cellTopologyUpdateTuples);
    using ExecutionSignature = void(CellIndices incidentCells,
                                    InputIndex pointIndex,
                                    _2 pFromCellSet,
                                    _3 faceNormals,
                                    _4 newPointStartingIndex,
                                    _5 pointCellsStartingIndex,
                                    _6 cellTopologyUpdateTuples);
    using InputDomain = _1;

    template <typename IncidentCellVecType,
              typename PointFromCellSetType,
              typename FaceNormalVecType,
              typename CellTopologyUpdateTuples>
    VISKORES_EXEC void operator()(const IncidentCellVecType& incidentCells,
                                  viskores::Id pointIndex,
                                  const PointFromCellSetType& pFromCellSet,
                                  const FaceNormalVecType& faceNormals,
                                  const viskores::Id& newPointStartingIndex,
                                  const viskores::Id& pointCellsStartingIndex,
                                  CellTopologyUpdateTuples& cellTopologyUpdateTuples) const
    {
      viskores::Id regionIndex = 0;
      viskores::Id visitedCellsRegionIndex[64] = { 0 };
      const bool foundConnections = internal::FindConnectedCellOwnerships(this->CosFeatureAngle,
                                                                          incidentCells,
                                                                          pointIndex,
                                                                          pFromCellSet,
                                                                          faceNormals,
                                                                          visitedCellsRegionIndex,
                                                                          regionIndex);
      if (foundConnections)
      {
        // For each new region you need a new point
        // Initialize the offset in the global cellTopologyUpdateTuples;
        viskores::Id cellTopologyUpdateTuplesIndex = pointCellsStartingIndex;
        const viskores::IdComponent size = incidentCells.GetNumberOfComponents();
        for (viskores::Id i = 0; i < size; i++)
        {
          if (visitedCellsRegionIndex[i])
          { // New region generated. Need to update the topology
            viskores::Id replacementPointId =
              NumberOfOldPoints + newPointStartingIndex + visitedCellsRegionIndex[i] - 1;
            viskores::Id globalCellId = incidentCells[static_cast<viskores::IdComponent>(i)];
            // (cellGlobalIndex, oldPointId, replacementPointId)
            viskores::Id3 tuple = viskores::make_Vec(globalCellId, pointIndex, replacementPointId);
            cellTopologyUpdateTuples.Set(cellTopologyUpdateTuplesIndex, tuple);
            cellTopologyUpdateTuplesIndex++;
          }
        }
      }
    }

  private:
    viskores::FloatDefault CosFeatureAngle; // Cos value of the feature angle
    viskores::Id NumberOfOldPoints;
  };

  template <typename CellSetType,
            typename FaceNormalsType,
            typename CoordsComType,
            typename CoordsInStorageType,
            typename CoordsOutStorageType,
            typename NewCellSetType>
  void Run(
    const CellSetType& oldCellset,
    const viskores::FloatDefault featureAngle,
    const FaceNormalsType& faceNormals,
    const viskores::cont::ArrayHandle<viskores::Vec<CoordsComType, 3>, CoordsInStorageType>&
      oldCoords,
    viskores::cont::ArrayHandle<viskores::Vec<CoordsComType, 3>, CoordsOutStorageType>& newCoords,
    NewCellSetType& newCellset)
  {
    viskores::cont::Invoker invoke;

    const viskores::FloatDefault featureAngleR = featureAngle /
      static_cast<viskores::FloatDefault>(180.0) * viskores::Pi<viskores::FloatDefault>();

    //Launch the first kernel that computes which points need to be split
    viskores::cont::ArrayHandle<viskores::Id> newPointNums, cellNeedUpdateNums;
    ClassifyPoint classifyPoint(viskores::Cos(featureAngleR));
    invoke(classifyPoint, oldCellset, oldCellset, faceNormals, newPointNums, cellNeedUpdateNums);
    VISKORES_ASSERT(newPointNums.GetNumberOfValues() == oldCoords.GetNumberOfValues());

    //Compute relevant information from cellNeedUpdateNums so we can release
    //that memory asap
    viskores::cont::ArrayHandle<viskores::Id> pointCellsStartingIndexs;
    viskores::cont::Algorithm::ScanExclusive(cellNeedUpdateNums, pointCellsStartingIndexs);

    const viskores::Id cellsNeedUpdateNum =
      viskores::cont::Algorithm::Reduce(cellNeedUpdateNums, viskores::Id(0));
    cellNeedUpdateNums.ReleaseResources();


    //Compute the mapping of new points to old points. This is required for
    //processing additional point fields
    const viskores::Id totalNewPointsNum =
      viskores::cont::Algorithm::Reduce(newPointNums, viskores::Id(0));
    this->NewPointsIdArray.Allocate(oldCoords.GetNumberOfValues() + totalNewPointsNum);
    viskores::cont::Algorithm::CopySubRange(
      viskores::cont::make_ArrayHandleCounting(
        viskores::Id(0), viskores::Id(1), oldCoords.GetNumberOfValues()),
      0,
      oldCoords.GetNumberOfValues(),
      this->NewPointsIdArray,
      0);
    auto newPointsIdArrayPortal = this->NewPointsIdArray.WritePortal();

    // Fill the new point coordinate system with all the existing values
    newCoords.Allocate(oldCoords.GetNumberOfValues() + totalNewPointsNum);
    viskores::cont::Algorithm::CopySubRange(oldCoords, 0, oldCoords.GetNumberOfValues(), newCoords);

    if (totalNewPointsNum > 0)
    { //only if we have new points do we need add any of the new
      //coordinate locations
      viskores::Id newCoordsIndex = oldCoords.GetNumberOfValues();
      auto oldCoordsPortal = oldCoords.ReadPortal();
      auto newCoordsPortal = newCoords.WritePortal();
      auto newPointNumsPortal = newPointNums.WritePortal();
      for (viskores::Id i = 0; i < oldCoords.GetNumberOfValues(); i++)
      { // Find out for each new point, how many times it should be added
        for (viskores::Id j = 0; j < newPointNumsPortal.Get(i); j++)
        {
          newPointsIdArrayPortal.Set(newCoordsIndex, i);
          newCoordsPortal.Set(newCoordsIndex++, oldCoordsPortal.Get(i));
        }
      }
    }

    // Allocate the size for the updateCellTopologyArray
    viskores::cont::ArrayHandle<viskores::Id3> cellTopologyUpdateTuples;
    cellTopologyUpdateTuples.Allocate(cellsNeedUpdateNum);

    viskores::cont::ArrayHandle<viskores::Id> newpointStartingIndexs;
    viskores::cont::Algorithm::ScanExclusive(newPointNums, newpointStartingIndexs);
    newPointNums.ReleaseResources();


    SplitSharpEdge splitSharpEdge(viskores::Cos(featureAngleR), oldCoords.GetNumberOfValues());
    invoke(splitSharpEdge,
           oldCellset,
           oldCellset,
           faceNormals,
           newpointStartingIndexs,
           pointCellsStartingIndexs,
           cellTopologyUpdateTuples);
    auto ctutPortal = cellTopologyUpdateTuples.ReadPortal();
    viskores::cont::printSummary_ArrayHandle(cellTopologyUpdateTuples, std::cout);


    // Create the new cellset
    CellDeepCopy::Run(oldCellset, newCellset, this->NewPointsIdArray.GetNumberOfValues());
    // FIXME: Since the non const get array function is not in CellSetExplict.h,
    // here I just get a non-const copy of the array handle.
    auto connectivityArrayHandle = newCellset.GetConnectivityArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    auto connectivityArrayHandleP = connectivityArrayHandle.WritePortal();
    auto offsetArrayHandle = newCellset.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                                        viskores::TopologyElementTagPoint());
    auto offsetArrayHandleP = offsetArrayHandle.WritePortal();
    for (viskores::Id i = 0; i < cellTopologyUpdateTuples.GetNumberOfValues(); i++)
    {
      viskores::Id cellId(ctutPortal.Get(i)[0]), oldPointId(ctutPortal.Get(i)[1]),
        newPointId(ctutPortal.Get(i)[2]);
      viskores::Id bound = (cellId + 1 == offsetArrayHandle.GetNumberOfValues())
        ? connectivityArrayHandle.GetNumberOfValues()
        : offsetArrayHandleP.Get(cellId + 1);
      viskores::Id k = 0;
      for (viskores::Id j = offsetArrayHandleP.Get(cellId); j < bound; j++, k++)
      {
        if (connectivityArrayHandleP.Get(j) == oldPointId)
        {
          connectivityArrayHandleP.Set(j, newPointId);
        }
      }
    }
  }

  viskores::cont::ArrayHandle<viskores::Id> GetNewPointsIdArray() const
  {
    return this->NewPointsIdArray;
  }

private:
  viskores::cont::ArrayHandle<viskores::Id> NewPointsIdArray;
};
}
} // viskores::worklet

#endif // viskores_worklet_SplitSharpEdges_h
