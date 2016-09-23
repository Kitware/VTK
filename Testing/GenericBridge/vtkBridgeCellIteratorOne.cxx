/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOne.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOne - Iterate over cells of a dataset.
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#include "vtkBridgeCellIteratorOne.h"

#include <cassert>

#include "vtkObjectFactory.h"
#include "vtkBridgeCell.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"

#include "vtkTriangle.h"
#include "vtkPolygon.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkVertex.h"
#include "vtkPolyVertex.h"

vtkStandardNewMacro(vtkBridgeCellIteratorOne);

//-----------------------------------------------------------------------------
void vtkBridgeCellIteratorOne::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOne::vtkBridgeCellIteratorOne()
{
  this->DataSet=0;
  this->InternalCell=0;
  this->Cell=0;
  this->Id=0;
  this->cIsAtEnd=0;
//  this->DebugOn();
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOne::~vtkBridgeCellIteratorOne()
{
  if((this->Cell!=0)&&((this->DataSet!=0)||(this->InternalCell!=0)))
  {
    // dataset mode or points mode
    this->Cell->Delete();
    this->Cell=0;
  }
  if(this->DataSet!=0)
  {
    this->DataSet->Delete();
    this->DataSet=0;
  }

  if(this->InternalCell!=0)
  {
    this->InternalCell->Delete();
    this->InternalCell=0;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgeCellIteratorOne::Begin()
{
  this->cIsAtEnd=0;
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
int vtkBridgeCellIteratorOne::IsAtEnd()
{
  return this->cIsAtEnd;
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void vtkBridgeCellIteratorOne::GetCell(vtkGenericAdaptorCell *c)
{
  assert("pre: not_at_end" && !this->IsAtEnd());
  assert("pre: c_exists" && c!=0);

  vtkBridgeCell *c2=static_cast<vtkBridgeCell *>(c);
  if(this->DataSet!=0)
  {
    c2->Init(this->DataSet,this->Id);
  }
  else
  {
    if(this->InternalCell!=0)
    {
      c2->InitWithCell(this->InternalCell,this->Id);
    }
    else
    {
      c2->DeepCopy(this->Cell);
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
vtkGenericAdaptorCell *vtkBridgeCellIteratorOne::GetCell()
{
  assert("pre: not_at_end" && !this->IsAtEnd());

  vtkGenericAdaptorCell *result=this->Cell;

  assert("post: result_exits" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void vtkBridgeCellIteratorOne::Next()
{
  assert("pre: not_off" && !this->IsAtEnd());

  this->cIsAtEnd=1;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeDataSet.
// Iterate on one cell `id' of `ds'.
// \pre ds_exists: ds!=0
// \pre valid_id: (id>=0)&&(id<=ds->GetNumberOfCells())
void vtkBridgeCellIteratorOne::InitWithOneCell(vtkBridgeDataSet *ds,
                                               vtkIdType cellid)
{
  assert("pre: ds_exists" && ds!=0);
  assert("pre: valid_id" && ((cellid>=0)&&(cellid<=ds->GetNumberOfCells())));

  if((this->Cell!=0)&&(this->DataSet==0)&&(this->InternalCell==0))
  {
    // previous mode was InitWithOneCell(vtkBridgeCell *c)
//    this->Cell->Delete();
    this->Cell=0;
  }

  if(this->Cell==0)
  {
    // first init or previous mode was InitWithOneCell(vtkBridgeCell *c)
    this->Cell=vtkBridgeCell::New();
  }

  vtkSetObjectBodyMacro(InternalCell,vtkCell,0);
  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,ds);
  this->Id=cellid;
  this->cIsAtEnd=1;
  this->Cell->Init(this->DataSet,this->Id);
}
//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeCell.
// Iterate on one cell `c'.
// \pre c_exists: c!=0
void vtkBridgeCellIteratorOne::InitWithOneCell(vtkBridgeCell *c)
{
  assert("pre: c_exists" && c!=0);

  if((this->Cell!=0)&&((this->DataSet!=0)||(this->InternalCell!=0)))
  {
    // dataset mode or points mode
    this->Cell->Delete();
  }
  vtkSetObjectBodyMacro(InternalCell,vtkCell,0);
  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);

  this->Cell=c; // no register to prevent reference cycle with vtkBridgeCell
  this->Id=c->GetId();
  this->cIsAtEnd=1;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeCell.
// Iterate on a boundary cell (defined by its points `pts' with coordinates
// `coords', dimension `dim' and unique id `cellid') of a cell.
// \pre coords_exist: coords!=0
// \pre pts_exist: pts!=0
// \pre valid_dim: dim>=0 && dim<=2
// \pre valid_points: pts->GetNumberOfIds()>dim
void vtkBridgeCellIteratorOne::InitWithPoints(vtkPoints *coords,
                                              vtkIdList *pts,
                                              int dim,
                                              vtkIdType cellid)
{
  assert("pre: coords_exist" && coords!=0);
  assert("pre: pts_exist" && pts!=0);
  assert("pre: valid_dim" && dim>=0 && dim<=2);
  assert("pre: valid_points" && pts->GetNumberOfIds()>dim);


  if((this->DataSet==0)&&(this->InternalCell==0))
  {
    // previous mode was InitWithOneCell(vtkBridgeCell *c)
//    this->Cell->Delete();
    this->Cell=0;
  }

  if(this->Cell==0)
  {
    // first init or previous mode was InitWithOneCell(vtkBridgeCell *c)
    this->Cell=vtkBridgeCell::New();
  }

  vtkCell *cell=0;

  switch(dim)
  {
    case 2:
      if (pts->GetNumberOfIds()==3)
      {
        cell=vtkTriangle::New();
      }
      else
      {
        cell=vtkPolygon::New();
      }
      break;
    case 1:
      // line or polyline
      if(pts->GetNumberOfIds()==2)
      {
        cell=vtkLine::New();
      }
      else
      {
        cell=vtkPolyLine::New();
      }
      break;
    case 0:
      // vertex polyvertex
      if(pts->GetNumberOfIds()==1)
      {
        cell=vtkVertex::New();
      }
      else
      {
        cell=vtkPolyVertex::New();
      }
      break;
  }
  cell->Points=coords;
  cell->PointIds=pts;
  vtkSetObjectBodyMacro(InternalCell,vtkCell,cell);
  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
  this->Id=cellid;
  this->cIsAtEnd=1;
  this->Cell->InitWithCell(this->InternalCell,this->Id);
}
