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
#include "PolyMap.hh"
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

  this->LoadVerts = 1;
  this->LoadLines = 1;
  this->LoadPolys = 1;
  this->LoadStrips = 1;

  this->TriangleMesh = 0;
  this->Writable = 0;

  this->Cells = NULL;
  this->Links = NULL;
}

vlPolyData::vlPolyData(const vlPolyData& pd)
{
  this->Verts = pd.Verts;
  if (this->Verts) this->Verts->Register(this);

  this->Lines = pd.Lines;
  if (this->Lines) this->Lines->Register(this);

  this->Polys = pd.Polys;
  if (this->Polys) this->Polys->Register(this);

  this->Strips = pd.Strips;
  if (this->Strips) this->Strips->Register(this);
 
  this->LoadVerts = pd.LoadVerts;
  this->LoadLines = pd.LoadLines;
  this->LoadPolys = pd.LoadPolys;
  this->LoadStrips = pd.LoadStrips;

  this->TriangleMesh = pd.TriangleMesh;
  this->Writable = pd.Writable;

  this->Cells = pd.Cells;
  if (this->Cells) this->Cells->Register(this);

  this->Links = pd.Links;
  if (this->Links) this->Links->Register(this);
}

vlPolyData::~vlPolyData()
{
  vlPolyData::Initialize();
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

vlMapper *vlPolyData::MakeMapper()
{
  vlPolyMapper *mapper;

  if ( this->Mapper == NULL  )
    {
    this->Mapper = mapper = new vlPolyMapper;
    this->Mapper->Register(this);
    mapper->SetInput(this);
    }
  return this->Mapper;
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
    os << indent << "Load Verts: " << (this->LoadVerts ? "On\n" : "Off\n");
    os << indent << "Load Lines: " << (this->LoadLines ? "On\n" : "Off\n");
    os << indent << "Load Polys: " << (this->LoadPolys ? "On\n" : "Off\n");
    os << indent << "Load Strips: " << (this->LoadStrips ? "On\n" : "Off\n");
    os << indent << "Triangle Mesh: " << (this->TriangleMesh ? "On\n" : "Off\n");
    os << indent << "Writable: " << (this->Writable ? "On\n" : "Off\n");
    }
}

