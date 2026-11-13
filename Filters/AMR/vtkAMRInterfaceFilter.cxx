// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRInterfaceFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCartesianGrid.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMergePoints.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataNormals.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
struct AMRPoint_t
{
  unsigned int level;
  unsigned int idx;
  vtkIdType pointIdx;
  bool contact = false; // contact points are point that touch multiple grids at the same time
};

/*
 * Recover the walk axis between two point indices in provided grid
 * Also set the min and max struct coordinates and set the inverted flag accordingly
 */
int RecoverWalkAxis(vtkCartesianGrid* grid, const std::array<vtkIdType, 2>& pointIds, int* ijkMin,
  int* ijkMax, bool& inverted)
{
  // Recover the ijkMin and ijkMax of the edge
  int ijk[2][3];
  for (int i = 0; i < 2; i++)
  {
    vtkStructuredData::ComputePointStructuredCoordsForExtent(
      pointIds[i], grid->GetExtent(), ijk[i]);

    // Extract the min and max
    for (int j = 0; j < 3; j++)
    {
      ijkMin[j] = std::min(ijk[i][j], ijkMin[j]);
      ijkMax[j] = std::max(ijk[i][j], ijkMax[j]);
    }
  }

  // Identify the axis of the edge
  int walkAxis = -1;
  for (int j = 0; j < 3; j++)
  {
    if (ijkMin[j] != ijkMax[j])
    {
      walkAxis = j;
      break;
    }
  }
  assert(walkAxis != -1);

  inverted = ijk[0][walkAxis] > ijk[1][walkAxis];
  return walkAxis;
}

/*
 * Walk along an edge and find all intermiedary refined points
 */
bool InsertIntermediaryAMRPoints(vtkOverlappingAMR* amr, vtkCartesianGrid* roughGrid,
  const std::array<vtkIdType, 2>& pointIds, std::vector<AMRPoint_t>& amrPoints, bool& warned)
{
  assert(amr != nullptr);
  assert(amrPoints.size() == 2);

  if (amrPoints[0].level == amrPoints[1].level && amrPoints[0].idx == amrPoints[1].idx)
  {
    // Both points in the same grid, recover all points in between, along the edge
    vtkCartesianGrid* refinedGrid =
      amr->GetDataSetAsCartesianGrid(amrPoints[0].level, amrPoints[0].idx);

    int ijkMin[3] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
      std::numeric_limits<int>::max() };
    int ijkMax[3] = { 0, 0, 0 };
    std::array<vtkIdType, 2> refinedPointIds = { amrPoints[0].pointIdx, amrPoints[1].pointIdx };
    bool inverted;
    int walkAxis = RecoverWalkAxis(refinedGrid, refinedPointIds, ijkMin, ijkMax, inverted);

    // walk alongside the edge, ignoring first and last point
    AMRPoint_t amrPoint = amrPoints[0];
    for (ijkMin[walkAxis]++; ijkMin[walkAxis] < ijkMax[walkAxis]; ijkMin[walkAxis]++)
    {
      amrPoint.pointIdx =
        vtkStructuredData::ComputePointIdForExtent(refinedGrid->GetExtent(), ijkMin);
      if (inverted)
      {
        amrPoints.insert(amrPoints.begin() + 1, amrPoint);
      }
      else
      {
        amrPoints.insert(amrPoints.end() - 1, amrPoint);
      }
    }
  }
  else
  {
    if (amrPoints[0].level != amrPoints[1].level && !warned)
    {
      vtkWarningWithObjectMacro(nullptr,
        "A refined edge is in contact with grids of different levels, resulting may not be "
        "watertight");
      warned = true;
    }

    // Different grids, split the edge in two

    // Recover the walkaxis using the rough edge
    int ijkMin[3] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
      std::numeric_limits<int>::max() };
    int ijkMax[3] = { 0, 0, 0 };
    bool inverted;
    int walkAxis = RecoverWalkAxis(roughGrid, pointIds, ijkMin, ijkMax, inverted);

    // Recover the grids
    AMRPoint_t amrPoint = amrPoints.front();
    AMRPoint_t backPoint = amrPoints.back();
    vtkCartesianGrid* firstGrid = amr->GetDataSetAsCartesianGrid(amrPoint.level, amrPoint.idx);
    vtkCartesianGrid* secondGrid = amr->GetDataSetAsCartesianGrid(backPoint.level, backPoint.idx);

    // Recover the first grid extent
    int extent[6];
    firstGrid->GetExtent(extent);

    // walk alongside the edge, ignoring first point, up to the edge of the first grid extent or
    // to the back point
    vtkStructuredData::ComputePointStructuredCoordsForExtent(amrPoint.pointIdx, extent, ijkMin);

    double tmpPt[3];
    vtkIdType secondGridPointId = -1;

    bool skipStartPoint = false;
    if (ijkMin[walkAxis] == extent[walkAxis * 2 + 1])
    {
      // First point is at the edge, flag as contact
      amrPoints.front().contact = true;

      // Second point should be right here, recover it
      firstGrid->GetPoint(amrPoint.pointIdx, tmpPt);
      secondGridPointId = secondGrid->FindPoint(tmpPt);
      skipStartPoint = true;
    }
    else
    {
      for (ijkMin[walkAxis]++; ijkMin[walkAxis] <= extent[walkAxis * 2 + 1]; ijkMin[walkAxis]++)
      {
        amrPoint.pointIdx = vtkStructuredData::ComputePointIdForExtent(extent, ijkMin);

        // Recover point coordinates
        firstGrid->GetPoint(amrPoint.pointIdx, tmpPt);
        secondGridPointId = secondGrid->FindPoint(tmpPt);
        if (secondGridPointId != -1 && secondGridPointId == backPoint.pointIdx)
        {
          // We reached the second point
          // This check is only important when touching grids of different levels, which is not
          // fully supported yet
          break;
        }
        // Add up to the last point, but not the last
        else if (ijkMin[walkAxis] < extent[walkAxis * 2 + 1])
        {
          if (inverted)
          {
            amrPoints.insert(amrPoints.begin() + 1, amrPoint);
          }
          else
          {
            amrPoints.insert(amrPoints.end() - 1, amrPoint);
          }
        }
      }
    }

    if (secondGridPointId == -1)
    {
      vtkErrorWithObjectMacro(nullptr,
        "Could not walk the edge from one grid to another, results may be incorrect, this can "
        "happen if there is a rough edge touching more than two different refined grids");
      return false;
    }
    else if (secondGridPointId == backPoint.pointIdx)
    {
      // If we reached the end point, just stop here and flag it as contact.
      amrPoints.back().contact = true;
      return true;
    }

    // walk alongside the edge, skipping the start point only if needed, until last point is reached
    amrPoint = backPoint;
    secondGrid->GetExtent(extent);
    vtkStructuredData::ComputePointStructuredCoordsForExtent(secondGridPointId, extent, ijkMin);
    bool startPoint = true;
    for (; ijkMin[walkAxis] <= extent[walkAxis * 2 + 1]; ijkMin[walkAxis]++)
    {
      amrPoint.contact = false;
      if (startPoint)
      {
        startPoint = false;
        amrPoint.contact = true;
        if (skipStartPoint)
        {
          continue;
        }
      }

      amrPoint.pointIdx =
        vtkStructuredData::ComputePointIdForExtent(secondGrid->GetExtent(), ijkMin);
      if (amrPoint.pointIdx == amrPoints.back().pointIdx)
      {
        break;
      }

      if (inverted)
      {
        amrPoints.insert(amrPoints.begin() + 1, amrPoint);
      }
      else
      {
        amrPoints.insert(amrPoints.end() - 1, amrPoint);
      }
    }
  }
  return true;
}

