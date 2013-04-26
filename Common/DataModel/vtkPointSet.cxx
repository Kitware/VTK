/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSet.h"

#include "vtkCell.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointLocator.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <set>


vtkCxxSetObjectMacro(vtkPointSet,Points,vtkPoints);

vtkPointSet::vtkPointSet ()
{
  this->Points = NULL;
  this->Locator = NULL;
}

//----------------------------------------------------------------------------
vtkPointSet::~vtkPointSet ()
{
  this->Cleanup();

  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

//----------------------------------------------------------------------------
// Copy the geometric structure of an input point set object.
void vtkPointSet::CopyStructure(vtkDataSet *ds)
{
  vtkPointSet *ps=static_cast<vtkPointSet *>(ds);

  if ( this->Points != ps->Points )
    {
    if ( this->Locator )
      {
      this->Locator->Initialize();
      }
    this->SetPoints(ps->Points);
    }
}

//----------------------------------------------------------------------------
void vtkPointSet::Cleanup()
{
  if ( this->Points )
    {
    this->Points->UnRegister(this);
    this->Points = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPointSet::Initialize()
{
  vtkDataSet::Initialize();

  this->Cleanup();

  if ( this->Locator )
    {
    this->Locator->Initialize();
    }
}
//----------------------------------------------------------------------------
void vtkPointSet::ComputeBounds()
{
  double *bounds;

  if ( this->Points )
    {
    bounds = this->Points->GetBounds();
    for (int i=0; i<6; i++)
      {
      this->Bounds[i] = bounds[i];
      }
    this->ComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
unsigned long int vtkPointSet::GetMTime()
{
  unsigned long int dsTime = vtkDataSet::GetMTime();

  if ( this->Points )
    {
    if ( this->Points->GetMTime() > dsTime )
      {
      dsTime = this->Points->GetMTime();
      }
    }

  // don't get locator's mtime because its an internal object that cannot be
  // modified directly from outside. Causes problems due to FindCell()
  // SetPoints() method.

  return dsTime;
}

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindPoint(double x[3])
{
  if ( !this->Points )
    {
    return -1;
    }

  if ( !this->Locator )
    {
    this->Locator = vtkPointLocator::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    this->Locator->SetDataSet(this);
    }

  if ( this->Points->GetMTime() > this->Locator->GetMTime() )
    {
    this->Locator->SetDataSet(this);
    }

  return this->Locator->FindClosestPoint(x);
}

//the furthest the walk can be - prevents aimless wandering
#define VTK_MAX_WALK 12

//-----------------------------------------------------------------------------
// Used internally by FindCell to walk through neighbors from a starting cell.
// The arguments are the same as those for FindCell.  In addition, visitedCells
// keeps a list of cells already traversed.  If we run into such already
// visited, the walk terminates since we assume we already walked from that cell
// and found nothing.  The ptIds and neighbors lists are buffers used
// internally.  They are passed in so that they do not have to be continuously
// reallocated.
static vtkIdType FindCellWalk(vtkPointSet *self, double x[3], vtkCell *cell,
                              vtkGenericCell *gencell, vtkIdType cellId,
                              double tol2, int &subId, double pcoords[3],
                              double *weights,
                              std::set<vtkIdType> &visitedCells,
                              vtkIdList *ptIds, vtkIdList *neighbors)
{
  for (int walk = 0; walk < VTK_MAX_WALK; walk++)
    {
    // Check to see if we already visited this cell.
    if (visitedCells.find(cellId) != visitedCells.end()) break;
    visitedCells.insert(cellId);

    // Get information for the cell.
    if (!cell)
      {
      if (gencell)
        {
        self->GetCell(cellId, gencell);
        cell = gencell;
        }
      else
        {
        cell = self->GetCell(cellId);
        }
      }

    // Check to see if the cell contains the point.
    double closestPoint[3];
    double dist2;
    if (   (cell->EvaluatePosition(x, closestPoint, subId,
                                   pcoords, dist2, weights) == 1)
        && (dist2 <= tol2) )
      {
      return cellId;
      }

    // This is not the right cell.  Find the next one.
    cell->CellBoundary(subId, pcoords, ptIds);
    self->GetCellNeighbors(cellId, ptIds, neighbors);
    // If there is no next one, exit.
    if (neighbors->GetNumberOfIds() < 1) break;
    // Set the next cell as the current one and iterate.
    cellId = neighbors->GetId(0);
    cell = NULL;
    }

  // Could not find a cell.
  return -1;
}

//-----------------------------------------------------------------------------
static vtkIdType FindCellWalk(vtkPointSet *self, double x[3],
                              vtkGenericCell *gencell, vtkIdList *cellIds,
                              double tol2, int &subId, double pcoords[3],
                              double *weights,
                              std::set<vtkIdType> &visitedCells,
                              vtkIdList *ptIds, vtkIdList *neighbors)
{
  for (vtkIdType i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
    vtkIdType cellId = cellIds->GetId(i);
    vtkIdType foundCell = FindCellWalk(self, x, NULL, gencell, cellId,
                                       tol2, subId, pcoords, weights,
                                       visitedCells, ptIds, neighbors);
    if (foundCell >= 0) return foundCell;
    }
  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindCell(double x[3], vtkCell *cell,
                                vtkGenericCell *gencell, vtkIdType cellId,
                                double tol2, int& subId, double pcoords[3],
                                double *weights)
{
  vtkIdType foundCell;

  // make sure everything is up to snuff
  if ( !this->Points || this->Points->GetNumberOfPoints() < 1)
    {
    return -1;
    }

  // Check to see if the point is within the bounds of the data.  This is not
  // a strict check, but it is fast.
  double bounds[6];
  this->GetBounds(bounds);
  if (   (x[0] < bounds[0]) || (x[0] > bounds[1])
      || (x[1] < bounds[2]) || (x[1] > bounds[3])
      || (x[2] < bounds[4]) || (x[2] > bounds[5]) )
    {
    return -1;
    }

  if ( !this->Locator )
    {
    this->Locator = vtkPointLocator::New();
    this->Locator->Register(this);
    this->Locator->Delete();
    this->Locator->SetDataSet(this);
    this->Locator->BuildLocator();
    }

  if ( this->Points->GetMTime() > this->Locator->GetMTime() )
    {
    this->Locator->SetDataSet(this);
    this->Locator->BuildLocator();
    }

  std::set<vtkIdType> visitedCells;
  VTK_CREATE(vtkIdList, ptIds);
  ptIds->Allocate(8, 100);
  VTK_CREATE(vtkIdList, neighbors);
  neighbors->Allocate(8, 100);

  // If we are given a starting cell, try that.
  if (cell && (cellId >= 0))
    {
    foundCell = FindCellWalk(this, x, cell, gencell, cellId,
                             tol2, subId, pcoords, weights,
                             visitedCells, ptIds, neighbors);
    if (foundCell >= 0) return foundCell;
    }

  VTK_CREATE(vtkIdList, cellIds);
  cellIds->Allocate(8,100);

  // Now find the point closest to the coordinates given and search from the
  // adjacent cells.
  vtkIdType ptId = this->Locator->FindClosestPoint(x);
  if (ptId < 0) return -1;
  this->GetPointCells(ptId, cellIds);
  foundCell = FindCellWalk(this, x, gencell, cellIds,
                           tol2, subId, pcoords, weights,
                           visitedCells, ptIds, neighbors);
  if (foundCell >= 0) return foundCell;

  // It is possible that the toplogy is not fully connected.  That is, two
  // points in the point list could be coincident.  Handle that by looking
  // at every point within the tolerance and consider all cells connected.
  // It has been suggested that we should really do this coincident point
  // check at every point as we walk through neighbors, which would happen
  // in FindCellWalk.  If that were ever implemented, this step might become
  // unnecessary.
  double ptCoord[3];
  this->GetPoint(ptId, ptCoord);
  VTK_CREATE(vtkIdList, coincidentPtIds);
  coincidentPtIds->Allocate(8, 100);
  this->Locator->FindPointsWithinRadius(tol2, ptCoord, coincidentPtIds);
  coincidentPtIds->DeleteId(ptId);      // Already searched this one.
  for (vtkIdType i = 0; i < coincidentPtIds->GetNumberOfIds(); i++)
    {
    this->GetPointCells(coincidentPtIds->GetId(i), cellIds);
    foundCell = FindCellWalk(this, x, gencell, cellIds,
                             tol2, subId, pcoords, weights,
                             visitedCells, ptIds, neighbors);
    if (foundCell >= 0) return foundCell;
    }

  // Could not find the cell.
  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindCell(double x[3], vtkCell *cell, vtkIdType cellId,
                                double tol2, int& subId,double pcoords[3],
                                double *weights)
{
  return
    this->FindCell( x, cell, NULL, cellId, tol2, subId, pcoords, weights );
}

#undef VTK_MAX_WALK


//----------------------------------------------------------------------------
void vtkPointSet::Squeeze()
{
  if ( this->Points )
    {
    this->Points->Squeeze();
    }
  vtkDataSet::Squeeze();
}

//----------------------------------------------------------------------------
void vtkPointSet::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Locator, "Locator");
}

//----------------------------------------------------------------------------
unsigned long vtkPointSet::GetActualMemorySize()
{
  unsigned long size=this->vtkDataSet::GetActualMemorySize();
  if ( this->Points )
    {
    size += this->Points->GetActualMemorySize();
    }
  return size;
}

//----------------------------------------------------------------------------
void vtkPointSet::ShallowCopy(vtkDataObject *dataObject)
{
  vtkPointSet *pointSet = vtkPointSet::SafeDownCast(dataObject);

  if ( pointSet != NULL )
    {
    this->SetPoints(pointSet->GetPoints());
    }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkPointSet::DeepCopy(vtkDataObject *dataObject)
{
  vtkPointSet *pointSet = vtkPointSet::SafeDownCast(dataObject);

  if ( pointSet != NULL )
    {
    vtkPoints* newPoints;
    vtkPoints* pointsToCopy = pointSet->GetPoints();
    if (pointsToCopy)
      {
      newPoints = pointsToCopy->NewInstance();
      newPoints->SetDataType(pointsToCopy->GetDataType());
      newPoints->DeepCopy(pointsToCopy);
      }
    else
      {
      newPoints = vtkPoints::New();
      }
    this->SetPoints(newPoints);
    newPoints->Delete();
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkPointSet::GetData(vtkInformation* info)
{
  return info? vtkPointSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkPointSet* vtkPointSet::GetData(vtkInformationVector* v, int i)
{
  return vtkPointSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Point Coordinates: " << this->Points << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
}

//----------------------------------------------------------------------------
void vtkPointSet::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkPointSet::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}
