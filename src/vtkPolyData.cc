/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPolyData.hh"
#include "vtkVertex.hh"
#include "vtkPolyVertex.hh"
#include "vtkLine.hh"
#include "vtkPolyLine.hh"
#include "vtkTriangle.hh"
#include "vtkTriangleStrip.hh"
#include "vtkQuad.hh"
#include "vtkPolygon.hh"

// Initialize static member.  This member is used to simplify traversal
// of verts, lines, polygons, and triangle strips lists.  It basically 
// "marks" empty lists so that the traveral method "GetNextCell" 
// works properly.
vtkCellArray *vtkPolyData::Dummy = NULL;

vtkPolyData::vtkPolyData ()
{
  this->Verts = NULL;
  this->Lines = NULL;
  this->Polys = NULL;
  this->Strips = NULL;

  // static variable, initialized only once.
  if (!this->Dummy) this->Dummy = new vtkCellArray;

  this->Cells = NULL;
  this->Links = NULL;
}

// Description:
// Perform shallow construction of vtkPolyData.
vtkPolyData::vtkPolyData(const vtkPolyData& pd) :
vtkPointSet(pd)
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

vtkPolyData::~vtkPolyData()
{
  vtkPolyData::Initialize();
}

// Description:
// Copy the geometric and topological structure of an input poly data object.
void vtkPolyData::CopyStructure(vtkDataSet *ds)
{
  vtkPolyData *pd=(vtkPolyData *)ds;
  vtkPointSet::CopyStructure(ds);

  this->Verts = pd->Verts;
  if (this->Verts) this->Verts->Register(this);

  this->Lines = pd->Lines;
  if (this->Lines) this->Lines->Register(this);

  this->Polys = pd->Polys;
  if (this->Polys) this->Polys->Register(this);

  this->Strips = pd->Strips;
  if (this->Strips) this->Strips->Register(this);
}

int vtkPolyData::GetCellType(int cellId)
{
  if ( !this->Cells ) this->BuildCells();
  return this->Cells->GetCellType(cellId);
}