/**
 * Create interface pyramids over multiple refined grids
 */
void GenerateFacePyramids(vtkCartesianGrid* inputGrid, vtkIdType cellIdx, vtkIdType centerId,
  vtkIdList* centerInterpolateOutIds, vtkCartesianGrid* grid, int* ijkMin, int* ijkMaxOrig,
  vtkMergePoints* pointInsertor, vtkPointData* pointData, vtkCellArray* cellArray,
  vtkUnsignedCharArray* cellTypesArray, vtkCellData* cellData)
{
  // identify the face axis
  int faceAxis = -1;
  double ijkMax[3];
  for (int j = 0; j < 3; j++)
  {
    if (ijkMin[j] == ijkMaxOrig[j])
    {
      ijkMax[j] = ijkMaxOrig[j] + 1;
      faceAxis = j;
    }
    else
    {
      ijkMax[j] = ijkMaxOrig[j];
    }
  }
  assert(faceAxis != -1);

  // Iterate over pyramids point to (potentially) add using the point insertor, also adds pyramids
  int ijk[3];
  vtkIdType pyramidPointIds[5] = { -1, -1, -1, -1, centerId };
  for (ijk[0] = ijkMin[0]; ijk[0] < ijkMax[0]; ijk[0]++)
  {
    for (ijk[1] = ijkMin[1]; ijk[1] < ijkMax[1]; ijk[1]++)
    {
      for (ijk[2] = ijkMin[2]; ijk[2] < ijkMax[2]; ijk[2]++)
      {
        // For each lower left point of each pyramid base
        int localIJK[3];
        constexpr std::array<int, 8> looper{ 0, 0, 1, 0, 1, 1, 0, 1 };
        for (int i = 0; i < 4; i++)
        {
          // Recover all four points of the pyramid base
          int localAxisCount = 0;
          for (int j = 0; j < 3; j++)
          {
            localIJK[j] = ijk[j];
            if (faceAxis == j)
            {
              continue;
            }
            localIJK[j] += looper[i * 2 + localAxisCount];
            localAxisCount++;
          }
          vtkIdType ptId = vtkStructuredData::ComputePointIdForExtent(grid->GetExtent(), localIJK);
          double* tmpPt = grid->GetPoint(ptId);

          // Add the pyramid base, points may already have been added
          if (pointInsertor->InsertUniquePoint(tmpPt, pyramidPointIds[i]) != 0)
          {
            pointData->CopyData(grid->GetPointData(), ptId, pyramidPointIds[i]);
          }
          else if (grid != inputGrid)
          {
            // Copy the data again even if the point has already been added when grid != inputGrid
            // because in that case it means grid is of higher refinment than inputGrid and we need
            // to use higher refinement data have been added at some point
            pointData->CopyData(grid->GetPointData(), ptId, pyramidPointIds[i]);
          }

          // Add to the center interpolation id list
          centerInterpolateOutIds->InsertUniqueId(pyramidPointIds[i]);
        }

        vtkIdType interfaceCellId = cellArray->InsertNextCell(5, pyramidPointIds);
        cellTypesArray->InsertNextValue(VTK_PYRAMID);
        cellData->CopyData(inputGrid->GetCellData(), cellIdx, interfaceCellId);
      }
    }
  }
}

/**
 * Create interface pyramids for provided face for provided grid
 * \arg input Overlapping AMR input to look for refined grids into
 * \arg inputGrid non-refined grid containing the cell from which we generate the face pyramid on
 * \arg cellIdx index of the cell from which we generate the face pyramid on
 * \arg centerId id of the voxel center in the interface points
 * \arg centerInterpolateOutIds id list to accumulate added point ids into
 * \arg interfaceEdgesMap map to recover edges locations and grids from
 * \arg facePointIds point ids of the face we will generate a pyramid for
 * \arg pointInsertor locator to insert unique points into the interface
 * \arg pointData interface point data to copy point data into
 * \arg cellArray interface cell array to add pyramids into
 * \arg cellTypesArray interface cell types array to add pyramid types into
 * \arg cellData interface cell data to copy cell data into
 */
