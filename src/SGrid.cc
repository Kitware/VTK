/*=========================================================================

  Program:   Visualization Library
  Module:    SGrid.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SGrid.hh"
#include "Vertex.hh"
#include "Line.hh"
#include "Quad.hh"
#include "Hexa.hh"

vlStructuredGrid::vlStructuredGrid()
{
}

vlStructuredGrid::vlStructuredGrid(const vlStructuredGrid& sg) :
vlStructuredData(sg), vlPointSet(sg)
{
}

vlStructuredGrid::~vlStructuredGrid()
{
  this->Initialize();
}

unsigned long vlStructuredGrid::GetMtime()
{
  unsigned long dtime = this->vlPointSet::GetMTime();
  unsigned long ftime = this->vlStructuredData::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredGrid::Initialize()
{
  vlPointSet::Initialize(); 
  vlStructuredData::_Initialize();
}

int vlStructuredGrid::GetCellType(int cellId)
{
  switch (this->DataDescription)
    {
    case SINGLE_POINT: 
      return vlVERTEX;

    case X_LINE: case Y_LINE: case Z_LINE:
      return vlLINE;

    case XY_PLANE: case YZ_PLANE: case XZ_PLANE:
      return vlQUAD;

    case XYZ_GRID:
      return vlHEXAHEDRON;

    default:
      vlErrorMacro(<<"Bad data description!");
      return vlNULL_ELEMENT;
    }
}

vlCell *vlStructuredGrid::GetCell(int cellId)
{
  static vlVertex vertex;
  static vlLine line;
  static vlQuad quad;
  static vlHexahedron hexa;
  static vlCell *cell;
  int idx;
  int i, j, k;
  int d01, offset1, offset2;
 
  // Make sure data is defined
  if ( ! this->Points )
    {
    vlErrorMacro (<<"No data");
    return NULL;
    }

  // 
  switch (this->DataDescription)
    {
    case SINGLE_POINT: // cellId can only be = 0
      cell = &vertex;
      cell->PointIds.InsertId(0,0);
      break;

    case X_LINE:
      cell = &line;
      cell->PointIds.InsertId(0,cellId);
      cell->PointIds.InsertId(1,cellId+1);
      break;

    case Y_LINE:
      cell = &line;
      cell->PointIds.InsertId(0,cellId);
      cell->PointIds.InsertId(1,cellId+1);
      break;

    case Z_LINE:
      cell = &line;
      cell->PointIds.InsertId(0,cellId);
      cell->PointIds.InsertId(1,cellId+1);
      break;

    case XY_PLANE:
      cell = &quad;
      i = cellId % (this->Dimensions[0]-1);
      j = cellId / (this->Dimensions[0]-1);
      idx = i + j*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      break;

    case YZ_PLANE:
      cell = &quad;
      j = cellId % (this->Dimensions[1]-1);
      k = cellId / (this->Dimensions[1]-1);
      idx = j + k*this->Dimensions[1];
      offset1 = 1;
      offset2 = this->Dimensions[1];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      break;

    case XZ_PLANE:
      cell = &quad;
      i = cellId % (this->Dimensions[0]-1);
      k = cellId / (this->Dimensions[0]-1);
      idx = i + k*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      break;

    case XYZ_GRID:
      cell = &hexa;
      d01 = this->Dimensions[0]*this->Dimensions[1];
      i = cellId % (this->Dimensions[0] - 1);
      j = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      k = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      idx = i+ j*this->Dimensions[0] + k*d01;
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      idx += d01;
      cell->PointIds.InsertId(4,idx);
      cell->PointIds.InsertId(5,idx+offset1);
      cell->PointIds.InsertId(6,idx+offset1+offset2);
      cell->PointIds.InsertId(7,idx+offset2);
      break;
    }

  // Extract point coordinates and point ids. NOTE: the ordering of the VlQuad
  // and vlHexahedron cells are tricky.
  for (i=0; i<cell->PointIds.GetNumberOfIds(); i++)
    {
    idx = cell->PointIds.GetId(i);
    cell->Points.InsertPoint(i,this->Points->GetPoint(idx));
    }

  return cell;
}

void vlStructuredGrid::PrintSelf(ostream& os, vlIndent indent)
{
  vlPointSet::PrintSelf(os,indent);
//    vlStructuredData::PrintSelf(os,indent);
}

