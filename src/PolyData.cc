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
//
// DataSet methods
//
#include "PolyData.hh"
#include "PolyMap.hh"
#include "CellType.hh"
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
vlCellArray *vlPolyData::Dummy = 0;

vlPolyData::vlPolyData ()
{
  this->Points = 0;

  this->Verts = 0;
  this->Lines = 0;
  this->Polys = 0;
  this->Strips = 0;

  // static variable, initialized only once.
  if (!this->Dummy) this->Dummy = new vlCellArray;

  this->LoadVerts = 1;
  this->LoadLines = 1;
  this->LoadPolys = 1;
  this->LoadStrips = 1;

  this->TriangleMesh = 0;
  this->Writable = 0;

  this->Cells = 0;
  this->Links = 0;
}

vlPolyData::vlPolyData(const vlPolyData& pd)
{

  this->Points = pd.Points;
  if (this->Points) this->Points->Register((void *)this);

  this->Verts = pd.Verts;
  if (this->Verts) this->Verts->Register((void *)this);

  this->Lines = pd.Lines;
  if (this->Lines) this->Lines->Register((void *)this);

  this->Polys = pd.Polys;
  if (this->Polys) this->Polys->Register((void *)this);

  this->Strips = pd.Strips;
  if (this->Strips) this->Strips->Register((void *)this);
 
  this->LoadVerts = pd.LoadVerts;
  this->LoadLines = pd.LoadLines;
  this->LoadPolys = pd.LoadPolys;
  this->LoadStrips = pd.LoadStrips;

  this->TriangleMesh = pd.TriangleMesh;
  this->Writable = pd.Writable;

  // these guys are not shared
  this->Cells = 0;
  this->Links = 0;
}

vlPolyData::~vlPolyData()
{
  vlPolyData::Initialize();
}

vlDataSet* vlPolyData::MakeObject()
{
  return new vlPolyData(*this);
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
    cell->Points.SetPoint(i,this->Points->GetPoint(i));
    }

  return cell;

}

void vlPolyData::SetVerts (vlCellArray* v) 
{
  if ( v != this->Verts && v != this->Dummy )
    {
    if (this->Verts) this->Verts->UnRegister((void *)this);
    this->Verts = v;
    if (this->Verts) this->Verts->Register((void *)this);
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
    if (this->Lines) this->Lines->UnRegister((void *)this);
    this->Lines = l;
    if (this->Lines) this->Lines->Register((void *)this);
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
    if (this->Polys) this->Polys->UnRegister((void *)this);
    this->Polys = p;
    if (this->Polys) this->Polys->Register((void *)this);
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
    if (this->Strips) this->Strips->UnRegister((void *)this);
    this->Strips = s;
    if (this->Strips) this->Strips->Register((void *)this);
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
  vlDataSet::Initialize();

  if ( this->Points ) 
  {
    this->Points->UnRegister((void *)this);
    this->Points = 0;
  }

  if ( this->Verts ) 
  {
    this->Verts->UnRegister((void *)this);
    this->Verts = 0;
  }

  if ( this->Lines ) 
  {
    this->Lines->UnRegister((void *)this);
    this->Lines = 0;
  }

  if ( this->Polys ) 
  {
    this->Polys->UnRegister((void *)this);
    this->Polys = 0;
  }

  if ( this->Strips ) 
  {
    this->Strips->UnRegister((void *)this);
    this->Strips = 0;
  }

  if ( this->Cells )
  {
    delete this->Cells;
    this->Cells = 0;
  }

  if ( this->Links )
  {
    delete this->Links;
    this->Links = 0;
  }

};

int vlPolyData::NumberOfCells() 
{
  return NumberOfVerts() + NumberOfLines() + NumberOfPolys() + NumberOfStrips();
}

int vlPolyData::NumberOfPoints() 
{
  return (this->Points ? this->Points->NumberOfPoints() : 0);
}

int vlPolyData::NumberOfVerts() 
{
  return (this->Verts ? this->Verts->GetNumberOfCells() : 0);
}

int vlPolyData::NumberOfLines() 
{
  return (this->Lines ? this->Lines->GetNumberOfCells() : 0);
}

int vlPolyData::NumberOfPolys() 
{
  return (this->Polys ? this->Polys->GetNumberOfCells() : 0);
}

int vlPolyData::NumberOfStrips() 
{
  return (this->Strips ? this->Strips->GetNumberOfCells() : 0);
}

vlMapper *vlPolyData::MakeMapper()
{
  vlPolyMapper *mapper;

  if ( ! this->Mapper )
    {
    this->Mapper = mapper = new vlPolyMapper;
    this->Mapper->Register((void *)this);
    mapper->SetInput(this);
    }
  return this->Mapper;
}
void vlPolyData::ComputeBounds()
{
  int i;
  float *bounds;

  if ( this->Points )
    {
    bounds = this->Points->GetBounds();
    for (i=0; i<6; i++) this->Bounds[i] = bounds[i];
    this->ComputeTime.Modified();
    }
}
void vlPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPolyData::GetClassName()))
    {
    vlDataSet::PrintSelf(os,indent);
    
    os << indent << "Number Of Points: " << this->NumberOfPoints() << "\n";
    os << indent << "Number Of Vertices: " << this->NumberOfVerts() << "\n";
    os << indent << "Number Of Lines: " << this->NumberOfLines() << "\n";
    os << indent << "Number Of Polygons: " << this->NumberOfPolys() << "\n";
    os << indent << "Number Of Triangle Strips: " << this->NumberOfStrips() << "\n";
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
  vlCellArray *verts=0;
  vlCellArray *lines=0;
  vlCellArray *polys=0;
  vlCellArray *strips=0;
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
    if (this->LoadVerts) numCells += this->NumberOfVerts();
    if (this->LoadLines) numCells += this->NumberOfLines();
    if (this->LoadPolys) numCells += this->NumberOfPolys();
    if (this->LoadStrips) numCells += this->NumberOfStrips();
    }
  else
    {
    // an initial estimate
    numCells += this->NumberOfPolys();
    numCells += this->NumberOfStrips();
    }

  if ( ! inPoints || numCells < 1 ) 
    {
    vlErrorMacro (<< "No data to build");
    return;
    }
  else
    {
    this->Cells = cells = new vlCellList(numCells,3*numCells);
    }
//
// If we are just reading the data structure, can use the input data
// without any copying or allocation.  Otherwise have to allocate
// working storage that is a copy of the input data. Triangle meshes always
// require new atorage.
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
    polys->Initialize(polys->EstimateSize(numCells,3),3*numCells);
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
          for (i=0; i<outVerts.NumberOfIds()/3; i++)
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
}
