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
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
  float *x;
 
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
      iMin = iMax = jMin = jMax = kMin = kMax = 0;
      cell = &vertex;
      break;

    case X_LINE:
      jMin = jMax = kMin = kMax = 0;
      iMin = cellId;
      iMax = cellId + 1;
      cell = &line;
      break;

    case Y_LINE:
      iMin = iMax = kMin = kMax = 0;
      jMin = cellId;
      jMax = cellId + 1;
      cell = &line;
      break;

    case Z_LINE:
      iMin = iMax = jMin = jMax = 0;
      kMin = cellId;
      kMax = cellId + 1;
      cell = &line;
      break;

    case XY_PLANE:
      kMin = kMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      cell = &quad;
      break;

    case YZ_PLANE:
      iMin = iMax = 0;
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      cell = &quad;
      break;

    case XZ_PLANE:
      jMin = jMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      cell = &quad;
      break;

    case XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      cell = &hexa;
      break;
    }

  // Extract point coordinates and point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        idx = loc[0] + loc[1]*this->Dimensions[0] + loc[2]*d01;
        x = this->Points->GetPoint(idx);
        cell->PointIds.InsertId(npts,idx);
        cell->Points.InsertPoint(npts++,x);
        }
      }
    }

  return cell;
}

void vlStructuredGrid::PrintSelf(ostream& os, vlIndent indent)
{
  vlPointSet::PrintSelf(os,indent);
//    vlStructuredData::PrintSelf(os,indent);
}

