/*=========================================================================

  Program:   Visualization Library
  Module:    PolyData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PolyData.hh"
#include "Point.hh"
#include "PolyPts.hh"
#include "Line.hh"
#include "PolyLine.hh"
#include "Triangle.hh"
#include "TriStrip.hh"
#include "Quad.hh"
#include "Polygon.hh"
//
// Initialize static member.  This member is used to simplify traversal
// of verts, lines, polygons, and triangle strips lists.  It basically 
// "marks" empty lists so that the traveral method "GetNextCell" 
// works properly.
//
vlCellArray *vlPolyData::Dummy = NULL;

vlPolyData::vlPolyData ()
{
  this->Verts = NULL;
  this->Lines = NULL;
  this->Polys = NULL;
  this->Strips = NULL;

  // static variable, initialized only once.
  if (!this->Dummy) this->Dummy = new vlCellArray;

  this->Cells = NULL;
  this->Links = NULL;
}

vlPolyData::vlPolyData(const vlPolyData& pd) :
vlPointSet(pd)
{
  this->Verts = pd.Verts;
  if (this->Verts) this->Verts->Register(this);

  this->Lines = pd.Lines;
  if (this->Lines) this->Lines->Register(this);

  this->Polys = pd.Polys;
  if (this->Polys) this->Polys->Register(this);

  this->Strips = pd.Strips;
  if (this->Strips) this->Strips->Register(this);
 
  this->Cells = pd.Cells;
  if (this->Cells) this->Cells->Register(this);

  this->Links = pd.Links;
  if (this->Links) this->Links->Register(this);
}

vlPolyData::~vlPolyData()
{
  vlPolyData::Initialize();
}

int vlPolyData::GetCellType(int cellId)
{
  if ( !this->Cells ) this->BuildCells();
  return this->Cells->GetCellType(cellId);
}

vlCell *vlPolyData::GetCell(int cellId)
{
  static vlPoint point;
  static vlPolyPoints ppoints;
  static vlLine line;
  static vlPolyLine pline;
  static vlTriangle triangle;
  static vlTriangleStrip strip;
  static vlPolygon poly;
  static vlQuad quad;
  int i, loc, numPts, *pts;
  vlCell *cell;
  unsigned char type;

  if ( !this->Cells ) this->BuildCells();

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case vlPOINT:
     cell = &point;
     this->Verts->GetCell(loc,numPts,pts);
     break;

    case vlPOLY_POINTS:
     cell = &ppoints;
     this->Verts->GetCell(loc,numPts,pts);
     break;

    case vlLINE: 
      cell = &line;
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case vlPOLY_LINE:
      cell = &pline;
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case vlTRIANGLE:
      cell = &triangle;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case vlQUAD:
      cell = &quad;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case vlPOLYGON:
      cell = &poly;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case vlTRIANGLE_STRIP:
      cell = &strip;
      this->Strips->GetCell(loc,numPts,pts);
      break;
    }
  for (i=0; i<numPts; i++)
    {
    cell->PointIds.SetId(i,pts[i]);
    cell->Points.SetPoint(i,this->Points->GetPoint(pts[i]));
    }

  return cell;

}

void vlPolyData::SetVerts (vlCellArray* v) 
{
  if ( v != this->Verts && v != this->Dummy )
    {
    if (this->Verts) this->Verts->UnRegister(this);
    this->Verts = v;
    if (this->Verts) this->Verts->Register(this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetVerts()
{
  if ( !this->Verts ) return this->Dummy;
  else return this->Verts;
}

void vlPolyData::SetLines (vlCellArray* l) 
{
  if ( l != this->Lines && l != this->Dummy )
    {
    if (this->Lines) this->Lines->UnRegister(this);
    this->Lines = l;
    if (this->Lines) this->Lines->Register(this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetLines()
{
  if ( !this->Lines ) return this->Dummy;
  else return this->Lines;
}

void vlPolyData::SetPolys (vlCellArray* p) 
{
  if ( p != this->Polys && p != this->Dummy )
    {
    if (this->Polys) this->Polys->UnRegister(this);
    this->Polys = p;
    if (this->Polys) this->Polys->Register(this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetPolys()
{
  if ( !this->Polys ) return this->Dummy;
  else return this->Polys;
}

void vlPolyData::SetStrips (vlCellArray* s) 
{
  if ( s != this->Strips && s != this->Dummy )
    {
    if (this->Strips) this->Strips->UnRegister(this);
    this->Strips = s;
    if (this->Strips) this->Strips->Register(this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetStrips()
{
  if ( !this->Strips ) return this->Dummy;
  else return this->Strips;
}

void vlPolyData::Initialize()
{
  vlPointSet::Initialize();

  if ( this->Verts ) 
  {
    this->Verts->UnRegister(this);
    this->Verts = NULL;
  }

  if ( this->Lines ) 
  {
    this->Lines->UnRegister(this);
    this->Lines = NULL;
  }

  if ( this->Polys ) 
  {
    this->Polys->UnRegister(this);
    this->Polys = NULL;
  }

  if ( this->Strips ) 
  {
    this->Strips->UnRegister(this);
    this->Strips = NULL;
  }

  if ( this->Cells )
  {
    this->Cells->UnRegister(this);
    this->Cells = NULL;
  }

  if ( this->Links )
  {
    this->Links->UnRegister(this);
    this->Links = NULL;
  }

};

int vlPolyData::GetNumberOfCells() 
{
  return GetNumberOfVerts() + GetNumberOfLines() + 
         GetNumberOfPolys() + GetNumberOfStrips();
}

int vlPolyData::GetNumberOfVerts() 
{
  return (this->Verts ? this->Verts->GetNumberOfCells() : 0);
}

int vlPolyData::GetNumberOfLines() 
{
  return (this->Lines ? this->Lines->GetNumberOfCells() : 0);
}

int vlPolyData::GetNumberOfPolys() 
{
  return (this->Polys ? this->Polys->GetNumberOfCells() : 0);
}

int vlPolyData::GetNumberOfStrips() 
{
  return (this->Strips ? this->Strips->GetNumberOfCells() : 0);
}

void vlPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPolyData::GetClassName()))
    {
    vlPointSet::PrintSelf(os,indent);
    
    os << indent << "Number Of Vertices: " << this->GetNumberOfVerts() << "\n";
    os << indent << "Number Of Lines: " << this->GetNumberOfLines() << "\n";
    os << indent << "Number Of Polygons: " << this->GetNumberOfPolys() << "\n";
    os << indent << "Number Of Triangle Strips: " << this->GetNumberOfStrips() << "\n";
    }
}

void vlPolyData::BuildCells()
{
  int numCells=0;
  vlCellArray *inVerts=this->GetVerts();
  vlCellArray *inLines=this->GetLines();
  vlCellArray *inPolys=this->GetPolys();
  vlCellArray *inStrips=this->GetStrips();
  int i, j;
  int npts, *pts;
  vlIdList outVerts(3*MAX_CELL_SIZE);
  vlCellList *cells;
  vlPoints *inPoints=this->GetPoints();

  vlDebugMacro (<< "Building PolyData cells.");

  numCells = this->GetNumberOfCells();

  if ( inPoints == NULL || numCells < 1 ) 
    {
    vlErrorMacro (<< "No data to build");
    return;
    }
  else
    {
    this->Cells = cells = new vlCellList(numCells,3*numCells);
    this->Cells->Register(this);
    }
//
// Traverse various lists to create cell array
//
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
    {
    if ( npts > 1 )
      cells->InsertNextCell(vlPOLY_POINTS,inVerts->GetLocation(npts));
    else
      cells->InsertNextCell(vlPOINT,inVerts->GetLocation(npts));
    }

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    if ( npts > 2 )
      cells->InsertNextCell(vlPOLY_LINE,inLines->GetLocation(npts));
    else
      cells->InsertNextCell(vlLINE,inLines->GetLocation(npts));
    }

  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    if ( npts == 3 )
      cells->InsertNextCell(vlTRIANGLE,inPolys->GetLocation(npts));
    else if ( npts == 4 )
      cells->InsertNextCell(vlQUAD,inPolys->GetLocation(npts));
    else
      cells->InsertNextCell(vlPOLYGON,inPolys->GetLocation(npts));
    }

  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    cells->InsertNextCell(vlTRIANGLE_STRIP,inStrips->GetLocation(npts));
    }
}

void vlPolyData::BuildLinks()
{
  if ( this->Cells == NULL ) this->BuildCells();
  this->Links = new vlLinkList(this->GetNumberOfPoints());
  this->Links->Register(this);

  this->Links->BuildLinks(this);
}

void vlPolyData::GetCellPoints(int cellId, vlIdList& ptIds)
{
  int i, loc, numPts, *pts;
  unsigned char type;

  ptIds.Reset();
  if ( this->Cells == NULL ) this->BuildCells();

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case vlPOINT: case vlPOLY_POINTS:
     this->Verts->GetCell(loc,numPts,pts);
     break;

    case vlLINE: case vlPOLY_LINE:
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case vlTRIANGLE: case vlQUAD: case vlPOLYGON:
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case vlTRIANGLE_STRIP:
      this->Strips->GetCell(loc,numPts,pts);
      break;
    }
  for (i=0; i<numPts; i++) ptIds.SetId(i,pts[i]);
}

void vlPolyData::GetPointCells(int ptId, vlIdList& cellIds)
{
  int *cells;
  int numCells;
  int i;

  if ( ! this->Links ) this->BuildLinks();
  cellIds.Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  for (i=0; i < numCells; i++)
    {
    cellIds.InsertId(i,cells[i]);
    }
}


void vlPolyData::InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE])
{


  switch (type)
    {
    case vlPOINT: case vlPOLY_POINTS:
      if ( this->Verts == NULL ) // hasn't been initialized
        {
        this->SetVerts(new vlCellArray(1000,1000));
        }
      this->Verts->InsertNextCell(npts,pts);
      break;

    case vlLINE: case vlPOLY_LINE:
      if ( this->Lines == NULL ) // hasn't been initialized
        {
        this->SetLines(new vlCellArray(1000,1000));
        }
      this->Lines->InsertNextCell(npts,pts);
      break;

    case vlTRIANGLE: case vlQUAD: case vlPOLYGON:
      if ( this->Polys == NULL ) // hasn't been initialized
        {
        this->SetPolys(new vlCellArray(1000,1000));
        }
      this->Polys->InsertNextCell(npts,pts);
      break;

    case vlTRIANGLE_STRIP:
      if ( this->Strips == NULL ) // hasn't been initialized
        {
        this->SetStrips(new vlCellArray(1000,1000));
        }
      this->Strips->InsertNextCell(npts,pts);
      break;

    default:
      vlErrorMacro(<<"Bad cell type! Can't insert!");
    }
}

void vlPolyData::Squeeze()
{
  if ( this->Verts != NULL ) this->Verts->Squeeze();
  if ( this->Lines != NULL ) this->Lines->Squeeze();
  if ( this->Polys != NULL ) this->Polys->Squeeze();
  if ( this->Strips != NULL ) this->Strips->Squeeze();

  vlPointSet::Squeeze();
}

void vlPolyData::ReverseCell(int cellId)
{
  int loc, type;

  if ( this->Cells == NULL ) this->BuildCells();
  loc = this->Cells->GetCellLocation(cellId);
  type = this->Cells->GetCellType(cellId);

  switch (type)
    {
    case vlPOINT: case vlPOLY_POINTS:
     this->Verts->ReverseCell(loc);
     break;

    case vlLINE: case vlPOLY_LINE:
      this->Lines->ReverseCell(loc);
      break;

    case vlTRIANGLE: case vlQUAD: case vlPOLYGON:
      this->Polys->ReverseCell(loc);
      break;

    case vlTRIANGLE_STRIP:
      this->Strips->ReverseCell(loc);
      break;
    }
}

void vlPolyData::ReplaceCell(int cellId, vlIdList& ptIds)
{
  int loc, type;

  if ( this->Cells == NULL ) this->BuildCells();
  loc = this->Cells->GetCellLocation(cellId);
  type = this->Cells->GetCellType(cellId);

  switch (type)
    {
    case vlPOINT: case vlPOLY_POINTS:
     this->Verts->ReplaceCell(loc,ptIds);
     break;

    case vlLINE: case vlPOLY_LINE:
      this->Lines->ReplaceCell(loc,ptIds);
      break;

    case vlTRIANGLE: case vlQUAD: case vlPOLYGON:
      this->Polys->ReplaceCell(loc,ptIds);
      break;

    case vlTRIANGLE_STRIP:
      this->Strips->ReplaceCell(loc,ptIds);
      break;
    }
}

