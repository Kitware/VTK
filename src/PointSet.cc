/*=========================================================================

  Program:   Visualization Library
  Module:    PointSet.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PointSet.hh"

vlPointSet::vlPointSet ()
{
  this->Points = NULL;
  this->Locator = NULL;
}

vlPointSet::vlPointSet(const vlPointSet& ps) :
vlDataSet(ps)
{
  this->Points = ps.Points;
  if (this->Points) this->Points->Register(this);

  this->Locator = ps.Locator;
  if (this->Locator) this->Locator->Register(this);
}

void vlPointSet::Initialize()
{
  vlDataSet::Initialize();

  if ( this->Points ) 
  {
    this->Points->UnRegister(this);
    this->Points = NULL;
  }

  if ( this->Locator ) 
  {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
  }
}
void vlPointSet::ComputeBounds()
{
  float *bounds;

  if ( this->Points )
    {
    bounds = this->Points->GetBounds();
    for (int i=0; i<6; i++) this->Bounds[i] = bounds[i];
    this->ComputeTime.Modified();
    }
}

unsigned long int vlPointSet::GetMTime()
{
  unsigned long int dsTime = vlDataSet::GetMTime();

  if ( this->Points ) 
    {
    if ( this->Points->GetMTime() > dsTime ) dsTime = this->Points->GetMTime();
    }

  if ( this->Locator )
    {
    if ( this->Locator->GetMTime() > dsTime ) dsTime = this->Locator->GetMTime();
    }

  return dsTime;
}

void vlPointSet::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPointSet::GetClassName()))
    {
    vlDataSet::PrintSelf(os,indent);
    
    os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
    os << indent << "Point Data: " << this->Points;
    os << indent << "Locator: " << this->Locator;
    }
}

int vlPointSet::FindCell(float x[3], vlCell *cell, float tol2, int& subId,
                         float pcoords[3])
{
  int i;
  int closestCell = -1;
  vlCell *cell;
  int ptId, cellId;
  float dist2;
  static vlIdList cellIds(MAX_CELL_SIZE);
  int sId;
  float pc[3], weights[MAX_CELL_SIZE];
  float closestPoint[3];

  if ( !this->Points ) return -1;

  if ( !this->Locator )
    {
    this->Locator = new vlLocator;
    this->Locator->SetPoints(this->Points);
    }

  if ( this->Points->GetMTime() > this->Locator->GetMTime() )
    {
    this->Locator->SetPoints(this->Points);
    }

// Find the closest point to the input position.  Then get the cells that 
// use the point.  Then determine if point is in any of the cells.

  if ( (ptId = this->Locator->FindClosestPoint(x)) >= 0 )
    {
    this->GetPointCells(ptId, cellIds);
    for (i=0; i<cellIds.GetNumberOfIds(); i++)
      {
      cellId = cellIds.GetId(i);
      cell = this->GetCell(cellId);
      cell->EvaluatePosition(x,closestPoint,sId,pc,dist2,weights);
      if ( dist2 <= tol2 )
        {
        closestCell = cellId;
        subId = sId;
        pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = pc[2]; 
        }
      }
    }
  return closestCell;
}

void vlPointSet::Squeeze()
{
  if ( this->Points ) this->Points->Squeeze();
  vlDataSet::Squeeze();
}
