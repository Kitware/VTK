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
#include "vtkCell.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"

#include <set>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkClosestPointStrategy);

//----------------------------------------------------------------------------
vtkClosestPointStrategy::vtkClosestPointStrategy()
{
  // Preallocate for performance
  this->PointIds = vtkIdList::New();
  this->PointIds->Allocate(16);
  this->Neighbors = vtkIdList::New();
  this->Neighbors->Allocate(32);
  this->CellIds = vtkIdList::New();
  this->CellIds->Allocate(32);
  this->NearPointIds = vtkIdList::New();
  this->NearPointIds->Allocate(32);

  // You may ask, why this OwnsLocator rigamarole. The reason is because the
  // reference counting garbage collecter gets confused when the locator,
  // point set, and strategy are all mixed together; resulting in memory
  // leaks etc.
  this->OwnsLocator = false;
  this->PointLocator = nullptr;
}

//----------------------------------------------------------------------------
vtkClosestPointStrategy::~vtkClosestPointStrategy()
{
  this->PointIds->Delete();
  this->Neighbors->Delete();
  this->CellIds->Delete();
  this->NearPointIds->Delete();

  if (this->OwnsLocator && this->PointLocator != nullptr)
  {
    this->PointLocator->Delete();
    this->PointLocator = nullptr;
  }
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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
    if (this->PointLocator != nullptr && this->OwnsLocator)
    {
      this->PointLocator->SetDataSet(ps);
      this->PointLocator->BuildLocator();
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
    this->PointLocator = psPL;
    this->OwnsLocator = false;
  }

  this->InitializeTime.Modified();

  return 1;
}

//-----------------------------------------------------------------------------
namespace
{

//-----------------------------------------------------------------------------
// Used internally by FindCell to walk through neighbors from a starting cell.
// The arguments are the same as those for FindCell.  In addition, visitedCells
// keeps a list of cells already traversed.  If we run into such already
// visited, the walk terminates since we assume we already walked from that cell
// and found nothing.  The ptIds and neighbors lists are buffers used
// internally.  They are passed in so that they do not have to be continuously
// reallocated.
vtkIdType FindCellWalk(vtkClosestPointStrategy* self, vtkPointSet* ps, double x[3], vtkCell* cell,
  vtkGenericCell* gencell, vtkIdType cellId, double tol2, int& subId, double pcoords[3],
  double* weights, std::set<vtkIdType>& visitedCells, vtkIdList* ptIds, vtkIdList* neighbors)
{
  const int VTK_MAX_WALK = 12;
  for (int walk = 0; walk < VTK_MAX_WALK; walk++)
  {
    // Check to see if we already visited this cell.
    if (visitedCells.find(cellId) != visitedCells.end())
      break;
    visitedCells.insert(cellId);

    // Get information for the cell.
    cell = self->SelectCell(ps, cellId, cell, gencell);

    // Check to see if the current, cached cell contains the point.
    double closestPoint[3];
    double dist2;
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
      break;
    // Set the next cell as the current one and iterate.
    cellId = neighbors->GetId(0);
    cell = nullptr;
  }

  // Could not find a cell.
  return -1;
}

//-----------------------------------------------------------------------------
vtkIdType FindCellWalk(vtkClosestPointStrategy* self, vtkPointSet* ps, double x[3],
  vtkGenericCell* gencell, vtkIdList* cellIds, double tol2, int& subId, double pcoords[3],
  double* weights, std::set<vtkIdType>& visitedCells, vtkIdList* ptIds, vtkIdList* neighbors)
{
  vtkIdType numCellIds = cellIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numCellIds; i++)
  {
    vtkIdType cellId = cellIds->GetId(i);
    vtkIdType foundCell = FindCellWalk(self, ps, x, nullptr, gencell, cellId, tol2, subId, pcoords,
      weights, visitedCells, ptIds, neighbors);
    if (foundCell >= 0)
    {
      return foundCell;
    }
  }
  return -1;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
vtkIdType vtkClosestPointStrategy::FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell,
  vtkIdType cellId, double tol2, int& subId, double pcoords[3], double* weights)
{
  // Check to see if the point is within the bounds of the data.  This is not
  // a strict check, but it is fast.
  const double* bounds = this->Bounds;
  double tol = sqrt(tol2);
  if ((x[0] < bounds[0] - tol) || (x[0] > bounds[1] + tol) || (x[1] < bounds[2] - tol) ||
    (x[1] > bounds[3] + tol) || (x[2] < bounds[4] - tol) || (x[2] > bounds[5] + tol))
  {
    return -1;
  }

  // Implement the strategy proper
  this->VisitedCells.clear();

  // If we are given a starting cell, try that.
  vtkIdType foundCell;
  if (cell && (cellId >= 0))
  {
    foundCell = FindCellWalk(this, this->PointSet, x, cell, gencell, cellId, tol2, subId, pcoords,
      weights, this->VisitedCells, this->PointIds, this->Neighbors);
    if (foundCell >= 0)
    {
      return foundCell;
    }
  }

  // The starting cell didn't work, find the point closest to the coordinates
  // given and search the attached cells.
  vtkIdType ptId = this->PointLocator->FindClosestPoint(x);
  if (ptId < 0)
    return -1;
  this->PointSet->GetPointCells(ptId, this->CellIds);
  foundCell = FindCellWalk(this, this->PointSet, x, gencell, this->CellIds, tol2, subId, pcoords,
    weights, this->VisitedCells, this->PointIds, this->Neighbors);
  if (foundCell >= 0)
  {
    return foundCell;
  }

  // It is possible that the toplogy is not fully connected as points are
  // coincident.  Handle this by looking at every point within the tolerance
  // and consider all cells connected.  It has been suggested that we should
  // really do this coincident point check at every point as we walk through
  // neighbors, which would happen in FindCellWalk.  If that were ever
  // implemented, this step might become unnecessary.
  double ptCoord[3];
  this->PointSet->GetPoint(ptId, ptCoord);
  this->PointLocator->FindPointsWithinRadius(sqrt(tol2), ptCoord, this->NearPointIds);
  this->NearPointIds->DeleteId(ptId); // Already searched this one.
  vtkIdType i, numPts = this->NearPointIds->GetNumberOfIds();
  for (i = 0; i < numPts; i++)
  {
    this->PointSet->GetPointCells(this->NearPointIds->GetId(i), this->CellIds);
    foundCell = FindCellWalk(this, this->PointSet, x, gencell, this->CellIds, tol2, subId, pcoords,
      weights, this->VisitedCells, this->PointIds, this->Neighbors);
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

//----------------------------------------------------------------------------
void vtkClosestPointStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PointLocator: " << this->PointLocator << "\n";
}