vtkCell *vtkPolyData::GetCell(int cellId)
{
  static vtkVertex vertex;
  static vtkPolyVertex pvertex;
  static vtkLine line;
  static vtkPolyLine pline;
  static vtkTriangle triangle;
  static vtkTriangleStrip strip;
  static vtkPolygon poly;
  static vtkQuad quad;
  int i, loc, numPts, *pts;
  vtkCell *cell;
  unsigned char type;

  if ( !this->Cells ) this->BuildCells();

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case VTK_VERTEX:
     cell = &vertex;
     this->Verts->GetCell(loc,numPts,pts);
     break;

    case VTK_POLY_VERTEX:
     cell = &pvertex;
     this->Verts->GetCell(loc,numPts,pts);
     break;

    case VTK_LINE: 
      cell = &line;
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case VTK_POLY_LINE:
      cell = &pline;
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case VTK_TRIANGLE:
      cell = &triangle;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_QUAD:
      cell = &quad;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_POLYGON:
      cell = &poly;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_TRIANGLE_STRIP:
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

// Description:
// Set the cell array defining vertices.
void vtkPolyData::SetVerts (vtkCellArray* v) 
{
  if ( v != this->Verts && v != this->Dummy )
    {
    if (this->Verts) this->Verts->UnRegister(this);
    this->Verts = v;
    if (this->Verts) this->Verts->Register(this);
    this->Modified();
    }
}

// Description:
// Get the cell array defining vertices. If there are no vertices, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetVerts()
{
  if ( !this->Verts ) return this->Dummy;
  else return this->Verts;
}

// Description:
// Set the cell array defining lines.
void vtkPolyData::SetLines (vtkCellArray* l) 
{
  if ( l != this->Lines && l != this->Dummy )
    {
    if (this->Lines) this->Lines->UnRegister(this);
    this->Lines = l;
    if (this->Lines) this->Lines->Register(this);
    this->Modified();
    }
}

// Description:
// Get the cell array defining lines. If there are no lines, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetLines()
{
  if ( !this->Lines ) return this->Dummy;
  else return this->Lines;
}

// Description:
// Set the cell array defining polygons.
void vtkPolyData::SetPolys (vtkCellArray* p) 
{
  if ( p != this->Polys && p != this->Dummy )
    {
    if (this->Polys) this->Polys->UnRegister(this);
    this->Polys = p;
    if (this->Polys) this->Polys->Register(this);
    this->Modified();
    }
}

// Description:
// Get the cell array defining polygons. If there are no polygons, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetPolys()
{
  if ( !this->Polys ) return this->Dummy;
  else return this->Polys;
}

// Description:
// Set the cell array defining triangle strips.
void vtkPolyData::SetStrips (vtkCellArray* s) 
{
  if ( s != this->Strips && s != this->Dummy )
    {
    if (this->Strips) this->Strips->UnRegister(this);
    this->Strips = s;
    if (this->Strips) this->Strips->Register(this);
    this->Modified();
    }
}

// Description:
// Get the cell array defining triangle strips. If there are no
// triangle strips, an empty array will be returned (convenience to 
// simplify traversal).
vtkCellArray* vtkPolyData::GetStrips()
{
  if ( !this->Strips ) return this->Dummy;
  else return this->Strips;
}

// Description:
// Restore object to initial state. Release memory back to system.
void vtkPolyData::Initialize()
{
  vtkPointSet::Initialize();

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

int vtkPolyData::GetNumberOfCells() 
{
  return GetNumberOfVerts() + GetNumberOfLines() + 
         GetNumberOfPolys() + GetNumberOfStrips();
}

int vtkPolyData::GetNumberOfVerts() 
{
  return (this->Verts ? this->Verts->GetNumberOfCells() : 0);
}

int vtkPolyData::GetNumberOfLines() 
{
  return (this->Lines ? this->Lines->GetNumberOfCells() : 0);
}

int vtkPolyData::GetNumberOfPolys() 
{
  return (this->Polys ? this->Polys->GetNumberOfCells() : 0);
}

int vtkPolyData::GetNumberOfStrips() 
{
  return (this->Strips ? this->Strips->GetNumberOfCells() : 0);
}

// Description:
// Create data structure that allows random access of cells.
void vtkPolyData::BuildCells()
{
  int numCells=0;
  vtkCellArray *inVerts=this->GetVerts();
  vtkCellArray *inLines=this->GetLines();
  vtkCellArray *inPolys=this->GetPolys();
  vtkCellArray *inStrips=this->GetStrips();
  int npts, *pts;
  vtkCellList *cells;
  vtkPoints *inPoints=this->GetPoints();

  vtkDebugMacro (<< "Building PolyData cells.");

  numCells = this->GetNumberOfCells();

  if ( inPoints == NULL || numCells < 1 ) 
    {
    vtkErrorMacro (<< "No data to build");
    return;
    }
  else
    {
    this->Cells = cells = new vtkCellList(numCells,3*numCells);
    this->Cells->Register(this);
    cells->Delete();
    }
//
// Traverse various lists to create cell array
//
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
    {
    if ( npts > 1 )
      cells->InsertNextCell(VTK_POLY_VERTEX,inVerts->GetLocation(npts));
    else
      cells->InsertNextCell(VTK_VERTEX,inVerts->GetLocation(npts));
    }

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    if ( npts > 2 )
      cells->InsertNextCell(VTK_POLY_LINE,inLines->GetLocation(npts));
    else
      cells->InsertNextCell(VTK_LINE,inLines->GetLocation(npts));
    }

  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    if ( npts == 3 )
      cells->InsertNextCell(VTK_TRIANGLE,inPolys->GetLocation(npts));
    else if ( npts == 4 )
      cells->InsertNextCell(VTK_QUAD,inPolys->GetLocation(npts));
    else
      cells->InsertNextCell(VTK_POLYGON,inPolys->GetLocation(npts));
    }

  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    cells->InsertNextCell(VTK_TRIANGLE_STRIP,inStrips->GetLocation(npts));
    }
}

