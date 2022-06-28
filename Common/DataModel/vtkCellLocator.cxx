/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCellLocator.h"

#include "vtkBox.h"
#include "vtkCellArray.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include <array>
#include <cmath>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellLocator);

//------------------------------------------------------------------------------
vtkCellLocator::vtkNeighborCells::vtkNeighborCells(const int size)
{
  this->Points->Allocate(3 * size);
}

//------------------------------------------------------------------------------
int vtkCellLocator::vtkNeighborCells::GetNumberOfNeighbors()
{
  return (this->Points->GetMaxId() + 1) / 3;
}

//------------------------------------------------------------------------------
void vtkCellLocator::vtkNeighborCells::Reset()
{
  this->Points->Reset();
}

//------------------------------------------------------------------------------
int* vtkCellLocator::vtkNeighborCells::GetPoint(int i)
{
  return this->Points->GetPointer(3 * i);
}

//------------------------------------------------------------------------------
int vtkCellLocator::vtkNeighborCells::InsertNextPoint(int* x)
{
  int id = this->Points->GetMaxId() + 3;
  this->Points->InsertValue(id, x[2]);
  this->Points->SetValue(id - 2, x[0]);
  this->Points->SetValue(id - 1, x[1]);
  return id / 3;
}

//------------------------------------------------------------------------------
// Construct with automatic computation of divisions, averaging
// 25 cells per bucket.
vtkCellLocator::vtkCellLocator()
{
  this->MaxLevel = 8;
  this->Level = 8;
  this->NumberOfCellsPerNode = 25;
  this->Tree = nullptr;
  this->NumberOfDivisions = 1;
  this->H[0] = this->H[1] = this->H[2] = 1.0;

  this->NumberOfOctants = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
  this->Tree = nullptr;
}

//------------------------------------------------------------------------------
vtkCellLocator::~vtkCellLocator()
{
  this->FreeSearchStructure();
  this->FreeCellBounds();
}

//------------------------------------------------------------------------------
void vtkCellLocator::FreeSearchStructure()
{
  if (this->Tree)
  {
    this->TreeSharedPtr.reset();
    this->Tree = nullptr;
  }
}

//------------------------------------------------------------------------------
// Given an offset into the structure, the number of divisions in the octree,
// an i,j,k location in the octree; return the index (idx) into the structure.
// Method returns 1 is the specified i,j,k location is "outside" of the octree.
int vtkCellLocator::GenerateIndex(int offset, int numDivs, int i, int j, int k, vtkIdType& idx)
{
  if (i < 0 || i >= numDivs || j < 0 || j >= numDivs || k < 0 || k >= numDivs)
  {
    return 1;
  }

  idx = offset + i + j * numDivs + k * numDivs * numDivs;

  return 0;
}

//------------------------------------------------------------------------------
void vtkCellLocator::ComputeOctantBounds(double octantBounds[6], int i, int j, int k)
{
  octantBounds[0] = this->Bounds[0] + i * H[0];
  octantBounds[1] = octantBounds[0] + H[0];
  octantBounds[2] = this->Bounds[2] + j * H[1];
  octantBounds[3] = octantBounds[2] + H[1];
  octantBounds[4] = this->Bounds[4] + k * H[2];
  octantBounds[5] = octantBounds[4] + H[2];
}

//------------------------------------------------------------------------------
void vtkCellLocator::GetBucketIndices(const double x[3], int ijk[3])
{
  ijk[0] = static_cast<int>((x[0] - this->Bounds[0]) / this->H[0]);
  ijk[1] = static_cast<int>((x[1] - this->Bounds[2]) / this->H[1]);
  ijk[2] = static_cast<int>((x[2] - this->Bounds[4]) / this->H[2]);

  ijk[0] =
    (ijk[0] < 0 ? 0 : (ijk[0] >= this->NumberOfDivisions ? this->NumberOfDivisions - 1 : ijk[0]));
  ijk[1] =
    (ijk[1] < 0 ? 0 : (ijk[1] >= this->NumberOfDivisions ? this->NumberOfDivisions - 1 : ijk[1]));
  ijk[2] =
    (ijk[2] < 0 ? 0 : (ijk[2] >= this->NumberOfDivisions ? this->NumberOfDivisions - 1 : ijk[2]));
}