bool GenerateFacePyramids(vtkOverlappingAMR* input, vtkCartesianGrid* inputGrid, vtkIdType cellIdx,
  vtkIdType centerId, vtkIdList* centerInterpolateOutIds,
  const std::map<std::array<vtkIdType, 2>, std::vector<::AMRPoint_t>>& interfaceEdgesMap,
  vtkIdList* facePointIds, vtkMergePoints* pointInsertor, vtkPointData* pointData,
  vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypesArray, vtkCellData* cellData)
{
  std::set<vtkCartesianGrid*> foundGrids;

  // Iterate on each point of the face and create pyramids on the refined grid from there
  for (int i = 0; i < 4; i++)
  {
    // Recover both edges on each side of the point
    std::array<vtkIdType, 2> edge0{ facePointIds->GetId(i), facePointIds->GetId((i + 1) % 4) };
    std::vector<::AMRPoint_t> amrPointsEdge0 = interfaceEdgesMap.at(edge0);
    std::array<vtkIdType, 2> edge1{ facePointIds->GetId((i + 3) % 4), facePointIds->GetId(i) };
    std::vector<::AMRPoint_t> amrPointsEdge1 = interfaceEdgesMap.at(edge1);

    // Identify a subface for a single refined grid, which is just two opposite point of the subface

    // First point is the one common point on both edges
    bool foundCommon = false;
    bool contact = false;
    ::AMRPoint_t firstPoint;
    for (size_t iEdge = 0; iEdge < amrPointsEdge0.size() && foundCommon == false; iEdge++)
    {
      firstPoint = amrPointsEdge0[iEdge];
      for (::AMRPoint_t secondPoint : amrPointsEdge1)
      {
        if (firstPoint.level == secondPoint.level && firstPoint.idx == secondPoint.idx &&
          firstPoint.pointIdx == secondPoint.pointIdx)
        {
          contact = firstPoint.contact || secondPoint.contact;
          foundCommon = true;
          break;
        }
      }
    }
    assert(foundCommon);

    // Ignore contact points on the corners
    if (contact)
    {
      continue;
    }

    // Check if grid has already been processed
    vtkCartesianGrid* refinedGrid =
      input->GetDataSetAsCartesianGrid(firstPoint.level, firstPoint.idx);
    if (foundGrids.find(refinedGrid) != foundGrids.end())
    {
      continue;
    }
    foundGrids.emplace(refinedGrid);

    // Iterate on both edges to find the opposite point in the same refined grid, in local struct
    // coords Store the result in a vtkAMRBox for simplicity
    int ijk[3];
    vtkStructuredData::ComputePointStructuredCoordsForExtent(
      firstPoint.pointIdx, refinedGrid->GetExtent(), ijk);
    vtkAMRBox box(ijk, ijk);

    for (::AMRPoint_t amrPoint : amrPointsEdge0)
    {
      vtkCartesianGrid* tmpRefinedGrid =
        input->GetDataSetAsCartesianGrid(amrPoint.level, amrPoint.idx);
      if (refinedGrid == tmpRefinedGrid)
      {
        vtkStructuredData::ComputePointStructuredCoordsForExtent(
          amrPoint.pointIdx, refinedGrid->GetExtent(), ijk);
        box.Add(ijk);
      }
      else if (amrPoint.contact)
      {
        // Its a contact point, add it anyway, but from the current grid
        double* tmpPt = tmpRefinedGrid->GetPoint(amrPoint.pointIdx);
        vtkIdType refinedPointId = refinedGrid->FindPoint(tmpPt);
        if (refinedPointId == -1)
        {
          vtkErrorWithObjectMacro(
            nullptr, "Unexpected, could not find a point even though its a contact one, aborting");
          return false;
        }

        vtkStructuredData::ComputePointStructuredCoordsForExtent(
          refinedPointId, refinedGrid->GetExtent(), ijk);
        box.Add(ijk);
      }
    }

    for (::AMRPoint_t amrPoint : amrPointsEdge1)
    {
      vtkCartesianGrid* tmpRefinedGrid =
        input->GetDataSetAsCartesianGrid(amrPoint.level, amrPoint.idx);
      if (refinedGrid == tmpRefinedGrid)
      {
        vtkStructuredData::ComputePointStructuredCoordsForExtent(
          amrPoint.pointIdx, refinedGrid->GetExtent(), ijk);
        box.Add(ijk);
      }
      else if (amrPoint.contact)
      {
        // Its a contact point, add it anyway, but from the current grid
        double* tmpPt = tmpRefinedGrid->GetPoint(amrPoint.pointIdx);
        vtkIdType refinedPointId = refinedGrid->FindPoint(tmpPt);
        if (refinedPointId == -1)
        {
          vtkErrorWithObjectMacro(
            nullptr, "Unexpected, could not find a point even though its a contact one, aborting");
          return false;
        }

        vtkStructuredData::ComputePointStructuredCoordsForExtent(
          refinedPointId, refinedGrid->GetExtent(), ijk);
        box.Add(ijk);
      }
    }

    // Use the AMRBox to generate pyramids for the subface
    GenerateFacePyramids(inputGrid, cellIdx, centerId, centerInterpolateOutIds, refinedGrid,
      const_cast<int*>(box.GetLoCorner()), const_cast<int*>(box.GetHiCorner()), pointInsertor,
      pointData, cellArray, cellTypesArray, cellData);
  }
  return true;
}

/**
 * Create interface tetrahedron for provided face, linking grids of different refinements in a
 * coherent way \arg input Overlapping AMR input to look for refined grids into \arg inputGrid
 * non-refined grid containing the cell from which we generate the face pyramid on \arg
 * interfaceEdgesMap map of refined edges to find points to create tetrahedrons with \arg cellIdx
 * index of the cell where we are creating cells in the inputGrid \arg centerId index of the center
 * of the cell in the points \arg faceCenter index of the center of the face in the points \arg
 * facePointIds point ids of the face we are generating tetrahedrons for \arg
 * centerInterpolateOutIds id list to accumulate added point ids into \arg pointInsertor locator to
 * insert unique points into the interface \arg points used to recover already inserted points by id
 * \arg pointData interface point data to copy point data into
 * \arg cellArray interface cell array to add pyramids into
 * \arg cellTypesArray interface cell types array to add pyramid types into
 * \arg cellData interface cell data to copy cell data into
 */
