/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOnCellBoundaries.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOnCellBoundaries - Iterate over cells of a dataset.
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#include "vtkBridgeCellIteratorOnCellBoundaries.h"

#include <cassert>

#include "vtkObjectFactory.h"
#include "vtkBridgeCell.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"
#include "vtkCell.h"
#include "vtkVertex.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkBridgeCellIteratorOnCellBoundaries);

//-----------------------------------------------------------------------------
void vtkBridgeCellIteratorOnCellBoundaries::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOnCellBoundaries::vtkBridgeCellIteratorOnCellBoundaries()
{
  this->DataSetCell=0;
  this->Cell=vtkBridgeCell::New();
  this->Id=0;
//  this->DebugOn();
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOnCellBoundaries::~vtkBridgeCellIteratorOnCellBoundaries()
{
  // warn: this class does not own this->DataSetCell, do never delete it.
  this->Cell->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgeCellIteratorOnCellBoundaries::Begin()
{
  this->Id=0; // first id of the current dimension
  if(this->NumberOfFaces>0)
    {
    this->Dim=2;
    }
  else
    {
    if(this->NumberOfEdges>0)
      {
      this->Dim=1;
      }
    else
      {
       if(this->NumberOfVertices>0)
         {
         this->Dim=0;
         }
       else
         {
         this->Dim=-1; // is at end
         }
      }
    }
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
int vtkBridgeCellIteratorOnCellBoundaries::IsAtEnd()
{
  return this->Dim==-1;
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void vtkBridgeCellIteratorOnCellBoundaries::GetCell(vtkGenericAdaptorCell *c)
{
  assert("pre: not_at_end" && !IsAtEnd());
  assert("pre: c_exists" && c!=0);

  vtkBridgeCell *c2=static_cast<vtkBridgeCell *>(c);

  vtkCell *vc=0;

  switch(this->Dim)
    {
    case 2:
      vc=this->DataSetCell->Cell->GetFace(this->Id);
      break;
    case 1:
      vc=this->DataSetCell->Cell->GetEdge(this->Id);
      break;
    case 0:
      vc=vtkVertex::New();
      vc->Points->InsertNextPoint(this->DataSetCell->Cell->Points->GetPoint(this->Id));
      vc->PointIds->InsertNextId(0);
      break;
    default:
      assert("check: impossible case" && 0);
      break;
    }

  c2->InitWithCell(vc,this->Id); // this->Id unique?
  if(this->Dim==0)
    {
    vc->Delete();
    }
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
vtkGenericAdaptorCell *vtkBridgeCellIteratorOnCellBoundaries::GetCell()
{
  assert("pre: not_at_end" && !IsAtEnd());

  vtkCell *vc=0;

  switch(this->Dim)
    {
    case 2:
      vc=this->DataSetCell->Cell->GetFace(this->Id);
      break;
    case 1:
      vc=this->DataSetCell->Cell->GetEdge(this->Id);
      break;
    case 0:
      vc=vtkVertex::New();
      vc->Points->InsertNextPoint(this->DataSetCell->Cell->Points->GetPoint(this->Id));
      vc->PointIds->InsertNextId(0);
      break;
    default:
      assert("check: impossible case" && 0);
      break;
    }

  this->Cell->InitWithCell(vc,this->Id); // this->Id unique?
  if(this->Dim==0)
    {
    vc->Delete();
    }
  vtkGenericAdaptorCell *result=this->Cell;

  assert("post: result_exits" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void vtkBridgeCellIteratorOnCellBoundaries::Next()
{
  assert("pre: not_off" && !IsAtEnd());

  int atEndOfDimension=0;

  this->Id++; // next id of the current dimension

  switch(this->Dim)
    {
    case 2:
      atEndOfDimension=Id>=this->NumberOfFaces;
      break;
    case 1:
      atEndOfDimension=Id>=this->NumberOfEdges;
      break;
    case 0:
      atEndOfDimension=Id>=this->NumberOfVertices;
      break;
    default:
      assert("check: impossible case" && 0);
      break;
    }

  if(atEndOfDimension)
    {
    this->Id=0; // first id of the next dimension
    this->Dim--;

    if(this->Dim==1)
      {
      if(this->NumberOfEdges==0)
        {
        if(this->NumberOfVertices==0)
          {
          this->Dim=-1;
          }
        else
          {
          this->Dim=0;
          }
        }
      }
    else
      {
      if(this->NumberOfVertices==0)
        {
        this->Dim=-1;
        }
      }
    }
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeCell.
// Iterate on boundary cells of a cell.
// \pre cell_exists: cell!=0
// \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))
void vtkBridgeCellIteratorOnCellBoundaries::InitWithCellBoundaries(vtkBridgeCell *cell,
                                                                   int dim)
{
  assert("pre: cell_exists" && cell!=0);
  assert("pre: valid_dim_range" && ((dim==-1) || ((dim>=0)&&(dim<cell->GetDimension()))));

  this->DataSetCell=cell;

  if(((dim==-1)&&(2<cell->GetDimension()))||(dim==2)) // faces
    {
    this->NumberOfFaces=this->DataSetCell->Cell->GetNumberOfFaces();
    }
  else
    {
    this->NumberOfFaces=0;
    }

  if(((dim==-1)&&(1<cell->GetDimension()))||(dim==1)) // edges
    {
    this->NumberOfEdges=this->DataSetCell->Cell->GetNumberOfEdges();
    }
  else
    {
    this->NumberOfEdges=0;
    }

  if((dim==-1)||(dim==0)) // vertices
    {
    this->NumberOfVertices=this->DataSetCell->Cell->GetNumberOfPoints();
    }
  else
    {
    this->NumberOfVertices=0;
    }
}