//------------------------------------------------------------------------------
// Return intersection point (if any) AND the cell which was intersected by
// finite line.
int vtkCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return 0;
  }
  double* bounds = this->Bounds;
  double* h = this->H;
  double t0, t1, x0[3], x1[3], tHitCell;
  double hitCellBoundsPosition[3], cellBounds[6], *cellBoundsPtr;
  double octantBounds[6];
  int prod = this->NumberOfDivisions * this->NumberOfDivisions;
  vtkIdType cellIdBest = -1, cId, i, idx, numberOfCellsInBucket;
  int ijk[3], ijkEnd[3];
  int plane0, plane1, subIdBest = -1, hitCellBounds;
  double tBest = VTK_FLOAT_MAX, xBest[3], pCoordsBest[3], step[3], next[3], tMax[3], tDelta[3];
  double rayDir[3];
  vtkMath::Subtract(p2, p1, rayDir);
  int leafStart = this->NumberOfOctants -
    this->NumberOfDivisions * this->NumberOfDivisions * this->NumberOfDivisions;

  // Make sure the bounding box of the locator is hit. Also, determine the
  // entry and exit points into and out of the locator. This is used to
  // determine the bins where the ray starts and ends.
  cellId = -1;
  subId = 0;
  if (vtkBox::IntersectWithLine(bounds, p1, p2, t0, t1, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);

  // Get the i-j-k point of intersection and bin index. This is
  // clamped to the boundary of the locator.
  this->GetBucketIndices(x0, ijk);
  this->GetBucketIndices(x1, ijkEnd);
  idx = leafStart + ijk[0] + ijk[1] * this->NumberOfDivisions + ijk[2] * prod;

  // Set up some traversal parameters for traversing through bins
  step[0] = (rayDir[0] >= 0.0) ? 1.0 : -1.0;
  step[1] = (rayDir[1] >= 0.0) ? 1.0 : -1.0;
  step[2] = (rayDir[2] >= 0.0) ? 1.0 : -1.0;

  // If the ray is going in the negative direction, then the next voxel boundary
  // is on the "-" direction so we stay in the current voxel.
  next[0] = bounds[0] + h[0] * (rayDir[0] >= 0.0 ? (ijk[0] + step[0]) : ijk[0]);
  next[1] = bounds[2] + h[1] * (rayDir[1] >= 0.0 ? (ijk[1] + step[1]) : ijk[1]);
  next[2] = bounds[4] + h[2] * (rayDir[2] >= 0.0 ? (ijk[2] + step[2]) : ijk[2]);

  tMax[0] = (rayDir[0] != 0.0) ? (next[0] - x0[0]) / rayDir[0] : VTK_FLOAT_MAX;
  tMax[1] = (rayDir[1] != 0.0) ? (next[1] - x0[1]) / rayDir[1] : VTK_FLOAT_MAX;
  tMax[2] = (rayDir[2] != 0.0) ? (next[2] - x0[2]) / rayDir[2] : VTK_FLOAT_MAX;

  tDelta[0] = (rayDir[0] != 0.0) ? (h[0] / rayDir[0]) * step[0] : VTK_FLOAT_MAX;
  tDelta[1] = (rayDir[1] != 0.0) ? (h[1] / rayDir[1]) * step[1] : VTK_FLOAT_MAX;
  tDelta[2] = (rayDir[2] != 0.0) ? (h[2] / rayDir[2]) * step[2] : VTK_FLOAT_MAX;

  for (cellIdBest = (-1); cellIdBest < 0;)
  {
    if (this->Tree[idx] &&
      (numberOfCellsInBucket = this->Tree[idx]->GetNumberOfIds()) > 0) // there are some cell here
    {
      this->ComputeOctantBounds(octantBounds, ijk[0], ijk[1], ijk[2]);
      for (i = 0; i < numberOfCellsInBucket; ++i)
      {
        cId = this->Tree[idx]->GetId(i);
        if (!cellHasBeenVisited[cId])
        {
          cellHasBeenVisited[cId] = true;

          // check whether we intersect the cell bounds
          cellBoundsPtr = cellBounds;
          this->GetCellBounds(cId, cellBoundsPtr);
          hitCellBounds =
            vtkBox::IntersectBox(cellBoundsPtr, p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

          if (hitCellBounds)
          {
            // now, do the expensive GetCell call and the expensive
            // intersect with line call
            this->DataSet->GetCell(cId, cell);
            if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
            {
              // Make sure that intersection occurs within this octant or else spurious cell
              // intersections can occur behind this bin which are not the correct answer.
              if (!vtkAbstractCellLocator::IsInBounds(octantBounds, x, tol))
              {
                cellHasBeenVisited[cId] = false; // mark the cell non-visited
              }
              else
              {
                tBest = t;
                xBest[0] = x[0];
                xBest[1] = x[1];
                xBest[2] = x[2];
                pCoordsBest[0] = pcoords[0];
                pCoordsBest[1] = pcoords[1];
                pCoordsBest[2] = pcoords[2];
                subIdBest = subId;
                cellIdBest = cId;
              } // intersection point is in current octant
            }   // if intersection
          }     // if (hitCellBounds)
        }       // if (!CellHasBeenVisited[cId])
      }
    }

    // Exit before end of ray, saves a few cycles
    if (cellIdBest >= 0)
    {
      break;
    }

    // See if the traversal is complete (reached the end of the line).
    if (ijk[0] == ijkEnd[0] && ijk[1] == ijkEnd[1] && ijk[2] == ijkEnd[2])
    {
      break;
    }

    // move to the next octant
    // Advance to next voxel
    if (tMax[0] < tMax[1])
    {
      if (tMax[0] < tMax[2])
      {
        ijk[0] += static_cast<int>(step[0]);
        tMax[0] += tDelta[0];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }
    else
    {
      if (tMax[1] < tMax[2])
      {
        ijk[1] += static_cast<int>(step[1]);
        tMax[1] += tDelta[1];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }

    if (ijk[0] < 0 || ijk[0] >= this->NumberOfDivisions || ijk[1] < 0 ||
      ijk[1] >= this->NumberOfDivisions || ijk[2] < 0 || ijk[2] >= this->NumberOfDivisions)
    {
      break;
    }
    else
    {
      idx = leafStart + ijk[0] + ijk[1] * this->NumberOfDivisions + ijk[2] * prod;
    }
  }

  // If a cell has been intersected, recover the information and return.
  if (cellIdBest >= 0)
  {
    this->DataSet->GetCell(cellIdBest, cell);
    t = tBest;
    x[0] = xBest[0];
    x[1] = xBest[1];
    x[2] = xBest[2];
    pcoords[0] = pCoordsBest[0];
    pcoords[1] = pCoordsBest[1];
    pcoords[2] = pCoordsBest[2];
    subId = subIdBest;
    cellId = cellIdBest;
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellLocator::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2,
  int& inside)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return 0;
  }
  int i;
  vtkIdType j;
  int tmpInside;
  int* nei;
  int closestSubCell = -1;
  int leafStart;
  int ijk[3];
  double pcoords[3], point[3], cachedPoint[3];
  size_t nPoints;
  int returnVal = 0;
  vtkIdList* cellIds;
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);
  vtkNeighborCells buckets(10);
  std::vector<double> weights(8);

  double distance2ToBucket;
  double distance2ToCellBounds, cellBounds[6], *cellBoundsPtr, currentRadius;
  cellBoundsPtr = cellBounds;
  double distance2ToDataBounds, maxDistance;
  int ii, radiusLevels[3], radiusLevel, prevMinLevel[3], prevMaxLevel[3];

  cachedPoint[0] = 0.0;
  cachedPoint[1] = 0.0;
  cachedPoint[2] = 0.0;

  leafStart = this->NumberOfOctants -
    this->NumberOfDivisions * this->NumberOfDivisions * this->NumberOfDivisions;

  // init
  dist2 = -1.0;
  vtkIdType closestCell = -1;
  double radius2 = radius * radius;
  double minDist2 = 1.1 * radius2; // something slightly bigger....
  double refinedRadius;
  double refinedRadius2 = radius2;

  // Find bucket point is in.
  this->GetBucketIndices(x, ijk);

  // Start by searching the bucket that the point is in.
  //
  if ((cellIds = this->Tree[leafStart + ijk[0] + ijk[1] * this->NumberOfDivisions +
         ijk[2] * this->NumberOfDivisions * this->NumberOfDivisions]) != nullptr)
  {
    // query each cell
    for (j = 0; j < cellIds->GetNumberOfIds(); j++)
    {
      // get the cell
      cellId = cellIds->GetId(j);
      // skip if it has been visited
      if (cellHasBeenVisited[cellId])
      {
        continue;
      }
      cellHasBeenVisited[cellId] = true;

      // check whether we could be close enough to the cell by
      this->GetCellBounds(cellId, cellBoundsPtr);
      // testing the cell bounds
      distance2ToCellBounds = this->Distance2ToBounds(x, cellBoundsPtr);

      if (distance2ToCellBounds < refinedRadius2)
      {
        this->DataSet->GetCell(cellId, cell);

        // make sure we have enough storage space for the weights
        nPoints = static_cast<size_t>(cell->GetPointIds()->GetNumberOfIds());
        if (weights.size() < nPoints)
        {
          weights.resize(2 * nPoints);
        }

        // evaluate the position to find the closest point
        tmpInside = cell->EvaluatePosition(x, point, subId, pcoords, dist2, weights.data());
        if (dist2 < minDist2)
        {
          inside = tmpInside;
          closestCell = cellId;
          closestSubCell = subId;
          minDist2 = dist2;
          cachedPoint[0] = point[0];
          cachedPoint[1] = point[1];
          cachedPoint[2] = point[2];
          refinedRadius2 = dist2;
        }
      }
    }
  }

  // Now, search only those buckets that are within a radius. The radius used
  // is the minimum of std::sqrt(dist2) and the radius that is passed in. To avoid
  // checking a large number of buckets unnecessarily, if the radius is
  // larger than the dimensions of a bucket, we search outward using a
  // simple heuristic of rings.  This heuristic ends up collecting inner
  // buckets multiple times, but this only happens in the case where these
  // buckets are empty, so they are discarded quickly.
  //
  if (dist2 < radius2 && dist2 >= 0.0)
  {
    refinedRadius = std::sqrt(dist2);
    refinedRadius2 = dist2;
  }
  else
  {
    refinedRadius = radius;
    refinedRadius2 = radius2;
  }

  distance2ToDataBounds = this->Distance2ToBounds(x, this->Bounds);
  maxDistance = std::sqrt(distance2ToDataBounds) + this->DataSet->GetLength();
  if (refinedRadius > maxDistance)
  {
    refinedRadius = maxDistance;
    refinedRadius2 = maxDistance * maxDistance;
  }

  radiusLevels[0] = static_cast<int>(refinedRadius / this->H[0]);
  radiusLevels[1] = static_cast<int>(refinedRadius / this->H[1]);
  radiusLevels[2] = static_cast<int>(refinedRadius / this->H[2]);

  radiusLevel = radiusLevels[0];
  radiusLevel = radiusLevels[1] > radiusLevel ? radiusLevels[1] : radiusLevel;
  radiusLevel = radiusLevels[2] > radiusLevel ? radiusLevels[2] : radiusLevel;

  if (radiusLevel > this->NumberOfDivisions / 2)
  {
    radiusLevel = this->NumberOfDivisions / 2;
  }
  if (radiusLevel == 0)
  {
    radiusLevel = 1;
  }

  // radius schedule increases the radius each iteration, this is currently
  // implemented by decreasing ii by 1 each iteration.  another alternative
  // is to double the radius each iteration, i.e. ii = ii >> 1
  // In practice, reducing ii by one has been found to be more efficient.
  int numberOfBucketsPerPlane;
  numberOfBucketsPerPlane = this->NumberOfDivisions * this->NumberOfDivisions;
  prevMinLevel[0] = prevMaxLevel[0] = ijk[0];
  prevMinLevel[1] = prevMaxLevel[1] = ijk[1];
  prevMinLevel[2] = prevMaxLevel[2] = ijk[2];
  for (ii = radiusLevel; ii >= 1; ii--)
  {
    currentRadius = refinedRadius; // used in if at bottom of this for loop

    // Build up a list of buckets that are arranged in rings
    this->GetOverlappingBuckets(buckets, x, refinedRadius / ii, prevMinLevel, prevMaxLevel);

    for (i = 0; i < buckets.GetNumberOfNeighbors(); i++)
    {
      nei = buckets.GetPoint(i);

      if ((cellIds = this->Tree[leafStart + nei[0] + nei[1] * this->NumberOfDivisions +
             nei[2] * numberOfBucketsPerPlane]) != nullptr)
      {
        // do we still need to test this bucket?
        distance2ToBucket = this->Distance2ToBucket(x, nei);

        if (distance2ToBucket < refinedRadius2)
        {
          // still a viable bucket
          for (j = 0; j < cellIds->GetNumberOfIds(); j++)
          {
            // get the cell
            cellId = cellIds->GetId(j);
            // skip if it has been visited
            if (cellHasBeenVisited[cellId])
            {
              continue;
            }
            cellHasBeenVisited[cellId] = true;

            // check whether we could be close enough to the cell by
            this->GetCellBounds(cellId, cellBoundsPtr);
            // testing the cell bounds
            distance2ToCellBounds = this->Distance2ToBounds(x, cellBoundsPtr);

            if (distance2ToCellBounds < refinedRadius2)
            {
              this->DataSet->GetCell(cellId, cell);

              // make sure we have enough storage space for the weights
              nPoints = static_cast<size_t>(cell->GetPointIds()->GetNumberOfIds());
              if (weights.size() < nPoints)
              {
                weights.resize(2 * nPoints);
              }

              // evaluate the position to find the closest point
              tmpInside = cell->EvaluatePosition(x, point, subId, pcoords, dist2, weights.data());

              if (dist2 < minDist2)
              {
                inside = tmpInside;
                closestCell = cellId;
                closestSubCell = subId;
                minDist2 = dist2;
                cachedPoint[0] = point[0];
                cachedPoint[1] = point[1];
                cachedPoint[2] = point[2];
                refinedRadius = std::sqrt(minDist2);
                refinedRadius2 = minDist2;
              }
            } // if point close enough to cell bounds
          }   // for each cell in bucket
        }     // if bucket is within the current best distance
      }       // if cells in bucket
    }         // for each overlapping bucket

    // don't want to checker a smaller radius than we just checked so update
    // ii appropriately
    if (refinedRadius < currentRadius && ii > 2) // always check ii==1
    {
      ii = static_cast<int>(static_cast<double>(ii) * (refinedRadius / currentRadius)) + 1;
      if (ii < 2)
      {
        ii = 2;
      }
    }
  } // for each radius in the radius schedule

  if ((closestCell != -1) && (minDist2 <= radius2))
  {
    dist2 = minDist2;
    cellId = closestCell;
    subId = closestSubCell;
    closestPoint[0] = cachedPoint[0];
    closestPoint[1] = cachedPoint[1];
    closestPoint[2] = cachedPoint[2];
    this->DataSet->GetCell(cellId, cell);
    returnVal = 1;
  }

  return returnVal;
}

//------------------------------------------------------------------------------
// Internal method to find those buckets that are within distance specified.
// Only those buckets outside of level radiuses of ijk are returned. The
// bucket neighbors are indices into the "leaf-node" layer of the octree.
// These indices must be offset by number of octants before the leaf node
// layer before they can be used. Only buckets that have cells are placed
// in the bucket list.
void vtkCellLocator::GetOverlappingBuckets(vtkNeighborCells& buckets, const double x[3],
  double dist, int prevMinLevel[3], int prevMaxLevel[3])
{
  int i, j, k, nei[3], minLevel[3], maxLevel[3];
  int leafStart, kFactor, jFactor;
  int numberOfBucketsPerPlane, jkSkipFlag, kSkipFlag;

  numberOfBucketsPerPlane = this->NumberOfDivisions * this->NumberOfDivisions;
  leafStart = this->NumberOfOctants - numberOfBucketsPerPlane * this->NumberOfDivisions;

  // Initialize
  buckets.Reset();

  // Determine the range of indices in each direction
  for (i = 0; i < 3; i++)
  {
    minLevel[i] =
      static_cast<int>(static_cast<double>(((x[i] - dist) - this->Bounds[2 * i]) / this->H[i]));
    maxLevel[i] =
      static_cast<int>(static_cast<double>(((x[i] + dist) - this->Bounds[2 * i]) / this->H[i]));

    if (minLevel[i] < 0)
    {
      minLevel[i] = 0;
    }
    else if (minLevel[i] >= this->NumberOfDivisions)
    {
      minLevel[i] = this->NumberOfDivisions - 1;
    }
    if (maxLevel[i] >= this->NumberOfDivisions)
    {
      maxLevel[i] = this->NumberOfDivisions - 1;
    }
    else if (maxLevel[i] < 0)
    {
      maxLevel[i] = 0;
    }
  }

  if (minLevel[0] == prevMinLevel[0] && maxLevel[0] == prevMaxLevel[0] &&
    minLevel[1] == prevMinLevel[1] && maxLevel[1] == prevMaxLevel[1] &&
    minLevel[2] == prevMinLevel[2] && maxLevel[2] == prevMaxLevel[2])
  {
    return;
  }

  for (k = minLevel[2]; k <= maxLevel[2]; k++)
  {
    kFactor = k * numberOfBucketsPerPlane;
    if (k >= prevMinLevel[2] && k <= prevMaxLevel[2])
    {
      kSkipFlag = 1;
    }
    else
    {
      kSkipFlag = 0;
    }
    for (j = minLevel[1]; j <= maxLevel[1]; j++)
    {
      if (kSkipFlag && j >= prevMinLevel[1] && j <= prevMaxLevel[1])
      {
        jkSkipFlag = 1;
      }
      else
      {
        jkSkipFlag = 0;
      }
      jFactor = j * this->NumberOfDivisions;
      for (i = minLevel[0]; i <= maxLevel[0]; i++)
      {
        if (jkSkipFlag && i == prevMinLevel[0])
        {
          i = prevMaxLevel[0];
          continue;
        }
        // if this bucket has any cells, add it to the list
        if (this->Tree[leafStart + i + jFactor + kFactor])
        {
          nei[0] = i;
          nei[1] = j;
          nei[2] = k;
          buckets.InsertNextPoint(nei);
        }
      }
    }
  }

  prevMinLevel[0] = minLevel[0];
  prevMinLevel[1] = minLevel[1];
  prevMinLevel[2] = minLevel[2];
  prevMaxLevel[0] = maxLevel[0];
  prevMaxLevel[1] = maxLevel[1];
  prevMaxLevel[2] = maxLevel[2];
}

//------------------------------------------------------------------------------
// number of buckets available
int vtkCellLocator::GetNumberOfBuckets()
{
  if (this->Tree)
  {
    return this->NumberOfOctants;
  }
  else
  {
    vtkWarningMacro(<< "Attempting to access Tree before Locator has been built");
    return 0;
  }
}

//------------------------------------------------------------------------------
// Get the cells in a bucket.
vtkIdList* vtkCellLocator::GetCells(int octantId)
{
  // handle parents ?
  return this->Tree[octantId];
}

//------------------------------------------------------------------------------
void vtkCellLocator::BuildLocator()
{
  // don't rebuild if build time is newer than modified and dataset modified time
  if (this->Tree && this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
  {
    return;
  }
  // don't rebuild if UseExistingSearchStructure is ON and a search structure already exists
  if (this->Tree && this->UseExistingSearchStructure)
  {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkCellLocator::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
//  Method to form subdivision of space based on the cells provided and
//  subject to the constraints of levels and NumberOfCellsPerNode.
//  The result is directly addressable and of uniform subdivision.
void vtkCellLocator::BuildLocatorInternal()
{
  double length, cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  vtkIdType numCells;
  int ndivs, product;
  int i, j, k, ijkMin[3], ijkMax[3];
  vtkIdType cellId, idx;
  int parentOffset;
  vtkSmartPointer<vtkIdList> octant;
  int numCellsPerBucket = this->NumberOfCellsPerNode;
  int prod, numOctants;
  double hTol[3];

  vtkDebugMacro(<< "Subdividing octree...");

  if (!this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1)
  {
    vtkErrorMacro(<< "No cells to subdivide");
    return;
  }
  this->DataSet->ComputeBounds();

  //  Make sure the appropriate data is available
  this->FreeSearchStructure();

  //  Size the root cell.  Initialize cell data structure, compute
  //  level and divisions.
  const double* bounds = this->DataSet->GetBounds();
  length = this->DataSet->GetLength();
  for (i = 0; i < 3; i++)
  {
    this->Bounds[2 * i] = bounds[2 * i];
    this->Bounds[2 * i + 1] = bounds[2 * i + 1];
    if ((this->Bounds[2 * i + 1] - this->Bounds[2 * i]) <= (length / 1000.0))
    {
      // bump out the bounds a little of if min==max
      this->Bounds[2 * i] -= length / 100.0;
      this->Bounds[2 * i + 1] += length / 100.0;
    }
  }

  if (this->Automatic)
  {
    this->Level =
      static_cast<int>(std::ceil(std::log(static_cast<double>(numCells) / numCellsPerBucket) /
        (std::log(static_cast<double>(8.0)))));
  }
  this->Level = (this->Level > this->MaxLevel ? this->MaxLevel : this->Level);

  // compute number of octants and number of divisions
  for (ndivs = 1, prod = 1, numOctants = 1, i = 0; i < this->Level; i++)
  {
    ndivs *= 2;
    prod *= 8;
    numOctants += prod;
  }
  this->NumberOfDivisions = ndivs;
  this->NumberOfOctants = numOctants;

  this->TreeSharedPtr =
    std::make_shared<std::vector<vtkSmartPointer<vtkIdList>>>(numOctants, nullptr);
  this->Tree = TreeSharedPtr->data();

  this->ComputeCellBounds();

  //  Compute width of leaf octant in three directions
  for (i = 0; i < 3; i++)
  {
    this->H[i] = (this->Bounds[2 * i + 1] - this->Bounds[2 * i]) / ndivs;
    hTol[i] = this->H[i] / 100.0;
  }

  //  Insert each cell into the appropriate octant.  Make sure cell
  //  falls within octant.
  parentOffset = numOctants - (ndivs * ndivs * ndivs);
  product = ndivs * ndivs;
  auto parentOctant = vtkSmartPointer<vtkIdList>::New(); // This is just a place-holder for parents
  for (cellId = 0; cellId < numCells; cellId++)
  {
    this->GetCellBounds(cellId, cellBoundsPtr);

    // find min/max locations of bounding box
    for (i = 0; i < 3; i++)
    {
      ijkMin[i] =
        static_cast<int>((cellBoundsPtr[2 * i] - this->Bounds[2 * i] - hTol[i]) / this->H[i]);
      ijkMax[i] =
        static_cast<int>((cellBoundsPtr[2 * i + 1] - this->Bounds[2 * i] + hTol[i]) / this->H[i]);

      if (ijkMin[i] < 0)
      {
        ijkMin[i] = 0;
      }
      if (ijkMax[i] >= ndivs)
      {
        ijkMax[i] = ndivs - 1;
      }
    }

    // each octant between min/max point may have cell in it
    for (k = ijkMin[2]; k <= ijkMax[2]; k++)
    {
      for (j = ijkMin[1]; j <= ijkMax[1]; j++)
      {
        for (i = ijkMin[0]; i <= ijkMax[0]; i++)
        {
          idx = parentOffset + i + j * ndivs + k * product;
          this->MarkParents(parentOctant, i, j, k, ndivs, this->Level);
          octant = this->Tree[idx];
          if (!octant)
          {
            octant = vtkSmartPointer<vtkIdList>::New();
            octant->Allocate(numCellsPerBucket, numCellsPerBucket / 2);
            this->Tree[idx] = octant;
          }
          octant->InsertNextId(cellId);
        }
      }
    }
  } // for all cells

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkCellLocator::MarkParents(
  const vtkSmartPointer<vtkIdList>& parentOctant, int i, int j, int k, int ndivs, int level)
{
  int offset, prod, ii;
  vtkIdType parentIdx;

  for (offset = 0, prod = 1, ii = 0; ii < level - 1; ii++)
  {
    offset += prod;
    prod = prod << 3;
  }

  while (level > 0)
  {
    i = i >> 1;
    j = j >> 1;
    k = k >> 1;
    ndivs = ndivs >> 1;
    level--;

    parentIdx = offset + i + j * ndivs + k * ndivs * ndivs;

    // if it already matches just return
    if (parentOctant == this->Tree[parentIdx])
    {
      return;
    }

    this->Tree[parentIdx] = parentOctant;

    prod = prod >> 3;
    offset -= prod;
  }
}

//------------------------------------------------------------------------------
void vtkCellLocator::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return;
  }
  vtkPoints* pts;
  vtkCellArray* polys;
  int l, i, j, k, ii, boundary[3];
  vtkIdType idx = 0;
  vtkIdList *inside, *Inside[3] = { nullptr, nullptr, nullptr };
  int numDivs = 1;

  pts = vtkPoints::New();
  pts->Allocate(5000);
  polys = vtkCellArray::New();
  polys->AllocateEstimate(4096, 3);

  // Compute idx into tree at appropriate level; determine if
  // faces of octants are visible.
  //
  int parentIdx = 0;
  int numOctants = 1;

  if (level < 0)
  {
    level = this->Level;
  }
  for (l = 0; l < level; l++)
  {
    numDivs *= 2;
    parentIdx += numOctants;
    numOctants *= 8;
  }

  // loop over all octants generating visible faces
  for (k = 0; k < numDivs; k++)
  {
    for (j = 0; j < numDivs; j++)
    {
      for (i = 0; i < numDivs; i++)
      {
        this->GenerateIndex(parentIdx, numDivs, i, j, k, idx);
        inside = this->Tree[idx];

        if (!(boundary[0] = this->GenerateIndex(parentIdx, numDivs, i - 1, j, k, idx)))
        {
          Inside[0] = this->Tree[idx];
        }
        if (!(boundary[1] = this->GenerateIndex(parentIdx, numDivs, i, j - 1, k, idx)))
        {
          Inside[1] = this->Tree[idx];
        }
        if (!(boundary[2] = this->GenerateIndex(parentIdx, numDivs, i, j, k - 1, idx)))
        {
          Inside[2] = this->Tree[idx];
        }

        for (ii = 0; ii < 3; ii++)
        {
          if (boundary[ii])
          {
            if (inside)
            {
              this->GenerateFace(ii, numDivs, i, j, k, pts, polys);
            }
          }
          else
          {
            if ((Inside[ii] && !inside) || (!Inside[ii] && inside))
            {
              this->GenerateFace(ii, numDivs, i, j, k, pts, polys);
            }
          }
          // those buckets on "positive" boundaries can generate faces specially
          if ((i + 1) >= numDivs && inside)
          {
            this->GenerateFace(0, numDivs, i + 1, j, k, pts, polys);
          }
          if ((j + 1) >= numDivs && inside)
          {
            this->GenerateFace(1, numDivs, i, j + 1, k, pts, polys);
          }
          if ((k + 1) >= numDivs && inside)
          {
            this->GenerateFace(2, numDivs, i, j, k + 1, pts, polys);
          }

        } // over negative faces
      }   // over i divisions
    }     // over j divisions
  }       // over k divisions

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

//------------------------------------------------------------------------------
void vtkCellLocator::GenerateFace(
  int face, int numDivs, int i, int j, int k, vtkPoints* pts, vtkCellArray* polys)
{
  int ii;
  vtkIdType ids[4];
  double origin[3], x[3];
  double h[3];

  // define first corner; use ids[] as temporary array
  ids[0] = i;
  ids[1] = j;
  ids[2] = k;
  for (ii = 0; ii < 3; ii++)
  {
    h[ii] = (this->Bounds[2 * ii + 1] - this->Bounds[2 * ii]) / numDivs;
    origin[ii] = this->Bounds[2 * ii] + ids[ii] * h[ii];
  }

  ids[0] = pts->InsertNextPoint(origin);

  if (face == 0) // x face
  {
    x[0] = origin[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2] + h[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + h[2];
    ids[3] = pts->InsertNextPoint(x);
  }

  else if (face == 1) // y face
  {
    x[0] = origin[0] + h[0];
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + h[0];
    x[1] = origin[1];
    x[2] = origin[2] + h[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1];
    x[2] = origin[2] + h[2];
    ids[3] = pts->InsertNextPoint(x);
  }

  else // z face
  {
    x[0] = origin[0] + h[0];
    x[1] = origin[1];
    x[2] = origin[2];
    ids[1] = pts->InsertNextPoint(x);

    x[0] = origin[0] + h[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2];
    ids[2] = pts->InsertNextPoint(x);

    x[0] = origin[0];
    x[1] = origin[1] + h[1];
    x[2] = origin[2];
    ids[3] = pts->InsertNextPoint(x);
  }

  polys->InsertNextCell(4, ids);
}

//------------------------------------------------------------------------------
// Calculate the distance between the point x to the bucket "nei".
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make is 25% slower!!!!
//
double vtkCellLocator::Distance2ToBucket(const double x[3], int nei[3])
{
  double bounds[6];

  bounds[0] = nei[0] * this->H[0] + this->Bounds[0];
  bounds[1] = (nei[0] + 1) * this->H[0] + this->Bounds[0];
  bounds[2] = nei[1] * this->H[1] + this->Bounds[2];
  bounds[3] = (nei[1] + 1) * this->H[1] + this->Bounds[2];
  bounds[4] = nei[2] * this->H[2] + this->Bounds[4];
  bounds[5] = (nei[2] + 1) * this->H[2] + this->Bounds[4];

  return this->Distance2ToBounds(x, bounds);
}

//------------------------------------------------------------------------------
// Calculate the distance between the point x and the specified bounds
//
// WARNING!!!!! Be very careful altering this routine.  Simple changes to this
// routine can make it 25% slower!!!!
double vtkCellLocator::Distance2ToBounds(const double x[3], double bounds[6])
{
  // Are we within the bounds?
  if (x[0] >= bounds[0] && x[0] <= bounds[1] && x[1] >= bounds[2] && x[1] <= bounds[3] &&
    x[2] >= bounds[4] && x[2] <= bounds[5])
  {
    return 0.0;
  }
  double deltas[3];
  deltas[0] = x[0] < bounds[0] ? bounds[0] - x[0] : (x[0] > bounds[1] ? x[0] - bounds[1] : 0.0);
  deltas[1] = x[1] < bounds[2] ? bounds[2] - x[1] : (x[1] > bounds[3] ? x[1] - bounds[3] : 0.0);
  deltas[2] = x[2] < bounds[4] ? bounds[4] - x[2] : (x[2] > bounds[5] ? x[2] - bounds[5] : 0.0);
  return vtkMath::SquaredNorm(deltas);
}

//------------------------------------------------------------------------------
vtkIdType vtkCellLocator::FindCell(double x[3], double vtkNotUsed(tol2), vtkGenericCell* cell,
  int& subId, double pcoords[3], double* weights)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return -1;
  }
  // check if x outside of bounds
  if (!vtkAbstractCellLocator::IsInBounds(this->Bounds, x))
  {
    return -1;
  }

  vtkIdList* cellIds;
  int ijk[3];
  double dist2;
  vtkIdType idx, cellId;

  int leafStart = this->NumberOfOctants -
    this->NumberOfDivisions * this->NumberOfDivisions * this->NumberOfDivisions;

  // Find bucket point is in.
  this->GetBucketIndices(x, ijk);

  // Search the bucket that the point is in.
  if ((cellIds = this->Tree[leafStart + ijk[0] + ijk[1] * this->NumberOfDivisions +
         ijk[2] * this->NumberOfDivisions * this->NumberOfDivisions]) != nullptr)
  {
    // query each cell
    for (idx = 0; idx < cellIds->GetNumberOfIds(); ++idx)
    {
      // get the cell
      cellId = cellIds->GetId(idx);
      // check whether we could be close enough to the cell by
      // testing the cell bounds
      if (this->InsideCellBounds(x, cellId))
      {
        this->DataSet->GetCell(cellId, cell);
        if (cell->EvaluatePosition(x, nullptr, subId, pcoords, dist2, weights) == 1)
        {
          return cellId;
        }
      }
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
void vtkCellLocator::FindCellsWithinBounds(double* bbox, vtkIdList* cells)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return;
  }
  if (!cells)
  {
    return;
  }
  cells->Reset();

  // Get the locator locations for the two extreme corners of the bounding box
  double p1[3], p2[3];
  p1[0] = bbox[0];
  p1[1] = bbox[2];
  p1[2] = bbox[4];
  p2[0] = bbox[1];
  p2[1] = bbox[3];
  p2[2] = bbox[5];
  int ijk[2][3];

  //  Find bucket the points are in
  this->GetBucketIndices(p1, ijk[0]);
  this->GetBucketIndices(p2, ijk[1]);

  // Now loop over block to load in ids
  int leafStart = this->NumberOfOctants -
    this->NumberOfDivisions * this->NumberOfDivisions * this->NumberOfDivisions;
  vtkIdList* cellIds;
  vtkIdType idx;
  int i, j, k;
  for (k = ijk[0][2]; k <= ijk[1][2]; k++)
  {
    for (j = ijk[0][1]; j <= ijk[1][1]; j++)
    {
      for (i = ijk[0][0]; i <= ijk[1][0]; i++)
      {
        if ((cellIds = this->Tree[leafStart + i + j * this->NumberOfDivisions +
               k * this->NumberOfDivisions * this->NumberOfDivisions]) != nullptr)
        {
          for (idx = 0; idx < cellIds->GetNumberOfIds(); idx++)
          {
            cells->InsertUniqueId(cellIds->GetId(idx));
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
struct IntersectionInfo
{
  vtkIdType CellId;
  std::array<double, 3> IntersectionPoint;
  double T;

  IntersectionInfo(vtkIdType cellId, double x[3], double t)
    : CellId(cellId)
    , IntersectionPoint({ x[0], x[1], x[2] })
    , T(t)
  {
  }
};

//------------------------------------------------------------------------------
int vtkCellLocator::IntersectWithLine(const double p1[3], const double p2[3], const double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (this->Tree == nullptr)
  {
    return 0;
  }
  // Initialize the list of points/cells
  if (points)
  {
    points->Reset();
  }
  if (cellIds)
  {
    cellIds->Reset();
  }
  double* bounds = this->Bounds;
  double* h = this->H;
  double t0, t1, x0[3], x1[3], tHitCell;
  double hitCellBoundsPosition[3], cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  double octantBounds[6];
  int prod = this->NumberOfDivisions * this->NumberOfDivisions;
  vtkIdType cellIdBest = -1, cId, i, idx, numberOfCellsInBucket;
  int ijk[3], ijkEnd[3];
  int plane0, plane1, hitCellBounds, subId;
  double step[3], next[3], tMax[3], tDelta[3];
  double rayDir[3];
  vtkMath::Subtract(p2, p1, rayDir);
  double t, x[3], pcoords[3];
  int leafStart = this->NumberOfOctants -
    this->NumberOfDivisions * this->NumberOfDivisions * this->NumberOfDivisions;

  // Make sure the bounding box of the locator is hit. Also, determine the
  // entry and exit points into and out of the locator. This is used to
  // determine the bins where the ray starts and ends.
  if (vtkBox::IntersectWithLine(bounds, p1, p2, t0, t1, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);

  // Get the i-j-k point of intersection and bin index. This is
  // clamped to the boundary of the locator.
  this->GetBucketIndices(x0, ijk);
  this->GetBucketIndices(x1, ijkEnd);
  idx = leafStart + ijk[0] + ijk[1] * this->NumberOfDivisions + ijk[2] * prod;

  // Set up some traversal parameters for traversing through bins
  step[0] = (rayDir[0] >= 0.0) ? 1.0 : -1.0;
  step[1] = (rayDir[1] >= 0.0) ? 1.0 : -1.0;
  step[2] = (rayDir[2] >= 0.0) ? 1.0 : -1.0;

  // If the ray is going in the negative direction, then the next voxel boundary
  // is on the "-" direction so we stay in the current voxel.
  next[0] = bounds[0] + h[0] * (rayDir[0] >= 0.0 ? (ijk[0] + step[0]) : ijk[0]);
  next[1] = bounds[2] + h[1] * (rayDir[1] >= 0.0 ? (ijk[1] + step[1]) : ijk[1]);
  next[2] = bounds[4] + h[2] * (rayDir[2] >= 0.0 ? (ijk[2] + step[2]) : ijk[2]);

  tMax[0] = (rayDir[0] != 0.0) ? (next[0] - x0[0]) / rayDir[0] : VTK_FLOAT_MAX;
  tMax[1] = (rayDir[1] != 0.0) ? (next[1] - x0[1]) / rayDir[1] : VTK_FLOAT_MAX;
  tMax[2] = (rayDir[2] != 0.0) ? (next[2] - x0[2]) / rayDir[2] : VTK_FLOAT_MAX;

  tDelta[0] = (rayDir[0] != 0.0) ? (h[0] / rayDir[0]) * step[0] : VTK_FLOAT_MAX;
  tDelta[1] = (rayDir[1] != 0.0) ? (h[1] / rayDir[1]) * step[1] : VTK_FLOAT_MAX;
  tDelta[2] = (rayDir[2] != 0.0) ? (h[2] / rayDir[2]) * step[2] : VTK_FLOAT_MAX;

  // we will sort intersections by t, so keep track using these lists
  std::vector<IntersectionInfo> cellIntersections;

  for (cellIdBest = (-1); cellIdBest < 0;)
  {
    if (this->Tree[idx] &&
      (numberOfCellsInBucket = this->Tree[idx]->GetNumberOfIds()) > 0) // there are some cell here
    {
      this->ComputeOctantBounds(octantBounds, ijk[0], ijk[1], ijk[2]);
      for (i = 0; i < numberOfCellsInBucket; ++i)
      {
        cId = this->Tree[idx]->GetId(i);
        if (!cellHasBeenVisited[cId])
        {
          cellHasBeenVisited[cId] = true;

          // check whether we intersect the cell bounds
          this->GetCellBounds(cId, cellBoundsPtr);
          hitCellBounds =
            vtkBox::IntersectBox(cellBoundsPtr, p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

          if (hitCellBounds)
          {
            // Note because of cellHasBeenVisited[], we know this cId is unique
            if (cell)
            {
              this->DataSet->GetCell(cId, cell);
              if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
              {
                // Make sure that intersection occurs within this octant or else spurious cell
                // intersections can occur behind this bin which are not the correct answer.
                if (!vtkAbstractCellLocator::IsInBounds(octantBounds, x, tol))
                {
                  cellHasBeenVisited[cId] = false; // mark the cell non-visited
                }
                else
                {
                  cellIntersections.emplace_back(cId, x, t);
                }
              }
            }
            else
            {
              cellIntersections.emplace_back(cId, hitCellBoundsPosition, tHitCell);
            }
          } // if (hitCellBounds)
        }   // if (!CellHasBeenVisited[cId])
      }
    }

    // See if the traversal is complete (reached the end of the line).
    if (ijk[0] == ijkEnd[0] && ijk[1] == ijkEnd[1] && ijk[2] == ijkEnd[2])
    {
      break;
    }

    // move to the next octant
    // Advance to next voxel
    if (tMax[0] < tMax[1])
    {
      if (tMax[0] < tMax[2])
      {
        ijk[0] += static_cast<int>(step[0]);
        tMax[0] += tDelta[0];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }
    else
    {
      if (tMax[1] < tMax[2])
      {
        ijk[1] += static_cast<int>(step[1]);
        tMax[1] += tDelta[1];
      }
      else
      {
        ijk[2] += static_cast<int>(step[2]);
        tMax[2] += tDelta[2];
      }
    }

    if (ijk[0] < 0 || ijk[0] >= this->NumberOfDivisions || ijk[1] < 0 ||
      ijk[1] >= this->NumberOfDivisions || ijk[2] < 0 || ijk[2] >= this->NumberOfDivisions)
    {
      break;
    }
    else
    {
      idx = leafStart + ijk[0] + ijk[1] * this->NumberOfDivisions + ijk[2] * prod;
    }
  }

  // if we had intersections, sort them by increasing t
  if (!cellIntersections.empty())
  {
    vtkIdType numIntersections = static_cast<vtkIdType>(cellIntersections.size());
    std::sort(cellIntersections.begin(), cellIntersections.end(),
      [&](const IntersectionInfo& a, const IntersectionInfo& b) { return a.T < b.T; });
    if (points)
    {
      points->SetNumberOfPoints(numIntersections);
      for (i = 0; i < numIntersections; ++i)
      {
        points->SetPoint(i, cellIntersections[i].IntersectionPoint.data());
      }
    }
    if (cellIds)
    {
      cellIds->SetNumberOfIds(numIntersections);
      for (i = 0; i < numIntersections; ++i)
      {
        cellIds->SetId(i, cellIntersections[i].CellId);
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkCellLocator::ShallowCopy(vtkAbstractCellLocator* locator)
{
  vtkCellLocator* cellLocator = vtkCellLocator::SafeDownCast(locator);
  if (!cellLocator)
  {
    vtkErrorMacro("Cannot cast " << locator->GetClassName() << " to vtkCellLocator.");
    return;
  }
  // we only copy what's actually used by vtkCellLocator

  // vtkLocator parameters
  this->SetDataSet(cellLocator->GetDataSet());
  this->SetUseExistingSearchStructure(cellLocator->GetUseExistingSearchStructure());
  this->SetAutomatic(cellLocator->GetAutomatic());
  this->SetMaxLevel(cellLocator->GetMaxLevel());
  this->Level = cellLocator->Level;

  // vtkAbstractCellLocator parameters
  this->SetNumberOfCellsPerNode(cellLocator->GetNumberOfCellsPerNode());
  this->CacheCellBounds = cellLocator->CacheCellBounds;
  this->CellBoundsSharedPtr = cellLocator->CellBoundsSharedPtr; // This is important
  this->CellBounds = this->CellBoundsSharedPtr.get() ? this->CellBoundsSharedPtr->data() : nullptr;

  // vtkCellLocator parameters
  this->NumberOfOctants = cellLocator->NumberOfOctants;
  std::copy_n(cellLocator->Bounds, 6, this->Bounds);
  std::copy_n(cellLocator->H, 3, this->H);
  this->NumberOfDivisions = cellLocator->NumberOfDivisions;
  this->TreeSharedPtr = cellLocator->TreeSharedPtr; // This is important
  this->Tree = this->TreeSharedPtr.get() ? this->TreeSharedPtr->data() : nullptr;
}

//------------------------------------------------------------------------------
void vtkCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfOctants: " << this->NumberOfOctants << "\n";
  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1] << " " << this->Bounds[2]
     << " " << this->Bounds[3] << " " << this->Bounds[4] << " " << this->Bounds[5] << "\n";
  os << indent << "H: " << this->H[0] << " " << this->H[1] << " " << this->H[2] << "\n";
  os << indent << "NumberOfDivisions: " << this->NumberOfDivisions << "\n";
}
