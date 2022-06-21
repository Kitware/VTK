/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClosestPointStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClosestPointStrategy.h"

#include "vtkAbstractPointLocator.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkClosestPointStrategy);

//------------------------------------------------------------------------------
vtkClosestPointStrategy::vtkClosestPointStrategy()
{
  // Preallocate for performance
  this->PointIds->Allocate(16);
  this->Neighbors->Allocate(32);
  this->CellIds->Allocate(32);
  this->NearPointIds->Allocate(32);

  this->PointLocator = nullptr;
}

//------------------------------------------------------------------------------
vtkClosestPointStrategy::~vtkClosestPointStrategy()
{
  if (this->OwnsLocator && this->PointLocator != nullptr)
  {
    this->PointLocator->Delete();
    this->PointLocator = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkClosestPointStrategy::SetPointLocator(vtkAbstractPointLocator* pL)
{
  if (pL != this->PointLocator)
  {
    if (this->PointLocator != nullptr && this->OwnsLocator)
    {
      this->PointLocator->Delete();
    }

    this->PointLocator = pL;

    if (pL != nullptr)
    {
      pL->Register(this);
    }

    this->OwnsLocator = true;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkClosestPointStrategy::Initialize(vtkPointSet* ps)
{
  // See whether anything has changed. If not, just return.
  if (this->PointSet != nullptr && ps == this->PointSet && this->MTime < this->InitializeTime)
  {
    return 1;
  }

  // Set up the point set; return on failure.
  if (this->Superclass::Initialize(ps) == 0)
  {
    return 0;
  }

  // Use the point set's point locator preferentially. If no point locator,
  // then we need to create one. If one is specified here in the strategy,
  // use that. If not, then used the point set's default build point locator
  // method.
  vtkAbstractPointLocator* psPL = ps->GetPointLocator();
  if (psPL == nullptr)
  {
    if (this->PointLocator != nullptr)
    {
      // only the owner of the locator can change it
      if (this->OwnsLocator)
      {
        this->PointLocator->SetDataSet(ps);
        this->PointLocator->BuildLocator();
      }
    }
    else
    {
      ps->BuildPointLocator();
      psPL = ps->GetPointLocator();
      this->PointLocator = psPL;
      this->OwnsLocator = false;
    }
  }
  else
  {
    if (psPL != this->PointLocator)
    {
      this->PointLocator = psPL;
      this->OwnsLocator = false;
    }
    // ensure that the point-set's locator is up-to-date
    // this should be done only by one thread
    if (!this->IsACopy)
    {
      this->PointLocator->BuildLocator();
    }
  }
  this->VisitedCells.resize(static_cast<size_t>(ps->GetNumberOfCells()));
  this->Weights.resize(8);

  this->InitializeTime.Modified();

  return 1;
}

//------------------------------------------------------------------------------
namespace
{

//------------------------------------------------------------------------------
// Used internally by FindCell to walk through neighbors from a starting cell.
// The arguments are the same as those for FindCell.  In addition, visitedCells
// keeps a list of cells already traversed.  If we run into such already
// visited, the walk terminates since we assume we already walked from that cell
// and found nothing.  The ptIds and neighbors lists are buffers used
// internally.  They are passed in so that they do not have to be continuously
// reallocated.
vtkIdType FindCellWalk(vtkClosestPointStrategy* self, vtkPointSet* ps, double x[3], vtkCell* cell,
  vtkGenericCell* gencell, vtkIdType cellId, double tol2, int& subId, double pcoords[3],
  double* weights, std::vector<unsigned char>& visitedCells, vtkIdList* visitedCellIds,
  vtkIdList* ptIds, vtkIdList* neighbors)
{
  const int VTK_MAX_WALK = 12;
  double closestPoint[3];
  double dist2;
  for (int walk = 0; walk < VTK_MAX_WALK; walk++)
  {
    // Check to see if we already visited this cell.
    if (visitedCells[cellId])
    {
      break;
    }
    visitedCells[cellId] = true;
    visitedCellIds->InsertNextId(cellId);

    // Get information for the cell.
    cell = self->SelectCell(ps, cellId, cell, gencell);

    // Check to see if the current, cached cell contains the point.
    if ((cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) == 1) &&
      (dist2 <= tol2))
    {
      return cellId;
    }

    // This is not the right cell.  Find the next one.
    cell->CellBoundary(subId, pcoords, ptIds);
    ps->GetCellNeighbors(cellId, ptIds, neighbors);
    // If there is no next one, exit.
    if (neighbors->GetNumberOfIds() < 1)
    {
      break;
    }
    // Set the next cell as the current one and iterate.
    cellId = neighbors->GetId(0);
    cell = nullptr;
  }

  // Could not find a cell.
  return -1;
}

//------------------------------------------------------------------------------
vtkIdType FindCellWalk(vtkClosestPointStrategy* self, vtkPointSet* ps, double x[3],
  vtkGenericCell* gencell, vtkIdList* cellIds, double tol2, int& subId, double pcoords[3],
  double* weights, std::vector<unsigned char>& visitedCells, vtkIdList* visitedCellIds,
  vtkIdList* ptIds, vtkIdList* neighbors)
{
  vtkIdType numCellIds = cellIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numCellIds; i++)
  {
    vtkIdType cellId = cellIds->GetId(i);
    vtkIdType foundCell = FindCellWalk(self, ps, x, nullptr, gencell, cellId, tol2, subId, pcoords,
      weights, visitedCells, visitedCellIds, ptIds, neighbors);
    if (foundCell >= 0)
    {
      return foundCell;
    }
  }
  return -1;
}

} // anonymous namespace

//------------------------------------------------------------------------------
vtkIdType vtkClosestPointStrategy::FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell,
  vtkIdType cellId, double tol2, int& subId, double pcoords[3], double* weights)
{
  // Check to see if the point is within the bounds of the data.  This is not
  // a strict check, but it is fast.
  const double* bounds = this->Bounds;
  const double tol = std::sqrt(tol2);
  if ((x[0] < bounds[0] - tol) || (x[0] > bounds[1] + tol) || (x[1] < bounds[2] - tol) ||
    (x[1] > bounds[3] + tol) || (x[2] < bounds[4] - tol) || (x[2] > bounds[5] + tol))
  {
    return -1;
  }

  // reset the visited cells
  for (vtkIdType i = 0, max = this->VisitedCellIds->GetNumberOfIds(); i < max; ++i)
  {
    this->VisitedCells[this->VisitedCellIds->GetId(i)] = false;
  }
  this->VisitedCellIds->Reset();

  // If we are given a starting cell, try that.
  vtkIdType foundCell;
  if (cell && (cellId >= 0))
  {
    foundCell = FindCellWalk(this, this->PointSet, x, cell, gencell, cellId, tol2, subId, pcoords,
      weights, this->VisitedCells, this->VisitedCellIds, this->PointIds, this->Neighbors);
    if (foundCell >= 0)
    {
      return foundCell;
    }
  }

  // The starting cell didn't work, find the point closest to the coordinates
  // given and search the attached cells.
  vtkIdType ptId = this->PointLocator->FindClosestPoint(x);
  if (ptId < 0)
  {
    return -1;
  }
  this->PointSet->GetPointCells(ptId, this->CellIds);
  foundCell = FindCellWalk(this, this->PointSet, x, gencell, this->CellIds, tol2, subId, pcoords,
    weights, this->VisitedCells, this->VisitedCellIds, this->PointIds, this->Neighbors);
  if (foundCell >= 0)
  {
    return foundCell;
  }

  // It is possible that the topology is not fully connected as points may be
  // coincident.  Handle this by looking at every point within the tolerance
  // and consider all cells connected.  It has been suggested that we should
  // really do this coincident point check at every point as we walk through
  // neighbors, which would happen in FindCellWalk.  If that were ever
  // implemented, this step might become unnecessary.
  double ptCoord[3];
  this->PointSet->GetPoint(ptId, ptCoord);
  this->PointLocator->FindPointsWithinRadius(tol, ptCoord, this->NearPointIds);
  this->NearPointIds->DeleteId(ptId); // Already searched this one.
  for (vtkIdType i = 0, numPts = this->NearPointIds->GetNumberOfIds(); i < numPts; i++)
  {
    this->PointSet->GetPointCells(this->NearPointIds->GetId(i), this->CellIds);
    foundCell = FindCellWalk(this, this->PointSet, x, gencell, this->CellIds, tol2, subId, pcoords,
      weights, this->VisitedCells, this->VisitedCellIds, this->PointIds, this->Neighbors);
    if (foundCell >= 0)
    {
      return foundCell;
    }
  }

  // Could not find a containing cell. Either the query point is outside of
  // the dataset, or there is an uncommon pathology of disconnected cells and
  // points (if using a point locator approach). In this latter case, a cell
  // locator is necessary.
  return -1;
}

//------------------------------------------------------------------------------
vtkIdType vtkClosestPointStrategy::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* genCell, vtkIdType& closestCellId, int& closestSubId,
  double& minDist2, int& inside)
{
  // the implementation of this function is a copy of an old approach in
  // vtkAbstractInterpolatedVelocityField, check git history for more info
  // in the future something better could be implemented
  vtkIdType retVal = 0;

  // find the point closest to the coordinates
  // given and search the attached cells.
  vtkIdType ptId = this->PointLocator->FindClosestPoint(x);
  if (ptId < 0)
  {
    return retVal;
  }
  // find the cells adjacent to the closest point
  this->PointSet->GetPointCells(ptId, this->CellIds);
  double point[3], pcoords[3], dist2, closestPcoords[3];
  int subId, stat;
  vtkIdType cellId;
  closestSubId = -1;
  closestCellId = -1;
  minDist2 = this->PointSet->GetLength2();
  // find the closest adjacent cell
  for (vtkIdType id = 0, max = this->CellIds->GetNumberOfIds(); id < max; ++id)
  {
    cellId = this->CellIds->GetId(id);
    this->PointSet->GetCell(cellId, genCell);
    if (this->Weights.size() < static_cast<size_t>(genCell->GetNumberOfPoints()))
    {
      this->Weights.resize(static_cast<size_t>(genCell->GetNumberOfPoints()));
    }
    stat = genCell->EvaluatePosition(x, point, subId, pcoords, dist2, this->Weights.data());
    if (stat != -1 && dist2 < minDist2)
    {
      retVal = 1;
      inside = stat;
      minDist2 = dist2;
      closestCellId = cellId;
      closestSubId = subId;
      closestPoint[0] = point[0];
      closestPoint[1] = point[1];
      closestPoint[2] = point[2];
      closestPcoords[0] = pcoords[0];
      closestPcoords[1] = pcoords[1];
      closestPcoords[2] = pcoords[2];
    }
  }
  if (closestCellId == -1)
  {
    return retVal;
  }
  // Recover the closest cell
  this->PointSet->GetCell(closestCellId, genCell);
  // get the boundary point ids closest to the parametric coordinates
  genCell->CellBoundary(closestSubId, closestPcoords, this->PointIds);
  // get the neighbors cells of the boundary points
  this->PointSet->GetCellNeighbors(closestCellId, this->PointIds, this->Neighbors);
  // check if any of the neighbors is closer to the query point
  for (vtkIdType neighCellId = 0, max = this->Neighbors->GetNumberOfIds(); neighCellId < max;
       ++neighCellId)
  {
    cellId = this->Neighbors->GetId(neighCellId);
    this->PointSet->GetCell(cellId, genCell);
    if (this->Weights.size() < static_cast<size_t>(genCell->GetNumberOfPoints()))
    {
      this->Weights.resize(static_cast<size_t>(genCell->GetNumberOfPoints()));
    }
    stat = genCell->EvaluatePosition(x, point, subId, pcoords, dist2, this->Weights.data());
    if (stat != -1 && dist2 < minDist2)
    {
      retVal = 1;
      inside = stat;
      minDist2 = dist2;
      closestCellId = cellId;
      closestSubId = subId;
      closestPoint[0] = point[0];
      closestPoint[1] = point[1];
      closestPoint[2] = point[2];
      // pcoords are not used again
    }
  }
  // the closest is within the given radius
  if (minDist2 > radius * radius)
  {
    retVal = 0;
  }
  return retVal;
}

//------------------------------------------------------------------------------
bool vtkClosestPointStrategy::InsideCellBounds(double x[3], vtkIdType cellId)
{
  double bounds[6];
  this->PointSet->GetCellBounds(cellId, bounds);
  return bounds[0] <= x[0] && x[0] <= bounds[1] && bounds[2] <= x[1] && x[1] <= bounds[3] &&
    bounds[4] <= x[2] && x[2] <= bounds[5];
}

//------------------------------------------------------------------------------
void vtkClosestPointStrategy::CopyParameters(vtkFindCellStrategy* from)
{
  this->Superclass::CopyParameters(from);

  if (auto strategy = vtkClosestPointStrategy::SafeDownCast(from))
  {
    if (strategy->PointLocator)
    {
      this->PointLocator = strategy->PointLocator;
      this->OwnsLocator = false;
    }
  }
}

//------------------------------------------------------------------------------
void vtkClosestPointStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PointLocator: " << this->PointLocator << "\n";
}
