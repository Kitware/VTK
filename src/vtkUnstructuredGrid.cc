/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.cc
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
#include "vtkUnstructuredGrid.hh"
#include "vtkVertex.hh"
#include "vtkPolyVertex.hh"
#include "vtkLine.hh"
#include "vtkPolyLine.hh"
#include "vtkTriangle.hh"
#include "vtkTriangleStrip.hh"
#include "vtkQuad.hh"
#include "vtkPixel.hh"
#include "vtkPolygon.hh"
#include "vtkTetra.hh"
#include "vtkHexahedron.hh"
#include "vtkVoxel.hh"

vtkUnstructuredGrid::vtkUnstructuredGrid ()
{
  this->Connectivity = NULL;
  this->Cells = NULL;
  this->Links = NULL;
}

// Description:
// Allocate memory space for data insertion. Execute this method before
// inserting and cells into object.
void vtkUnstructuredGrid::Allocate (int numCells, int extSize)
{
  if ( numCells < 1 ) numCells = 1000;
  if ( extSize < 1 ) extSize = 1000;

  this->Connectivity = new vtkCellArray(numCells,4*extSize);
  this->Connectivity->Register(this);
  this->Connectivity->Delete();

  this->Cells = new vtkCellList(numCells,extSize);
  this->Cells->Register(this);
  this->Cells->Delete();
}

// Description:
// Shallow construction of object.
vtkUnstructuredGrid::vtkUnstructuredGrid(const vtkUnstructuredGrid& pd) :
vtkPointSet(pd)
{
  this->Connectivity = pd.Connectivity;
  if (this->Connectivity) this->Connectivity->Register(this);

  this->Cells = pd.Cells;
  if (this->Cells) this->Cells->Register(this);

  this->Links = pd.Links;
  if (this->Links) this->Links->Register(this);
}

vtkUnstructuredGrid::~vtkUnstructuredGrid()
{
  vtkUnstructuredGrid::Initialize();
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
};

int vtkUnstructuredGrid::GetCellType(int cellId)
{
  return this->Cells->GetCellType(cellId);
}

vtkCell *vtkUnstructuredGrid::GetCell(int cellId)
{
  static vtkVertex vertex;
  static vtkPolyVertex pvertex;
  static vtkLine line;
  static vtkPolyLine pline;
  static vtkTriangle triangle;
  static vtkTriangleStrip strip;
  static vtkPolygon poly;
  static vtkPixel pixel;
  static vtkQuad quad;
  static vtkTetra tetra;
  static vtkVoxel voxel;
  static vtkHexahedron hexa;
  int i, loc, numPts, *pts;
  vtkCell *cell;

  switch (this->Cells->GetCellType(cellId))
    {
    case vtkVERTEX:
     cell = &vertex;
     break;

    case vtkPOLY_VERTEX:
     cell = &pvertex;
     break;

    case vtkLINE: 
      cell = &line;
      break;

    case vtkPOLY_LINE:
      cell = &pline;
      break;

    case vtkTRIANGLE:
      cell = &triangle;
      break;

    case vtkTRIANGLE_STRIP:
      cell = &strip;
      break;

    case vtkPIXEL:
      cell = &pixel;
      break;

    case vtkQUAD:
      cell = &quad;
      break;

    case vtkPOLYGON:
      cell = &poly;
      break;

    case vtkTETRA:
      cell = &tetra;
      break;

    case vtkVOXEL:
      cell = &voxel;
      break;

    case vtkHEXAHEDRON:
      cell = &hexa;
      break;
    }

  loc = this->Cells->GetCellLocation(cellId);
  this->Connectivity->GetCell(loc,numPts,pts); 

  for (i=0; i<numPts; i++)
    {
    cell->PointIds.SetId(i,pts[i]);
    cell->Points.SetPoint(i,this->Points->GetPoint(pts[i]));
    }

  return cell;
}

int vtkUnstructuredGrid::GetNumberOfCells() 
{
  return (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0);
}

void vtkUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet::PrintSelf(os,indent);
}

// Description:
// Insert/create cell in object by type and list of point ids defining
// cell topology.
int vtkUnstructuredGrid::InsertNextCell(int type, vtkIdList& ptIds)
{
  int i;
  int npts=ptIds.GetNumberOfIds();

  // insert connectivity
  this->Connectivity->InsertNextCell(npts);
  for (i=0; i < npts; i++) this->Connectivity->InsertCellPoint(ptIds.GetId(i));

  // insert type and storage information   
  return
    this->Cells->InsertNextCell(type,this->Connectivity->GetLocation(npts));
}

// Description:
// Insert/create cell in object by type and list of point ids defining
// cell topology.
int vtkUnstructuredGrid::InsertNextCell(int type, int npts, 
				       int pts[MAX_CELL_SIZE])
{
  this->Connectivity->InsertNextCell(npts,pts);

  return
    this->Cells->InsertNextCell(type,this->Connectivity->GetLocation(npts));
}

void vtkUnstructuredGrid::SetCells(int *types, vtkCellArray *cells)
{
  int i, npts, *pts;

  // set cell array
  if ( this->Connectivity ) this->Connectivity->UnRegister(this);
  this->Connectivity = cells;
  if ( this->Connectivity ) this->Connectivity->Register(this);

  // see whether there are cell types available
  if ( this->Cells ) this->Cells->UnRegister(this);
  this->Cells = new vtkCellList(cells->GetNumberOfCells(),1000);
  this->Cells->Register(this);
  this->Cells->Delete();

  // build types
  for (i=0, cells->InitTraversal(); cells->GetNextCell(npts,pts); i++)
    {
    this->Cells->InsertNextCell(types[i],cells->GetLocation(npts));
    }
}

void vtkUnstructuredGrid::BuildLinks()
{
  this->Links = new vtkLinkList(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->BuildLinks(this);
  this->Links->Delete();
}

void vtkUnstructuredGrid::GetCellPoints(int cellId, vtkIdList& ptIds)
{
  int i, loc, numPts, *pts;

  ptIds.Reset();
  loc = this->Cells->GetCellLocation(cellId);
  this->Connectivity->GetCell(loc,numPts,pts); 

  for (i=0; i<numPts; i++) ptIds.SetId(i,pts[i]);
}

void vtkUnstructuredGrid::GetPointCells(int ptId, vtkIdList& cellIds)
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

void vtkUnstructuredGrid::Squeeze()
{
  if ( this->Connectivity ) this->Connectivity->Squeeze();
  if ( this->Cells ) this->Cells->Squeeze();
  if ( this->Links ) this->Links->Squeeze();

  vtkPointSet::Squeeze();
}