// Description:
// Create upward links from points to cells that use each point. Enables
// topologically complex queries.
void vtkPolyData::BuildLinks()
{
  if ( this->Cells == NULL ) this->BuildCells();
  this->Links = new vtkLinkList(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->Delete();

  this->Links->BuildLinks(this);
}

// Description:
// Copy a cells point ids into list provided. (Less efficient).
void vtkPolyData::GetCellPoints(int cellId, vtkIdList& ptIds)
{
  int i, npts, *pts;

  ptIds.Reset();
  if ( this->Cells == NULL ) this->BuildCells();

  this->vtkPolyData::GetCellPoints(cellId, npts, pts);
  for (i=0; i<npts; i++) ptIds.SetId(i,pts[i]);
}

// Description:
// Return a pointer to a list of point ids defining cell. (More efficient).
// Assumes that cells have been built (with BuildCells()).
void vtkPolyData::GetCellPoints(int cellId, int& npts, int* &pts)
{
  int loc;
  unsigned char type;

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
     this->Verts->GetCell(loc,npts,pts);
     break;

    case VTK_LINE: case VTK_POLY_LINE:
      this->Lines->GetCell(loc,npts,pts);
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      this->Polys->GetCell(loc,npts,pts);
      break;

    case VTK_TRIANGLE_STRIP:
      this->Strips->GetCell(loc,npts,pts);
      break;
    }
}

void vtkPolyData::GetPointCells(int ptId, vtkIdList& cellIds)
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

// Description:
// Method allocates initial storage for vertex, line, polygon, and 
// triangle strips arrays. Use this method before the method 
// PolyData::InsertNextCell(). (Or, provide vertex, line, polygon, and
// triangle strip cell arrays.)
void vtkPolyData::Allocate(int numCells, int extSize)
{
  vtkCellArray *cells;

  cells = new vtkCellArray(numCells,extSize);
  this->SetVerts(cells);
  cells->Delete();

  cells = new vtkCellArray(numCells,extSize);
  this->SetLines(cells);
  cells->Delete();

  cells = new vtkCellArray(numCells,extSize);
  this->SetPolys(cells);
  cells->Delete();

  cells = new vtkCellArray(numCells,extSize);
  this->SetStrips(cells);
  cells->Delete();
}

// Description:
// Insert a cell of type vtkVERTEX, vtkPOLY_VERTEX, vtkLINE, vtkPOLY_LINE,
// vtkTRIANGLE, vtkQUAD, vtkPOLYGON, or vtkTRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert vtkPIXEL, but converts it to vtkQUAD.
int vtkPolyData::InsertNextCell(int type, int npts, int pts[VTK_MAX_CELL_SIZE])
{
  int id = 0;

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
      id = this->Verts->InsertNextCell(npts,pts);
      break;

    case VTK_LINE: case VTK_POLY_LINE:
      id = this->Lines->InsertNextCell(npts,pts);
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      id = this->Polys->InsertNextCell(npts,pts);
      break;

    case VTK_PIXEL: //need to rearrange vertices
      {
      static int pixPts[4];
      pixPts[0] = pts[0];
      pixPts[1] = pts[1];
      pixPts[2] = pts[3];
      pixPts[3] = pts[2];
      id = this->Polys->InsertNextCell(npts,pixPts);
      break;
      }

    case VTK_TRIANGLE_STRIP:
      id = this->Strips->InsertNextCell(npts,pts);
      break;

    default:
      vtkErrorMacro(<<"Bad cell type! Can't insert!");
    }
  return id;
}

