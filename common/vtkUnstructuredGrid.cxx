/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"
#include "vtkPolyVertex.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkQuad.h"
#include "vtkPixel.h"
#include "vtkPolygon.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkVoxel.h"

vtkUnstructuredGrid::vtkUnstructuredGrid ()
{
  this->Vertex = vtkVertex::New();
  this->PolyVertex = vtkPolyVertex::New();
  this->Line = vtkLine::New();
  this->PolyLine = vtkPolyLine::New();
  this->Triangle = vtkTriangle::New();
  this->TriangleStrip = vtkTriangleStrip::New();
  this->Pixel = vtkPixel::New();
  this->Quad = vtkQuad::New();
  this->Polygon = vtkPolygon::New();
  this->Tetra = vtkTetra::New();
  this->Voxel = vtkVoxel::New();
  this->Hexahedron = vtkHexahedron::New();

  this->Connectivity = NULL;
  this->Cells = NULL;
  this->Links = NULL;
}

// Allocate memory space for data insertion. Execute this method before
// inserting any cells into object.
void vtkUnstructuredGrid::Allocate (int numCells, int extSize)
{
  if ( numCells < 1 )
    {
    numCells = 1000;
    }
  if ( extSize < 1 )
    {
    extSize = 1000;
    }

  this->Connectivity = vtkCellArray::New();
  this->Connectivity->Allocate(numCells,4*extSize);
  this->Connectivity->Register(this);
  this->Connectivity->Delete();

  this->Cells = new vtkCellTypes(numCells,extSize);
  this->Cells->Register(this);
  this->Cells->Delete();
}

// Shallow construction of object.
vtkUnstructuredGrid::vtkUnstructuredGrid(const vtkUnstructuredGrid& ug) :
vtkPointSet(ug)
{
  this->Connectivity = ug.Connectivity;
  if (this->Connectivity)
    {
    this->Connectivity->Register(this);
    }

  this->Cells = ug.Cells;
  if (this->Cells)
    {
    this->Cells->Register(this);
    }

  this->Links = ug.Links;
  if (this->Links)
    {
    this->Links->Register(this);
    }
}

vtkUnstructuredGrid::~vtkUnstructuredGrid()
{
  vtkUnstructuredGrid::Initialize();
  
  this->Vertex->Delete();
  this->PolyVertex->Delete();
  this->Line->Delete();
  this->PolyLine->Delete();
  this->Triangle->Delete();
  this->TriangleStrip->Delete();
  this->Pixel->Delete();
  this->Quad->Delete();
  this->Polygon->Delete();
  this->Tetra->Delete();
  this->Voxel->Delete();
  this->Hexahedron->Delete();
}

// Copy the geometric and topological structure of an input unstructured grid.
void vtkUnstructuredGrid::CopyStructure(vtkDataSet *ds)
{
  vtkUnstructuredGrid *ug=(vtkUnstructuredGrid *)ds;
  vtkPointSet::CopyStructure(ds);

  this->Connectivity = ug->Connectivity;
  if (this->Connectivity)
    {
    this->Connectivity->Register(this);
    }

  this->Cells = ug->Cells;
  if (this->Cells)
    {
    this->Cells->Register(this);
    }

  this->Links = ug->Links;
  if (this->Links)
    {
    this->Links->Register(this);
    }
}

void vtkUnstructuredGrid::Initialize()
{
  vtkPointSet::Initialize();

  if ( this->Connectivity )
    {
    this->Connectivity->UnRegister(this);
    this->Connectivity = NULL;
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
}

int vtkUnstructuredGrid::GetCellType(int cellId)
{
  return this->Cells->GetCellType(cellId);
}

vtkCell *vtkUnstructuredGrid::GetCell(int cellId)
{
  int i, loc, numPts, *pts;
  vtkCell *cell = NULL;

  switch (this->Cells->GetCellType(cellId))
    {
    case VTK_VERTEX:
      cell = this->Vertex;
      break;

    case VTK_POLY_VERTEX:
      cell = this->PolyVertex;
      break;

    case VTK_LINE: 
      cell = this->Line;
      break;

    case VTK_POLY_LINE:
      cell = this->PolyLine;
      break;

    case VTK_TRIANGLE:
      cell = this->Triangle;
      break;

    case VTK_TRIANGLE_STRIP:
      cell = this->TriangleStrip;
      break;

    case VTK_PIXEL:
      cell = this->Pixel;
      break;

    case VTK_QUAD:
      cell = this->Quad;
      break;

    case VTK_POLYGON:
      cell = this->Polygon;
      break;

    case VTK_TETRA:
      cell = this->Tetra;
      break;

    case VTK_VOXEL:
      cell = this->Voxel;
      break;

    case VTK_HEXAHEDRON:
      cell = this->Hexahedron;
      break;
    }

  loc = this->Cells->GetCellLocation(cellId);
  this->Connectivity->GetCell(loc,numPts,pts); 

  for (i=0; i<numPts; i++)
    {
    cell->PointIds->InsertId(i,pts[i]);
    cell->Points->InsertPoint(i,this->Points->GetPoint(pts[i]));
    }

  return cell;
}

int vtkUnstructuredGrid::GetMaxCellSize()
{
  if (this->Connectivity)
    {
    return this->Connectivity->GetMaxCellSize();
    }
  else
    {
    return 0;
    }
}

int vtkUnstructuredGrid::GetNumberOfCells() 
{
  return (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0);
}

void vtkUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet::PrintSelf(os,indent);
}

