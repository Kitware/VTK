/*=========================================================================

  Program:   Visualization Library
  Module:    UGrid.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "UGrid.hh"
#include "Point.hh"
#include "PolyPts.hh"
#include "Line.hh"
#include "PolyLine.hh"
#include "Triangle.hh"
#include "TriStrip.hh"
#include "Quad.hh"
#include "Rect.hh"
#include "Polygon.hh"
#include "Tetra.hh"
#include "Hexa.hh"
#include "Brick.hh"

vlUnstructuredGrid::vlUnstructuredGrid ()
{
  this->Connectivity = 0;
  this->Cells = 0;
  this->Links = 0;
}

vlUnstructuredGrid::vlUnstructuredGrid(const vlUnstructuredGrid& pd)
{
  this->Connectivity = pd.Connectivity;
  if (this->Connectivity) this->Connectivity->Register((void *)this);

  this->Cells = pd.Cells;
  if (this->Cells) this->Cells->Register((void *)this);

  this->Links = pd.Links;
  if (this->Links) this->Links->Register((void *)this);
}

vlUnstructuredGrid::~vlUnstructuredGrid()
{
  vlUnstructuredGrid::Initialize();
}

void vlUnstructuredGrid::Initialize()
{
  vlPointSet::Initialize();

  if ( this->Connectivity )
  {
    this->Connectivity->UnRegister((void *)this);
    this->Connectivity = 0;
  }

  if ( this->Cells )
  {
    this->Cells->UnRegister((void *)this);
    this->Cells = 0;
  }

  if ( this->Links )
  {
    this->Links->UnRegister((void *)this);
    this->Links = 0;
  }
};

vlCell *vlUnstructuredGrid::GetCell(int cellId)
{
  static vlPoint point;
  static vlPolyPoints ppoints;
  static vlLine line;
  static vlPolyLine pline;
  static vlTriangle triangle;
  static vlTriangleStrip strip;
  static vlPolygon poly;
  static vlRectangle rect;
  static vlQuad quad;
  static vlTetra tetra;
  static vlBrick brick;
  static vlHexahedron hexa;
  int i, loc, numPts, *pts;
  vlCell *cell;

  switch (this->Cells->GetCellType(cellId))
    {
    case vlPOINT:
     cell = &point;
     break;

    case vlPOLY_POINTS:
     cell = &ppoints;
     break;

    case vlLINE: 
      cell = &line;
      break;

    case vlPOLY_LINE:
      cell = &pline;
      break;

    case vlTRIANGLE:
      cell = &triangle;
      break;

    case vlTRIANGLE_STRIP:
      cell = &strip;
      break;

    case vlRECTANGLE:
      cell = &rect;
      break;

    case vlQUAD:
      cell = &quad;
      break;

    case vlPOLYGON:
      cell = &poly;
      break;

    case vlTETRA:
      cell = &tetra;
      break;

    case vlBRICK:
      cell = &brick;
      break;

    case vlHEXAHEDRON:
      cell = &hexa;
      break;
    }

  loc = this->Cells->GetCellLocation(cellId);
  this->Connectivity->GetCell(loc,numPts,pts); 

  for (i=0; i<numPts; i++)
    {
    cell->PointIds.SetId(i,pts[i]);
    cell->Points.SetPoint(i,this->Points->GetPoint(i));
    }

  return cell;
}

int vlUnstructuredGrid::GetNumberOfCells() 
{
  return (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0);
}

void vlUnstructuredGrid::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlUnstructuredGrid::GetClassName()))
    {
    vlDataSet::PrintSelf(os,indent);
    }
}

void vlUnstructuredGrid::InsertNextCell(vlCell *cell)
{

}

void vlUnstructuredGrid::InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE])
{
  this->Connectivity->InsertNextCell(npts,pts);
  this->Cells->InsertNextCell(type,this->Connectivity->GetLocation(npts));
}

void vlUnstructuredGrid::InsertCells(int numCells, int width, int *data)
{

}

void vlUnstructuredGrid::InsertCells(int numCells, int *data)
{

}

void vlUnstructuredGrid::BuildLinks()
{
  this->Links = new vlLinkList(this->GetNumberOfPoints());
  this->Links->BuildLinks(this);
}

void vlUnstructuredGrid::GetPointCells(int ptId, vlIdList *cellIds)
{
  int *cells;
  int numCells;
  int i;

  if ( ! this->Links ) this->BuildLinks();
  cellIds->Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  for (i=0; i < numCells; i++)
    {
    cellIds->InsertId(i,cells[i]);
    }
}