void GenerateInterfaceTetrahedrons(vtkOverlappingAMR* input, vtkCartesianGrid* inputGrid,
  const std::map<std::array<vtkIdType, 2>, std::vector<::AMRPoint_t>>& interfaceEdgesMap,
  vtkIdType cellIdx, vtkIdType centerId, double* faceCenter, vtkIdList* facePointIds,
  vtkIdList* centerInterpolateOutIds, vtkMergePoints* pointInsertor, vtkPoints* points,
  vtkPointData* pointData, vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypesArray,
  vtkCellData* cellData)
{
  // = Add all refined edge points and store their ids =

  vtkNew<vtkIdList> faceOutIds;
  std::vector<std::array<vtkIdType, 2>> nonRefineEdges;
  std::vector<int> nonRefineEdgeIds;
  std::array<std::vector<vtkIdType>, 4> edgeOutPointIds;

  for (int edgeId = 0; edgeId < 4; edgeId++)
  {
    std::vector<vtkIdType> outPointIds;

    // Check each edge, access cannot fail
    std::array<vtkIdType, 2> edge{ facePointIds->GetId(edgeId),
      facePointIds->GetId((edgeId + 1) % 4) };
    std::vector<::AMRPoint_t> amrPoints = interfaceEdgesMap.at(edge);
    if (amrPoints.size() == 2)
    {
      // Not a refined edge, store the edgeId to add
      nonRefineEdges.emplace_back(edge);
      nonRefineEdgeIds.emplace_back(edgeId);
      continue;
    }
    else
    {
      // Refined edge, find all points in the refined grid alongside the edge

      // Alongside the edge, recover points from the refined grid and add them
      for (AMRPoint_t amrPoint : amrPoints)
      {
        vtkCartesianGrid* refinedGrid =
          input->GetDataSetAsCartesianGrid(amrPoint.level, amrPoint.idx);
        double* tmpPt = refinedGrid->GetPoint(amrPoint.pointIdx);
        vtkIdType outPtId;

        if (pointInsertor->InsertUniquePoint(tmpPt, outPtId) != 0)
        {
          // Recover data from the refined grid
          pointData->CopyData(refinedGrid->GetPointData(), amrPoint.pointIdx, outPtId);
        }

        // add to the face id list
        faceOutIds->InsertUniqueId(outPtId);

        // add to the output id vector
        outPointIds.emplace_back(outPtId);

        // Add to the center interpolation id list
        centerInterpolateOutIds->InsertUniqueId(outPtId);
      }
    }

    // Store the point id vector for the edge
    edgeOutPointIds[edgeId] = outPointIds;
  }

  // = Add all remaining non-refined edge points =

  for (size_t i = 0; i < nonRefineEdges.size(); i++)
  {
    auto edge = nonRefineEdges[i];
    int edgeId = nonRefineEdgeIds[i];
    std::vector<vtkIdType> outPointIds;
    for (int j = 0; j < 2; j++)
    {
      vtkIdType outPtId;
      if (pointInsertor->InsertUniquePoint(inputGrid->GetPoint(edge[j]), outPtId) != 0)
      {
        // Recover data from the non-refined grid
        pointData->CopyData(inputGrid->GetPointData(), edge[j], outPtId);
      }

      // Add to face id list
      faceOutIds->InsertUniqueId(outPtId);

      // Add to output id vector
      outPointIds.emplace_back(outPtId);

      // Add to the center interpolation id list
      centerInterpolateOutIds->InsertUniqueId(outPtId);
    }

    // Store the point id vector for the edge
    edgeOutPointIds[edgeId] = outPointIds;
  }

  // = Add the face center if not already added =

  vtkIdType faceCenterId;
  if (pointInsertor->InsertUniquePoint(faceCenter, faceCenterId) != 0)
  {
    // Interpolate the face center data from the edges data using a simple sheperd interpolation
    std::vector<double> vecWeights(faceOutIds->GetNumberOfIds());
    double runningDist = 0;
    for (int i = 0; i < faceOutIds->GetNumberOfIds(); i++)
    {
      // distance cannot be 0
      vecWeights[i] = 1 /
        std::sqrt(
          vtkMath::Distance2BetweenPoints(faceCenter, points->GetPoint(faceOutIds->GetId(i))));
      runningDist += vecWeights[i];
    }
    std::transform(vecWeights.begin(), vecWeights.end(), vecWeights.begin(),
      [runningDist](double dist) { return dist / runningDist; });
    pointData->InterpolatePoint(pointData, faceCenterId, faceOutIds, vecWeights.data());

    // Add to the center interpolation id list
    centerInterpolateOutIds->InsertUniqueId(faceCenterId);
  }

  // = Add tetrahedron for each edges of the face =

  for (int i = 0; i < 4; i++)
  {
    const std::vector<vtkIdType>& edgeOutIds = edgeOutPointIds[i];

    // Create a tetrahedron for each part of the edge
    vtkIdType tetraPointIds[5] = { -1, -1, faceCenterId, centerId };
    for (size_t j = 0; j < edgeOutIds.size() - 1; j++)
    {
      tetraPointIds[0] = edgeOutIds.at(j);
      tetraPointIds[1] = edgeOutIds.at(j + 1);
      vtkIdType interfaceCellId = cellArray->InsertNextCell(4, tetraPointIds);
      cellTypesArray->InsertNextValue(VTK_TETRA);
      cellData->CopyData(inputGrid->GetCellData(), cellIdx, interfaceCellId);
    }
  }
}
}

//------------------------------------------------------------------------------
namespace
{
constexpr int BORROWED_GRID_TAG = 31415;

/*
 * Remove the layers of duplicate ghost cells around an image grid, so the interface is built on the
 * valid region only. Point data computed with ghost cells, which agrees across boxes, is kept.
 */
vtkSmartPointer<vtkCartesianGrid> PeelGhostLayers(vtkCartesianGrid* grid)
{
  vtkImageData* image = vtkImageData::SafeDownCast(grid);
  vtkUnsignedCharArray* ghosts = grid->GetCellGhostArray();
  if (!image || !ghosts)
  {
    return grid;
  }

  int extent[6];
  image->GetExtent(extent);
  int cropped[6];
  std::copy(extent, extent + 6, cropped);

  auto isGhostSlab = [&](int axis, int index)
  {
    int lo[3], hi[3];
    for (int d = 0; d < 3; ++d)
    {
      lo[d] = d == axis ? index : cropped[2 * d];
      hi[d] = d == axis ? index : std::max(cropped[2 * d], cropped[2 * d + 1] - 1);
    }
    int ijk[3];
    for (ijk[2] = lo[2]; ijk[2] <= hi[2]; ++ijk[2])
    {
      for (ijk[1] = lo[1]; ijk[1] <= hi[1]; ++ijk[1])
      {
        for (ijk[0] = lo[0]; ijk[0] <= hi[0]; ++ijk[0])
        {
          if (!(ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(extent, ijk)) &
                vtkDataSetAttributes::DUPLICATECELL))
          {
            return false;
          }
        }
      }
    }
    return true;
  };

  for (int axis = 0; axis < 3; ++axis)
  {
    while (cropped[2 * axis + 1] - cropped[2 * axis] > 1 && isGhostSlab(axis, cropped[2 * axis]))
    {
      ++cropped[2 * axis];
    }
    while (
      cropped[2 * axis + 1] - cropped[2 * axis] > 1 && isGhostSlab(axis, cropped[2 * axis + 1] - 1))
    {
      --cropped[2 * axis + 1];
    }
  }
  if (std::equal(extent, extent + 6, cropped))
  {
    return grid;
  }

  vtkSmartPointer<vtkImageData> peeled = vtkSmartPointer<vtkImageData>::Take(image->NewInstance());
  peeled->SetOrigin(image->GetOrigin());
  peeled->SetSpacing(image->GetSpacing());
  peeled->SetDirectionMatrix(image->GetDirectionMatrix());
  peeled->SetExtent(cropped);

  vtkCellData* inCD = image->GetCellData();
  vtkCellData* outCD = peeled->GetCellData();
  outCD->CopyAllocate(inCD, peeled->GetNumberOfCells());
  int ijk[3];
  int cellHi[3];
  for (int d = 0; d < 3; ++d)
  {
    cellHi[d] = std::max(cropped[2 * d], cropped[2 * d + 1] - 1);
  }
  for (ijk[2] = cropped[4]; ijk[2] <= cellHi[2]; ++ijk[2])
  {
    for (ijk[1] = cropped[2]; ijk[1] <= cellHi[1]; ++ijk[1])
    {
      for (ijk[0] = cropped[0]; ijk[0] <= cellHi[0]; ++ijk[0])
      {
        outCD->CopyData(inCD, vtkStructuredData::ComputeCellIdForExtent(extent, ijk),
          vtkStructuredData::ComputeCellIdForExtent(cropped, ijk));
      }
    }
  }