// Description:
// Recover extra allocated memory when creating data whose initial size
// is unknown. Examples include using the InsertNextCell() method, or
// when using the CellArray::EstimateSize() method to create vertices,
// lines, polygons, or triangle strips.

void vtkPolyData::Squeeze()
{
  if ( this->Verts != NULL ) this->Verts->Squeeze();
  if ( this->Lines != NULL ) this->Lines->Squeeze();
  if ( this->Polys != NULL ) this->Polys->Squeeze();
  if ( this->Strips != NULL ) this->Strips->Squeeze();

  vtkPointSet::Squeeze();
}

// Description:
// Reverse the order of point ids defining the cell.
void vtkPolyData::ReverseCell(int cellId)
{
  int loc, type;

  if ( this->Cells == NULL ) this->BuildCells();
  loc = this->Cells->GetCellLocation(cellId);
  type = this->Cells->GetCellType(cellId);

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
     this->Verts->ReverseCell(loc);
     break;

    case VTK_LINE: case VTK_POLY_LINE:
      this->Lines->ReverseCell(loc);
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      this->Polys->ReverseCell(loc);
      break;

    case VTK_TRIANGLE_STRIP:
      this->Strips->ReverseCell(loc);
      break;
    }
}

// Description:
// Replace the points defining cell "cellId" with a new set of points.
void vtkPolyData::ReplaceCell(int cellId, int npts, int *pts)
{
  int loc, type;

  if ( this->Cells == NULL ) this->BuildCells();
  loc = this->Cells->GetCellLocation(cellId);
  type = this->Cells->GetCellType(cellId);

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
     this->Verts->ReplaceCell(loc,npts,pts);
     break;

    case VTK_LINE: case VTK_POLY_LINE:
      this->Lines->ReplaceCell(loc,npts,pts);
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      this->Polys->ReplaceCell(loc,npts,pts);
      break;

    case VTK_TRIANGLE_STRIP:
      this->Strips->ReplaceCell(loc,npts,pts);
      break;
    }
}

void vtkPolyData::ReplaceLinkedCell(int cellId, int npts, int *pts)
{
  int loc = this->Cells->GetCellLocation(cellId);
  int type = this->Cells->GetCellType(cellId);

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
     this->Verts->ReplaceCell(loc,npts,pts);
     break;

    case VTK_LINE: case VTK_POLY_LINE:
      this->Lines->ReplaceCell(loc,npts,pts);
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      this->Polys->ReplaceCell(loc,npts,pts);
      break;

    case VTK_TRIANGLE_STRIP:
      this->Strips->ReplaceCell(loc,npts,pts);
      break;
    }

  for (int i=0; i < npts; i++)
    {
    this->Links->InsertNextCellReference(pts[i],cellId);
    }
}

// Description:
// Get the neighbors at an edge. More efficient than the general 
// GetCellNeighbors(). Assumes links have been built (with BuildLinks()), 
// and looks specifically for edge neighbors.
void vtkPolyData::GetCellEdgeNeighbors(int cellId, int p1, int p2,
                                      vtkIdList& cellIds)
{
  int *cells;
  int numCells;
  int i,j;
  int npts, *pts;

  cellIds.Reset();

  numCells = this->Links->GetNcells(p1);
  cells = this->Links->GetCells(p1);

  for (i=0; i < numCells; i++)
    {
    if ( cells[i] != cellId )
      {
      this->GetCellPoints(cells[i],npts,pts);
      for (j=0; j < npts; j++) if ( pts[j] == p2 ) break;
      if ( j < npts ) cellIds.InsertNextId(cells[i]);
      }
    }
}

void vtkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);

  os << indent << "Number Of Vertices: " << this->GetNumberOfVerts() << "\n";
  os << indent << "Number Of Lines: " << this->GetNumberOfLines() << "\n";
  os << indent << "Number Of Polygons: " << this->GetNumberOfPolys() << "\n";
  os << indent << "Number Of Triangle Strips: " << this->GetNumberOfStrips() << "\n";
}