void vlPolyData::BuildCells()
{
  int numCells=0;
  vlCellArray *inVerts=this->GetVerts();
  vlCellArray *inLines=this->GetLines();
  vlCellArray *inPolys=this->GetPolys();
  vlCellArray *inStrips=this->GetStrips();
  vlCellArray *verts=NULL;
  vlCellArray *lines=NULL;
  vlCellArray *polys=NULL;
  vlCellArray *strips=NULL;
  int i, j;
  int npts, *pts;
  int p1, p2, p3;
  vlPolygon poly;
  vlIdList outVerts(3*MAX_CELL_SIZE);
  vlCellList *cells;
  vlPoints *inPoints=this->GetPoints();

  vlDebugMacro (<< "Building PolyData cells.");

  if ( ! this->TriangleMesh )
    {
    if (this->LoadVerts) numCells += this->GetNumberOfVerts();
    if (this->LoadLines) numCells += this->GetNumberOfLines();
    if (this->LoadPolys) numCells += this->GetNumberOfPolys();
    if (this->LoadStrips) numCells += this->GetNumberOfStrips();
    }
  else
    {
    // an initial estimate
    numCells += this->GetNumberOfPolys();
    numCells += this->GetNumberOfStrips();
    }

  if ( ! inPoints || numCells < 1 ) 
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
// If we are just reading the data structure, can use the input data
// without any copying or allocation.  Otherwise have to allocate
// working storage that is a copy of the input data. Triangle meshes always
// require new storage.
//
  if ( ! this->Writable && ! this->TriangleMesh ) 
    {
    verts = inVerts;
    lines = inLines;
    polys = inPolys;
    strips = inStrips;
    }
  else if ( this->Writable && ! this->TriangleMesh )
    {
    if (this->LoadVerts && inVerts) verts = new vlCellArray(*inVerts);
    if (this->LoadLines && inLines) lines = new vlCellArray(*inLines);
    if (this->LoadPolys && inPolys) polys = new vlCellArray(*inPolys);
    if (this->LoadStrips && inStrips) strips = new vlCellArray(*inStrips);
    }
  else
    {
    polys = new vlCellArray();
    polys->Allocate(polys->EstimateSize(numCells,3),3*numCells);
    }
//
// Update ourselves
//
  this->SetVerts(verts);
  this->SetLines(lines);
  this->SetPolys(polys);
  this->SetStrips(strips);
//
// Now traverse various lists to create cell array
//
  if ( ! this->TriangleMesh )
    {
    if ( verts )
      {
      for (verts->InitTraversal(); verts->GetNextCell(npts,pts); )
        {
        if ( npts > 1 )
          cells->InsertNextCell(vlPOLY_POINTS,verts->GetLocation(npts));
        else
          cells->InsertNextCell(vlPOINT,verts->GetLocation(npts));
        }
      }

    if ( lines )
      {
      for (lines->InitTraversal(); lines->GetNextCell(npts,pts); )
        {
        if ( npts > 1 )
          cells->InsertNextCell(vlPOLY_LINE,lines->GetLocation(npts));
        else
          cells->InsertNextCell(vlLINE,lines->GetLocation(npts));
        }
      }

    if ( polys )
      {
      for (polys->InitTraversal(); polys->GetNextCell(npts,pts); )
        {
        if ( npts == 3 )
          cells->InsertNextCell(vlTRIANGLE,polys->GetLocation(npts));
        else if ( npts == 4 )
          cells->InsertNextCell(vlQUAD,polys->GetLocation(npts));
        else
          cells->InsertNextCell(vlPOLYGON,polys->GetLocation(npts));
        }
      }

    if ( strips )
      {
      for (strips->InitTraversal(); strips->GetNextCell(npts,pts); )
        {
        cells->InsertNextCell(vlTRIANGLE_STRIP,strips->GetLocation(npts));
        }
      }
    }
//
// If triangle mesh, convert polygons and triangle strips to triangles.
//
  else
    {
    if ( inPolys )
      {
      for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
        {
        if ( npts == 3 )
          {
          cells->InsertNextCell(vlTRIANGLE,polys->GetLocation(npts));
          polys->InsertNextCell(npts,pts);
          }
        else // triangulate poly
          {
          poly.Initialize(npts,pts,inPoints);
          poly.Triangulate(outVerts);
          for (i=0; i<outVerts.GetNumberOfIds()/3; i++)
            {
            cells->InsertNextCell(vlTRIANGLE,polys->GetLocation(npts));
            polys->InsertNextCell(3);
            for (j=0; j<3; j++)
              polys->InsertCellPoint(outVerts.GetId(3*i+j));
            }
          }
        }
      }

    if ( inStrips )
      {
      for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
        {
        p1 = pts[0];
        p2 = pts[1];
        p3 = pts[2];
        for (i=0; i<(npts-2); i++)
          {
          cells->InsertNextCell(vlTRIANGLE,polys->GetLocation(npts));
          polys->InsertNextCell(3);
          if ( (i % 2) ) // flip ordering to preserve consistency
            {
            polys->InsertCellPoint(p2);
            polys->InsertCellPoint(p1);
            polys->InsertCellPoint(p3);
            }
          else
            {
            polys->InsertCellPoint(p1);
            polys->InsertCellPoint(p2);
            polys->InsertCellPoint(p3);
            }
          p1 = p2;
          p2 = p3;
          p3 = pts[3+i];
          }
        }
      }
    }
}

void vlPolyData::BuildLinks()
{
  if ( ! this->Cells ) this->BuildCells();
  this->Links = new vlLinkList(this->GetNumberOfPoints());
  this->Links->Register(this);

  this->Links->BuildLinks(this);
}

void vlPolyData::GetPointCells(int ptId, vlIdList *cellIds)
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


void vlPolyData::InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE])
{

  switch (type)
    {
    case vlPOINT: case vlPOLY_POINTS:
     this->Verts->InsertNextCell(npts,pts);
     break;

    case vlLINE: case vlPOLY_LINE:
     this->Lines->InsertNextCell(npts,pts);
      break;

    case vlTRIANGLE: case vlQUAD: case vlPOLYGON:
     this->Polys->InsertNextCell(npts,pts);
      break;

    case vlTRIANGLE_STRIP:
     this->Strips->InsertNextCell(npts,pts);
      break;
    }
}