  vtkPointData* inPD = image->GetPointData();
  vtkPointData* outPD = peeled->GetPointData();
  outPD->CopyAllocate(inPD, peeled->GetNumberOfPoints());
  for (ijk[2] = cropped[4]; ijk[2] <= cropped[5]; ++ijk[2])
  {
    for (ijk[1] = cropped[2]; ijk[1] <= cropped[3]; ++ijk[1])
    {
      for (ijk[0] = cropped[0]; ijk[0] <= cropped[1]; ++ijk[0])
      {
        outPD->CopyData(inPD, vtkStructuredData::ComputePointIdForExtent(extent, ijk),
          vtkStructuredData::ComputePointIdForExtent(cropped, ijk));
      }
    }
  }
  return peeled;
}

/*
 * Build the AMR the interface is computed on: local grids with their ghost layers peeled, plus,
 * when distributed, the remote grids that touch a local grid. Those are exchanged with their
 * owners, which compute the same pairs from the global metadata, so no request round is needed.
 * `isLocal` flags the blocks this process owns; only those produce output.
 */
vtkSmartPointer<vtkOverlappingAMR> PrepareInput(
  vtkOverlappingAMR* input, vtkMultiProcessController* controller, std::vector<bool>& isLocal)
{
  vtkNew<vtkOverlappingAMR> work;
  work->ShallowCopy(input);

  const unsigned int numberOfBlocks = input->GetNumberOfBlocks();
  isLocal.assign(numberOfBlocks, false);
  for (unsigned int blockIdx = 0; blockIdx < numberOfBlocks; ++blockIdx)
  {
    unsigned int level, gridIdx;
    input->ComputeIndexPair(blockIdx, level, gridIdx);
    if (vtkCartesianGrid* grid = input->GetDataSetAsCartesianGrid(level, gridIdx))
    {
      isLocal[blockIdx] = true;
      vtkSmartPointer<vtkCartesianGrid> peeled = ::PeelGhostLayers(grid);
      if (peeled != grid)
      {
        work->SetDataSet(level, gridIdx, peeled);
      }
    }
  }

  if (!controller || controller->GetNumberOfProcesses() < 2 || numberOfBlocks == 0)
  {
    return work;
  }

  const int rank = controller->GetLocalProcessId();
  const int numberOfProcesses = controller->GetNumberOfProcesses();

  std::vector<int> localOwner(numberOfBlocks, 0);
  std::vector<int> owner(numberOfBlocks, 0);
  for (unsigned int blockIdx = 0; blockIdx < numberOfBlocks; ++blockIdx)
  {
    localOwner[blockIdx] = isLocal[blockIdx] ? rank + 1 : 0;
  }
  controller->AllReduce(localOwner.data(), owner.data(), numberOfBlocks, vtkCommunicator::MAX_OP);
  for (int& o : owner)
  {
    --o;
  }

  std::vector<std::array<double, 6>> bounds(numberOfBlocks);
  for (unsigned int blockIdx = 0; blockIdx < numberOfBlocks; ++blockIdx)
  {
    unsigned int level, gridIdx;
    work->ComputeIndexPair(blockIdx, level, gridIdx);
    work->GetBounds(level, gridIdx, bounds[blockIdx].data());
  }
  double spacing[3];
  work->GetSpacing(work->GetNumberOfLevels() - 1, spacing);
  double tolerance = std::numeric_limits<double>::max();
  for (double h : spacing)
  {
    if (h > 0)
    {
      tolerance = std::min(tolerance, 1e-6 * h);
    }
  }
  if (tolerance == std::numeric_limits<double>::max())
  {
    tolerance = 0.0;
  }
  auto touches = [&](unsigned int a, unsigned int b)
  {
    for (int d = 0; d < 3; ++d)
    {
      if (bounds[a][2 * d] > bounds[b][2 * d + 1] + tolerance ||
        bounds[b][2 * d] > bounds[a][2 * d + 1] + tolerance)
      {
        return false;
      }
    }
    return true;
  };

  // The blocks are bucketed on a regular grid of about one bin per block, so each local block is
  // only tested against the blocks in the bins it overlaps. A pair (local block, block of a peer)
  // that touches is sent to and received from that peer; both sides find the same pairs.
  std::vector<std::set<unsigned int>> toSend(numberOfProcesses), toReceive(numberOfProcesses);
  {
    vtkBoundingBox domain;
    for (const auto& blockBounds : bounds)
    {
      domain.AddBounds(blockBounds.data());
    }
    double binBounds[6];
    int divisions[3];
    domain.ComputeDivisions(numberOfBlocks, binBounds, divisions);
    auto binIndex = [&](double x, int d)
    {
      const double length = binBounds[2 * d + 1] - binBounds[2 * d];
      const int index = length > 0
        ? static_cast<int>(std::floor((x - binBounds[2 * d]) / length * divisions[d]))
        : 0;
      return std::min(std::max(index, 0), divisions[d] - 1);
    };
    auto forEachBin = [&](unsigned int block, auto&& function)
    {
      int lo[3], hi[3];
      for (int d = 0; d < 3; ++d)
      {
        lo[d] = binIndex(bounds[block][2 * d] - tolerance, d);
        hi[d] = binIndex(bounds[block][2 * d + 1] + tolerance, d);
      }
      for (int k = lo[2]; k <= hi[2]; ++k)
      {
        for (int j = lo[1]; j <= hi[1]; ++j)
        {
          for (int i = lo[0]; i <= hi[0]; ++i)
          {
            function(i + divisions[0] * (j + divisions[1] * k));
          }
        }
      }
    };

    std::vector<std::vector<unsigned int>> bins(
      static_cast<std::size_t>(divisions[0]) * divisions[1] * divisions[2]);
    for (unsigned int blockIdx = 0; blockIdx < numberOfBlocks; ++blockIdx)
    {
      if (owner[blockIdx] >= 0)
      {
        forEachBin(blockIdx, [&](int bin) { bins[bin].push_back(blockIdx); });
      }
    }

    std::vector<unsigned int> lastTestedWith(numberOfBlocks, numberOfBlocks);
    for (unsigned int localIdx = 0; localIdx < numberOfBlocks; ++localIdx)
    {
      if (owner[localIdx] != rank)
      {
        continue;
      }
      forEachBin(localIdx,
        [&](int bin)
        {
          for (unsigned int other : bins[bin])
          {
            if (owner[other] == rank || lastTestedWith[other] == localIdx)
            {
              continue;
            }
            lastTestedWith[other] = localIdx;
            if (touches(localIdx, other))
            {
              toSend[owner[other]].insert(localIdx);
              toReceive[owner[other]].insert(other);
            }
          }
        });
    }
  }

  for (int peer = 0; peer < numberOfProcesses; ++peer)
  {
    if (peer == rank || (toSend[peer].empty() && toReceive[peer].empty()))
    {
      continue;
    }

    auto sendAll = [&]()
    {
      for (unsigned int blockIdx : toSend[peer])
      {
        unsigned int level, gridIdx;
        work->ComputeIndexPair(blockIdx, level, gridIdx);
        vtkCartesianGrid* grid = work->GetDataSetAsCartesianGrid(level, gridIdx);
        // The communicator does not marshal vtkUniformGrid, so send its image data.
        if (vtkImageData* image = vtkImageData::SafeDownCast(grid))
        {
          vtkNew<vtkImageData> sent;
          sent->ShallowCopy(image);
          controller->Send(sent, peer, BORROWED_GRID_TAG);
        }
        else
        {
          controller->Send(grid, peer, BORROWED_GRID_TAG);
        }
      }
    };
    auto receiveAll = [&]()
    {
      for (unsigned int blockIdx : toReceive[peer])
      {
        unsigned int level, gridIdx;
        work->ComputeIndexPair(blockIdx, level, gridIdx);
        vtkSmartPointer<vtkDataObject> received = vtkSmartPointer<vtkDataObject>::Take(
          controller->ReceiveDataObject(peer, BORROWED_GRID_TAG));
        work->SetDataSet(level, gridIdx, vtkDataSet::SafeDownCast(received));
        if (!received)
        {
          vtkLog(ERROR, "Could not receive AMR grid " << blockIdx << " from process " << peer);
        }
      }
    };
    // The lower rank of each pair sends first, so blocking sends cannot deadlock.
    if (rank < peer)
    {
      sendAll();
      receiveAll();
    }
    else
    {
      receiveAll();
      sendAll();
    }
  }
  return work;
}
}