// Insert/create cell in object by type and list of point ids defining
// cell topology.
int vtkUnstructuredGrid::InsertNextCell(int type, vtkIdList& ptIds)
{
  int npts=ptIds.GetNumberOfIds();

  // insert connectivity
  this->Connectivity->InsertNextCell(ptIds);

  // insert type and storage information   
  return 
    this->Cells->InsertNextCell(type,this->Connectivity->GetInsertLocation(npts));
}

// Insert/create cell in object by type and list of point ids defining
// cell topology.
int vtkUnstructuredGrid::InsertNextCell(int type, int npts, int *pts)
{
  this->Connectivity->InsertNextCell(npts,pts);

  return
    this->Cells->InsertNextCell(type,this->Connectivity->GetInsertLocation(npts));
}

void vtkUnstructuredGrid::SetCells(int *types, vtkCellArray *cells)
{
  int i, npts, *pts;

  // set cell array
  if ( this->Connectivity )
    {
    this->Connectivity->UnRegister(this);
    }
  this->Connectivity = cells;
  if ( this->Connectivity )
    {
    this->Connectivity->Register(this);
    }

  // see whether there are cell types available
  if ( this->Cells )
    {
    this->Cells->UnRegister(this);
    }
  this->Cells = new vtkCellTypes(cells->GetNumberOfCells(),1000);
  this->Cells->Register(this);
  this->Cells->Delete();

  // build types
  for (i=0, cells->InitTraversal(); cells->GetNextCell(npts,pts); i++)
    {
    this->Cells->InsertNextCell(types[i],cells->GetTraversalLocation(npts));
    }
}

void vtkUnstructuredGrid::BuildLinks()
{
  this->Links = new vtkCellLinks(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->BuildLinks(this);
  this->Links->Delete();
}

void vtkUnstructuredGrid::GetCellPoints(int cellId, vtkIdList *ptIds)
{
  int i, loc, numPts, *pts;

  loc = this->Cells->GetCellLocation(cellId);
  this->Connectivity->GetCell(loc,numPts,pts); 

  ptIds->SetNumberOfIds(numPts);
  for (i=0; i<numPts; i++)
    {
    ptIds->SetId(i,pts[i]);
    }
}

// Return a pointer to a list of point ids defining cell. (More efficient than alternative
// method.)
void vtkUnstructuredGrid::GetCellPoints(int cellId, int& npts, int* &pts)
{
  int loc;

  loc = this->Cells->GetCellLocation(cellId);
  this->Connectivity->GetCell(loc,npts,pts);
}

void vtkUnstructuredGrid::GetPointCells(int ptId, vtkIdList *cellIds)
{
  int *cells;
  int numCells;
  int i;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }
  cellIds->Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  cellIds->SetNumberOfIds(numCells);
  for (i=0; i < numCells; i++)
    {
    cellIds->SetId(i,cells[i]);
    }
}

void vtkUnstructuredGrid::Reset()
{
  if ( this->Connectivity )
    {
    this->Connectivity->Reset();
    }
  if ( this->Cells )
    {
    this->Cells->Reset();
    }
  if ( this->Links )
    {
    this->Links->Reset();
    }
}

void vtkUnstructuredGrid::Squeeze()
{
  if ( this->Connectivity )
    {
    this->Connectivity->Squeeze();
    }
  if ( this->Cells )
    {
    this->Cells->Squeeze();
    }
  if ( this->Links )
    {
    this->Links->Squeeze();
    }

  vtkPointSet::Squeeze();
}

// Remove a reference to a cell in a particular point's link list. You may 
// also consider using RemoveCellReference() to remove the references from 
// all the cell's points to the cell. This operator does not reallocate 
// memory; use the operator ResizeCellList() to do this if necessary.
void vtkUnstructuredGrid::RemoveReferenceToCell(int ptId, int cellId)
{
  this->Links->RemoveCellReference(cellId, ptId);  
}

// Add a reference to a cell in a particular point's link list. (You may also
// consider using AddCellReference() to add the references from all the 
// cell's points to the cell.) This operator does not realloc memory; use the
// operator ResizeCellList() to do this if necessary.
void vtkUnstructuredGrid::AddReferenceToCell(int ptId, int cellId)
{
  this->Links->AddCellReference(cellId, ptId);  
}

// Resize the list of cells using a particular point. (This operator assumes
// that BuildLinks() has been called.)
void vtkUnstructuredGrid::ResizeCellList(int ptId, int size)
{
  this->Links->ResizeCellList(ptId,size);
}

// Replace the points defining cell "cellId" with a new set of points. This
// operator is (typically) used when links from points to cells have not been
// built (i.e., BuildLinks() has not been executed). Use the operator 
// ReplaceLinkedCell() to replace a cell when cell structure has been built.
void vtkUnstructuredGrid::ReplaceCell(int cellId, int npts, int *pts)
{
  int loc, type;

  loc = this->Cells->GetCellLocation(cellId);
  type = this->Cells->GetCellType(cellId);

  this->Connectivity->ReplaceCell(loc,npts,pts);
}

// Add a new cell to the cell data structure (after cell links have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.)
int vtkUnstructuredGrid::InsertNextLinkedCell(int type, int npts, int *pts)
{
  int i, id;

  id = this->InsertNextCell(type,npts,pts);

  for (i=0; i<npts; i++)
    {
    this->Links->ResizeCellList(pts[i],1);
    this->Links->AddCellReference(id,pts[i]);  
    }

  return id;
}

