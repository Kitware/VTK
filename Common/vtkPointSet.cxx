/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSet.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSet.h"

#include "vtkSource.h"
#include "vtkPointLocator.h"

vtkCxxRevisionMacro(vtkPointSet, "1.74");

vtkCxxSetObjectMacro(vtkPointSet,Points,vtkPoints);

vtkPointSet::vtkPointSet ()
{
  this->Points = NULL;
  this->Locator = NULL;
}

//----------------------------------------------------------------------------
vtkPointSet::~vtkPointSet ()
{
  this->Initialize();
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
  vtkPointSet *ps=(vtkPointSet *)ds;
  this->Initialize();

  this->SetPoints(ps->Points);
}


//----------------------------------------------------------------------------
void vtkPointSet::Initialize()
{
  vtkDataSet::Initialize();

  if ( this->Points ) 
    {
    this->Points->UnRegister(this);
    this->Points = NULL;
    }

  if ( this->Locator ) 
    {
    this->Locator->Initialize();
    }
}
//----------------------------------------------------------------------------
void vtkPointSet::ComputeBounds()
{
  float *bounds;

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
vtkIdType vtkPointSet::FindPoint(float x[3])
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

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindCell(float x[3], vtkCell *cell,
                                vtkGenericCell *gencell, vtkIdType cellId,
                                float tol2, int& subId, float pcoords[3],
                                float *weights)
{
  vtkIdType       ptId;
  int             walk;
  float           closestPoint[3];
  float           dist2;
  vtkIdList       *cellIds, *ptIds;
  int             initialCellProvided = 1;

  // make sure everything is up to snuff
  if ( !this->Points )
    {
    return -1;
    }

  cellIds = vtkIdList::New();
  cellIds->Allocate(8,100);
  ptIds = vtkIdList::New();
  ptIds->Allocate(8,100);

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

  // If we don't have a starting cell, we'll have to find one. Find
  // the closest point to the input position, then get the cells that use
  // the point.  Then use one of the cells to begin the walking process.
  if ( !cell )
    {
    initialCellProvided = 0;
    ptId = this->Locator->FindClosestPoint(x);
    if ( ptId < 0 )
      {
      cellIds->Delete();
      ptIds->Delete();
      return (-1); //if point completely outside of data
      }

    this->GetPointCells(ptId, cellIds);
    if ( cellIds->GetNumberOfIds() > 0 )
      {
      cellId = cellIds->GetId(0); //arbitrarily use first cell in list
      if ( gencell )
        {
        this->GetCell(cellId, gencell);
        }
      else
        {
        cell = this->GetCell(cellId);
        }

      // See whether this randomly choosen cell contains the point      
      if ( ( gencell && 
             gencell->EvaluatePosition(x,closestPoint,subId,
                                       pcoords, dist2,weights) == 1
             && dist2 <= tol2 )  ||
           ( !gencell && 
             cell->EvaluatePosition(x,closestPoint,subId,
                                       pcoords, dist2,weights) == 1
             && dist2 <= tol2 ) )
        {
        cellIds->Delete();
        ptIds->Delete();  
        return cellId;
        }
      }
    }
  else //EvaluatePosition insures that pcoords is defined
    {
    cell->EvaluatePosition(x,NULL,subId,pcoords,dist2,weights);
    }
  
  // If a cell is supplied, or we didn't find a starting cell (in the
  // previous chunk of code), then we use this to start our search. A
  // walking scheme is used, where we walk towards the point and eventually
  // locate the cell that contains the point.
  if ( cell || cellIds->GetNumberOfIds() > 0 ) //we have a starting cell
    {
    for ( walk=0; walk < VTK_MAX_WALK; walk++ )
      {
      if ( cell )
        {
        cell->CellBoundary(subId, pcoords, ptIds);
        }
      else
        {
        gencell->CellBoundary(subId, pcoords, ptIds);
        }
      this->GetCellNeighbors(cellId, ptIds, cellIds);
      if ( cellIds->GetNumberOfIds() > 0 )
        {
        cellId = cellIds->GetId(0);
        if ( gencell )
          {
          cell = NULL;
          this->GetCell(cellId, gencell);
          }
        else
          {
          cell = this->GetCell(cellId);
          }
        }
      else
        {
        break; //outside of data
        }

      if ( ( (!cell && 
              gencell->EvaluatePosition(x,closestPoint,subId,pcoords,
                                        dist2,weights) == 1 ) ||
             (cell && cell->EvaluatePosition(x,closestPoint,
                                                 subId,pcoords,
                                                 dist2,weights) == 1 ) )
           && dist2 <= tol2 )
        {
        cellIds->Delete();
        ptIds->Delete();  
        return cellId;
        }

      }//for a walk
    }//if we have a starting cell

  cellIds->Delete();
  ptIds->Delete();

  //sometimes the initial cell is a really bad guess so we'll
  //just ignore it and start from scratch as a last resort
  if ( initialCellProvided )
    {
    return this->FindCell(x, NULL, gencell, cellId, tol2,
                          subId, pcoords, weights);
    }
  else
    {
    return -1;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkPointSet::FindCell(float x[3], vtkCell *cell, vtkIdType cellId,
                                float tol2, int& subId, float pcoords[3],
                                float *weights)
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
void vtkPointSet::UnRegister(vtkObject *o)
{
  // detect the circular loop source <-> data
  // If we have two references and one of them is my data
  // and I am not being unregistered by my data, break the loop.
  if (this->ReferenceCount == 2 && this->Source != NULL &&
      o != this->Source && this->Source->InRegisterLoop(this))
    {
    this->SetSource(NULL);
    }
  // detect the circular loop PointSet <-> Locator
  // If we have two references and one of them is my locator
  // and I am not being unregistered by my locator, break the loop.
  if (this->ReferenceCount == 2 && this->Locator &&
      this->Locator->GetDataSet() == this && 
      this->Locator != o)
    {
    this->Locator->SetDataSet(NULL);
    }
  // catch the case when both of the above are true
  if (this->ReferenceCount == 3 && this->Source != NULL &&
      o != this->Source && this->Source->InRegisterLoop(this) &&
      this->Locator &&
      this->Locator->GetDataSet() == this && 
      this->Locator != o)
    {
    this->SetSource(NULL);
    if (this->Locator)
      {
      this->Locator->SetDataSet(NULL);
      }
    }  
  
  this->vtkObject::UnRegister(o);
}


//----------------------------------------------------------------------------
int vtkPointSet::GetNetReferenceCount()
{
  if (this->Locator && this->Locator->GetDataSet() == this)
    {    
    return this->ReferenceCount - 1;
    }
  return this->ReferenceCount;
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
    if (this->Points == NULL)
      {
      if ( pointSet->GetPoints() != NULL )
        {
        this->Points = pointSet->GetPoints()->MakeObject();
        this->Points->Register(this);
        this->Points->Delete();
        }
      else
        {
        this->Points = vtkPoints::New();
        this->Points->Register(this);
        this->Points->Delete();
        }
      }
    this->Points->DeepCopy(pointSet->GetPoints());
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Point Coordinates: " << this->Points << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
}