vtkStandardNewMacro(vtkAMRInterfaceFilter);
vtkCxxSetObjectMacro(vtkAMRInterfaceFilter, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkAMRInterfaceFilter::vtkAMRInterfaceFilter()
{
  // setup a callback to report progress
  this->InternalProgressObserver->SetCallback(
    &vtkAMRInterfaceFilter::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkAMRInterfaceFilter::~vtkAMRInterfaceFilter()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkAMRInterfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << "\n";
}

//------------------------------------------------------------------------------
int vtkAMRInterfaceFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRInterfaceFilter::InternalProgressCallbackFunction(
  vtkObject* arg, unsigned long, void* clientdata, void*)
{
  reinterpret_cast<vtkAMRInterfaceFilter*>(clientdata)
    ->InternalProgressCallback(static_cast<vtkAlgorithm*>(arg));
}

//------------------------------------------------------------------------------
void vtkAMRInterfaceFilter::InternalProgressCallback(vtkAlgorithm* algorithm)
{
  double progress = algorithm->GetProgress();
  this->UpdateProgress(
    this->ProgressFloor + progress * (this->ProgressCeiling - this->ProgressFloor));
}

//------------------------------------------------------------------------------
int vtkAMRInterfaceFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkOverlappingAMR* originalInput = vtkOverlappingAMR::GetData(inputVector[0]);
  if (!originalInput)
  {
    vtkErrorMacro("Input AMR dataset is nullptr!");
    return 0;
  }
  std::vector<bool> isLocal;
  vtkSmartPointer<vtkOverlappingAMR> work =
    ::PrepareInput(originalInput, this->Controller, isLocal);
  vtkOverlappingAMR* input = work;

  auto output = vtkPartitionedDataSet::GetData(outputVector);
  if (!output)
  {
    vtkErrorMacro("Output partitioned dataset is nullptr!");
    return 0;
  }

  // Will be cleaned up at the end
  output->SetNumberOfPartitions(2 * input->GetNumberOfBlocks());

  // The algorithm it split in four main part, for each block
  double progressPart = (1 / static_cast<double>(input->GetNumberOfBlocks())) / 2.0;

  // ===== 1. Check each grid and identify the interface cells =====

  // Iterate on each AMR block
  bool warnedWatertight = false;
  for (unsigned int blockIdx = 0; blockIdx < input->GetNumberOfBlocks(); blockIdx++)
  {
    this->ProgressFloor = blockIdx / static_cast<double>(input->GetNumberOfBlocks());
    this->UpdateProgress(this->ProgressFloor);
    if (this->CheckAbort())
    {
      return 0;
    }

    // Recover level and gridIdx
    unsigned int level, gridIdx;
    input->ComputeIndexPair(blockIdx, level, gridIdx);
    vtkCartesianGrid* inputGrid = input->GetDataSetAsCartesianGrid(level, gridIdx);
    if (!inputGrid || !isLocal[blockIdx])
    {
      continue;
    }

    if (level == input->GetNumberOfLevels() - 1)
    {
      // in that case its a single part algorithm
      this->ProgressCeiling = (blockIdx + 1) / static_cast<double>(input->GetNumberOfBlocks());

      // ==== If the grid is refined, just add it to the output ====
      output->SetPartition(blockIdx, inputGrid);
      continue;
    }

    // ==== If the grid is non-refined, identify each cell in the interface of this grid ====

    // Recover current grid and shallow copy it
    vtkSmartPointer<vtkCartesianGrid> grid =
      vtkSmartPointer<vtkCartesianGrid>::Take(inputGrid->NewInstance());
    grid->ShallowCopy(inputGrid);

    // Deep copy the cell ghosts and remove them so the cells can be accessed
    vtkUnsignedCharArray* inputCellGhostArray = grid->GetCellGhostArray();
    vtkNew<vtkUnsignedCharArray> cellGhostArray;
    cellGhostArray->DeepCopy(inputCellGhostArray);
    grid->GetCellData()->RemoveArray(inputCellGhostArray->GetName());

    // Remove the point ghost array for performance reason
    grid->GetPointData()->RemoveArray(grid->GetPointData()->GhostArrayName());

    // Iterate on each cell and identify interface layer of cells
    std::set<vtkIdType> interfaceCells;

    // This edge maps lets us quickly analyse if a face is refined, non-refined or interface
    // key: two index of points in input
    // value: a vector of AMRPoint_t alongside the edge, in the correct order
    std::map<std::array<vtkIdType, 2>, std::vector<::AMRPoint_t>> blockEdgesMap;
    std::map<std::array<vtkIdType, 2>, std::vector<::AMRPoint_t>> interfaceEdgesMap;

    for (vtkIdType cellIdx = 0; cellIdx < grid->GetNumberOfCells(); cellIdx++)
    {
      // Check if cell is already hidden
      if (!(cellGhostArray->GetValue(cellIdx) &
            (vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL)))
      {
        std::map<std::array<vtkIdType, 2>, std::vector<::AMRPoint_t>> cellEdgesMap;

        // Recover the cell and its point ids
        vtkVoxel* voxel = vtkVoxel::SafeDownCast(inputGrid->GetCell(cellIdx));
        assert(voxel);
        vtkNew<vtkIdList> cellPointIds;
        inputGrid->GetCellPoints(cellIdx, cellPointIds);

        // === Iterate over each edge to check it is an interface cell, fill the edges map ===

        bool interfaceFlag = false;
        for (int edgeId = 0; edgeId < voxel->GetNumberOfEdges(); edgeId++)
        {
          // Recover an edge composed of input grid point ids
          const vtkIdType* localEdgeIds;
          voxel->GetEdgePoints(edgeId, localEdgeIds);
          std::array<vtkIdType, 2> edge{ cellPointIds->GetId(localEdgeIds[0]),
            cellPointIds->GetId(localEdgeIds[1]) };

          std::vector<::AMRPoint_t> edgeAMRPoints;

          // Check if edge have already been analysed
          auto edgeIt = blockEdgesMap.find(edge);
          if (edgeIt == blockEdgesMap.end())
          {
            // Recover edge points and center
            unsigned int edgeLevel, edgeGridId;
            double edgePoints[2][3];
            bool refinedEdgeFlag = true;
            double edgeCenter[3] = { 0, 0, 0 };

            for (int i = 0; i < 2; i++)
            {
              inputGrid->GetPoint(edge[i], edgePoints[i]);
              for (int j = 0; j < 3; j++)
              {
                edgeCenter[j] += edgePoints[i][j];
              }

              // Check if edge point is refined
              if (!input->FindGrid(edgePoints[i], edgeLevel, edgeGridId))
              {
                // This can happen if the AMR is invalid, eg. if a higher refinement level is bigger
                // bounds than a lower refinement level
                vtkErrorMacro("Could not find a grid for an edge point, this is unexpected and "
                              "unrecoverable and input AMR is probably invalid, aborting");
                return 0;
              }

              if (edgeLevel > level)
              {
                // Edge is refined
                interfaceFlag = true;
                vtkCartesianGrid* refinedGrid =
                  input->GetDataSetAsCartesianGrid(edgeLevel, edgeGridId);

                if (!refinedGrid)
                {
                  // This can happen if the AMR declare higher refinement that are not available as
                  // actual grids, which is invalid
                  vtkErrorMacro("A grid is missing a refined level despite available in its meta "
                                "data, this data is invalid, aborting.");
                  return 0;
                }

                // Add the refined point to the vector of points
                vtkIdType refinedPointId = refinedGrid->FindPoint(edgePoints[i]);
                edgeAMRPoints.emplace_back(AMRPoint_t{ edgeLevel, edgeGridId, refinedPointId });
              }
              else
              {
                // Edge is not refined, add the point as is to the vector of points
                refinedEdgeFlag = false;
                edgeAMRPoints.emplace_back(AMRPoint_t{ level, gridIdx, edge[i] });
              }
            }

            // Check if edge center point is refined
            for (int j = 0; j < 3; j++)
            {
              edgeCenter[j] /= 2;
            }
            if (!input->FindGrid(edgeCenter, edgeLevel, edgeGridId))
            {
              vtkWarningMacro("Could not find a grid for an edge center point, this is unexpected, "
                              "skipping edge");
              continue;
            }
            if (edgeLevel == level)
            {
              // edge center is not refined, no need to look for intermediary points
              // this can happen when a non-refined cell if between two refined grid
              refinedEdgeFlag = false;
            }

            if (refinedEdgeFlag)
            {
              // Recover all refined points on the edge
              if (!::InsertIntermediaryAMRPoints(
                    input, inputGrid, edge, edgeAMRPoints, warnedWatertight))
              {
                vtkErrorMacro("Could not insert intermediary points, aborting");
                return 0;
              }
            }

            // Emplace inverted as well as the edge points are not ordered
            cellEdgesMap.emplace(edge, edgeAMRPoints);
            cellEdgesMap.emplace(std::array<vtkIdType, 2>{ edge[1], edge[0] }, edgeAMRPoints);
          }
          else
          {
            // If edge has already been analysed, just recover it
            if (edgeIt->second.front().level > level || edgeIt->second.back().level > level)
            {
              interfaceFlag = true;
            }

            // Add it to the cell edges map, so it actually goes into the interface edge maps later
            cellEdgesMap.emplace(edge, edgeIt->second);
            cellEdgesMap.emplace(std::array<vtkIdType, 2>{ edge[1], edge[0] }, edgeIt->second);
          }
        }

        // Copy and add cell edges into the block map
        std::map<std::array<vtkIdType, 2>, std::vector<::AMRPoint_t>> cellEdgesMapCopy =
          cellEdgesMap;
        blockEdgesMap.merge(cellEdgesMapCopy);

        if (interfaceFlag)
        {
          // Add to the interface cells and edges for later usages
          interfaceCells.emplace(cellIdx);
          interfaceEdgesMap.merge(cellEdgesMap);
        }
      }
    }

    // Add ghost array back and blank the interface layer
    grid->GetCellData()->AddArray(cellGhostArray);
    for (vtkIdType cellIdx : interfaceCells)
    {
      grid->BlankCell(cellIdx);
    }

    // ==== Then add the modified grid to the output  ====
    output->SetPartition(blockIdx, grid);
    this->ProgressFloor = this->ProgressFloor + progressPart;
    this->UpdateProgress(this->ProgressFloor);
    if (this->CheckAbort())
    {
      return 0;
    }

    // ===== 2. For each non-refined grid create an interface layer =====

    // ==== Initalize the interface ====

    // Construct an unstructured grid of the interface for this grid
    vtkNew<vtkUnstructuredGrid> interface;
    vtkPointData* interfacePD = interface->GetPointData();
    vtkCellData* interfaceCD = interface->GetCellData();

    // Initial memory allocation, will be increased by insertion
    interfacePD->CopyAllocate(inputGrid->GetPointData(), inputGrid->GetNumberOfPoints());
    interfaceCD->CopyAllocate(inputGrid->GetCellData(), inputGrid->GetNumberOfCells());

    // Create point and point locator to insert unique points
    vtkNew<vtkPoints> interfacePoints;
    vtkNew<vtkMergePoints> interfacePointsLoc;
    interfacePointsLoc->InitPointInsertion(interfacePoints, grid->GetBounds());

    // Create cells to insert pyramids and tetrahedrons into
    vtkNew<vtkCellArray> outputInterfaceCells;
    vtkNew<vtkUnsignedCharArray> outputInterfaceCellTypes;

    // ==== Iterate over each cell to construct the interface cells
    for (vtkIdType cellIdx : interfaceCells)
    {
      // === Prepare for cell creation ===

      vtkVoxel* voxel = vtkVoxel::SafeDownCast(inputGrid->GetCell(cellIdx));
      assert(voxel);
      vtkNew<vtkIdList> cellPointIds;
      inputGrid->GetCellPoints(cellIdx, cellPointIds);

      // Recover the center of the voxel
      double center[3];
      {
        int subId;
        constexpr double pcoords[3] = { 0.5, 0.5, 0.5 };
        double weights[8];
        voxel->EvaluateLocation(subId, pcoords, center, weights);
      }

      // Add it to the interface, point data will be interpolated later using
      // centerInterpolateOutIds
      vtkIdType centerId;
      interfacePointsLoc->InsertUniquePoint(center, centerId);
      vtkNew<vtkIdList> centerInterpolateOutIds;

      // Iterate over each face of the cell
      for (vtkIdType faceIdx = 0; faceIdx < 6; faceIdx++)
      {
        const vtkIdType* localFaceIds;
        voxel->GetFacePoints(faceIdx, localFaceIds);
        vtkNew<vtkIdList> facePointIds;
        facePointIds->SetNumberOfIds(4);
        constexpr std::array<int, 4> inderFaceId{ 0, 1, 3, 2 };
        for (int i = 0; i < 4; i++)
        {
          facePointIds->SetId(i, cellPointIds->GetId(localFaceIds[inderFaceId[i]]));
        }

        // Recover face points
        double facePoints[4][3];
        for (int i = 0; i < 4; i++)
        {
          inputGrid->GetPoint(facePointIds->GetId(i), facePoints[i]);
        }

        // Find face center
        double faceCenter[3] = { 0, 0, 0 };
        for (int i = 0; i < 3; i++)
        {
          for (vtkIdType pointIdx = 0; pointIdx < 4; pointIdx++)
          {
            faceCenter[i] += facePoints[pointIdx][i];
          }
          faceCenter[i] /= 4.;
        }

        // === For each face, check if its a refined face, non-refined face or interface face ===

        // Count the number of split edges
        int nSplit = 0;
        for (int i = 0; i < 4; i++)
        {
          std::array<vtkIdType, 2> edge{ facePointIds->GetId(i), facePointIds->GetId((i + 1) % 4) };

          // this indexed access cannot fail
          assert(interfaceEdgesMap.find(edge) != interfaceEdgesMap.end());
          int nEdgeSplits = static_cast<int>(interfaceEdgesMap.at(edge).size());
          if (nEdgeSplits > 2)
          {
            nSplit++;
          }
        }

        // Identify the case based on number of splits
        bool done = false;
        if (nSplit == 4)
        {
          // All edges are split, check if FaceCenter is refined too
          // In the unlikely case that it is not, this will be handled by the interface face case
          unsigned int faceLevel, faceGridId;
          input->FindGrid(faceCenter, faceLevel, faceGridId);
          if (level < faceLevel)
          {
            // == This is a fully refined face, add internal refined points and create pyramids ==
            if (!::GenerateFacePyramids(input, inputGrid, cellIdx, centerId,
                  centerInterpolateOutIds, interfaceEdgesMap, facePointIds, interfacePointsLoc,
                  interfacePD, outputInterfaceCells, outputInterfaceCellTypes, interfaceCD))
            {
              return 0;
            }
            done = true;
          }
        }
        else if (nSplit == 0)
        {
          // == this face is facing a non refined grid, create a single pyramid ==

          // Recover face extent
          int ijkMin[3] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
            std::numeric_limits<int>::max() };
          int ijkMax[3] = { 0, 0, 0 };
          for (int i = 0; i < 4; i++)
          {
            int ijk[3];
            vtkStructuredData::ComputePointStructuredCoordsForExtent(
              facePointIds->GetId(i), inputGrid->GetExtent(), ijk);
            for (int j = 0; j < 3; j++)
            {
              ijkMin[j] = std::min(ijk[j], ijkMin[j]);
              ijkMax[j] = std::max(ijk[j], ijkMax[j]);
            }
          }

          // Generate a single pyramid
          ::GenerateFacePyramids(inputGrid, cellIdx, centerId, centerInterpolateOutIds, inputGrid,
            ijkMin, ijkMax, interfacePointsLoc, interfacePD, outputInterfaceCells,
            outputInterfaceCellTypes, interfaceCD);
          done = true;
        }

        if (!done)
        {
          // == This is an interface face, create tetrahedron to fill the space
          ::GenerateInterfaceTetrahedrons(input, inputGrid, interfaceEdgesMap, cellIdx, centerId,
            faceCenter, facePointIds, centerInterpolateOutIds, interfacePointsLoc, interfacePoints,
            interfacePD, outputInterfaceCells, outputInterfaceCellTypes, interfaceCD);
        }
      }

      // = Interpolate data on the center =

      // Interpolate point data on the center using a simple sheperd interpolation
      std::vector<double> vecWeights(centerInterpolateOutIds->GetNumberOfIds());
      double runningDist = 0;
      for (int i = 0; i < centerInterpolateOutIds->GetNumberOfIds(); i++)
      {
        // Distance cannot be 0
        vecWeights[i] = 1 /
          std::sqrt(vtkMath::Distance2BetweenPoints(
            center, interfacePoints->GetPoint(centerInterpolateOutIds->GetId(i))));
        runningDist += vecWeights[i];
      }
      std::transform(vecWeights.begin(), vecWeights.end(), vecWeights.begin(),
        [runningDist](double dist) { return dist / runningDist; });
      interfacePD->InterpolatePoint(
        interfacePD, centerId, centerInterpolateOutIds, vecWeights.data());
    }

    // Report progress
    this->ProgressFloor = this->ProgressFloor + progressPart;
    this->UpdateProgress(this->ProgressFloor);
    if (this->CheckAbort())
    {
      return 0;
    }

    // ===== 3. Cleanup and add interface to the output =====

    // Cleanup and add cells and points
    interfacePD->Squeeze();
    interfaceCD->Squeeze();
    interface->SetCells(outputInterfaceCellTypes, outputInterfaceCells);
    interface->SetPoints(interfacePoints);

    // Add the interface to the output
    output->SetPartition(input->GetNumberOfBlocks() + blockIdx, interface);
  }

  this->CleanupOutput(output);
  this->UpdateProgress(1.0);

  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRInterfaceFilter::CleanupOutput(vtkPartitionedDataSet* output)
{
  std::vector<int> nonEmptyIndices;
  for (unsigned int i = 0; i < output->GetNumberOfPartitions(); i++)
  {
    vtkDataSet* ds = output->GetPartition(i);
    if (ds && ds->GetNumberOfPoints() > 0)
    {
      nonEmptyIndices.emplace_back(i);
    }
  }

  for (std::size_t i = 0; i < nonEmptyIndices.size(); i++)
  {
    output->SetPartition(static_cast<unsigned int>(i), output->GetPartition(nonEmptyIndices[i]));
  }
  output->SetNumberOfPartitions(static_cast<unsigned int>(nonEmptyIndices.size()));
}

VTK_ABI_NAMESPACE_END
