/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPolyData.h"
#include "vtkVertex.h"
#include "vtkPolyVertex.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkQuad.h"
#include "vtkPolygon.h"
#include "vtkEmptyCell.h"
#include "vtkObjectFactory.h"
#include "vtkCriticalSection.h"


//----------------------------------------------------------------------------
vtkPolyData* vtkPolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyData");
  if(ret)
    {
    return (vtkPolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyData;
}




//----------------------------------------------------------------------------
// Initialize static member.  This member is used to simplify traversal
// of verts, lines, polygons, and triangle strips lists.  It basically 
// "marks" empty lists so that the traveral method "GetNextCell" 
// works properly.
vtkCellArray *vtkPolyData::Dummy = NULL;

static vtkSimpleCriticalSection DummyCritSect;

vtkPolyData::vtkPolyData ()
{
  // Create these guys only when needed. This saves a huge amount
  // of memory and time spent in memory allocation.
  this->Vertex = NULL;
  this->PolyVertex = NULL;
  this->Line = NULL;
  this->PolyLine = NULL;
  this->Triangle = NULL;
  this->Quad = NULL;
  this->Polygon = NULL;
  this->TriangleStrip = NULL;
  this->EmptyCell = NULL;
  
  this->Verts = NULL;
  this->Lines = NULL;
  this->Polys = NULL;
  this->Strips = NULL;

  // static variable, initialized only once.
  DummyCritSect.Lock();
  if (this->Dummy == NULL) 
    {
    this->Dummy = vtkCellArray::New();
    this->Dummy->Register(this);
    this->Dummy->Delete();
    }
  else
    {
    this->Dummy->Register(this);
    }
  DummyCritSect.Unlock();

  this->Cells = NULL;
  this->Links = NULL;
}

//----------------------------------------------------------------------------
vtkPolyData::~vtkPolyData()
{
  vtkPolyData::Initialize();
  // Reference to static dummy persists. 
  // Keep destructed dummy from being used again.
  DummyCritSect.Lock();
  if (this->Dummy->GetReferenceCount() == 1)
    {
    this->Dummy->UnRegister(this);
    this->Dummy = NULL;
    }
  else
    {
    this->Dummy->UnRegister(this);
    }
  DummyCritSect.Unlock();

  if (this->Vertex)
    {
    this->Vertex->Delete();
    }

  if (this->PolyVertex)
    {
    this->PolyVertex->Delete();
    }

  if (this->Line)
    {
    this->Line->Delete();
    }

  if (this->PolyLine)
    {
    this->PolyLine->Delete();
    }

  if (this->Triangle)
    {
    this->Triangle->Delete();
    }
  
  if (this->Quad)
    {
    this->Quad->Delete();
    }

  if (this->Polygon)
    {
    this->Polygon->Delete();
    }

  if (this->TriangleStrip)
    {
    this->TriangleStrip->Delete();
    }

  if (this->EmptyCell)
    {
    this->EmptyCell->Delete();
    }
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input poly data object.
void vtkPolyData::CopyStructure(vtkDataSet *ds)
{
  vtkPolyData *pd=(vtkPolyData *)ds;
  vtkPointSet::CopyStructure(ds);

  this->Verts = pd->Verts;
  if (this->Verts)
    {
    this->Verts->Register(this);
    }

  this->Lines = pd->Lines;
  if (this->Lines)
    {
    this->Lines->Register(this);
    }

  this->Polys = pd->Polys;
  if (this->Polys)
    {
    this->Polys->Register(this);
    }

  this->Strips = pd->Strips;
  if (this->Strips)
    {
    this->Strips->Register(this);
    }
}

//----------------------------------------------------------------------------
int vtkPolyData::GetCellType(vtkIdType cellId)
{
  if ( !this->Cells )
    {
    this->BuildCells();
    }
  return this->Cells->GetCellType(cellId);
}

//----------------------------------------------------------------------------
vtkCell *vtkPolyData::GetCell(vtkIdType cellId)
{
  int i, loc;
  vtkIdType *pts, numPts;
  vtkCell *cell = NULL;
  unsigned char type;

  if ( !this->Cells )
    {
    this->BuildCells();
    }

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case VTK_VERTEX:
      if (!this->Vertex)
        {
        this->Vertex = vtkVertex::New();
        }
      cell = this->Vertex;
      this->Verts->GetCell(loc,numPts,pts);
      break;

    case VTK_POLY_VERTEX:
      if (! this->PolyVertex)
        {
        this->PolyVertex = vtkPolyVertex::New();
        }
      cell = this->PolyVertex;
      this->Verts->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_LINE: 
      if (! this->Line )
        {
        this->Line = vtkLine::New();
        }
      cell = this->Line;
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case VTK_POLY_LINE:
      if (!this->PolyLine)
        {
        this->PolyLine = vtkPolyLine::New();
        }
      cell = this->PolyLine;
      this->Lines->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE:
      if (!this->Triangle)
        {
        this->Triangle = vtkTriangle::New();
        }
      cell = this->Triangle;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_QUAD:
      if (!this->Quad)
        {
        this->Quad = vtkQuad::New();
        }
      cell = this->Quad;
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_POLYGON:
      if (!this->Polygon)
        {
        this->Polygon = vtkPolygon::New();
        }
      cell = this->Polygon;
      this->Polys->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE_STRIP:
      if (!this->TriangleStrip)
        {
        this->TriangleStrip = vtkTriangleStrip::New();
        }
      cell = this->TriangleStrip;
      this->Strips->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    default:
      if (!this->EmptyCell)
        {
        this->EmptyCell = vtkEmptyCell::New();
        }
      cell = this->EmptyCell;
      numPts = 0;
      return cell;
    }

  for (i=0; i < numPts; i++)
    {
    cell->PointIds->SetId(i,pts[i]);
    cell->Points->SetPoint(i,this->Points->GetPoint(pts[i]));
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkPolyData::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  int             i, loc;
  vtkIdType       *pts, numPts;
  unsigned char   type;
  float           x[3];

  if ( !this->Cells )
    {
    this->BuildCells();
    }

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case VTK_VERTEX:
      cell->SetCellTypeToVertex();
      this->Verts->GetCell(loc,numPts,pts);
      break;

    case VTK_POLY_VERTEX:
      cell->SetCellTypeToPolyVertex();
      this->Verts->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_LINE: 
      cell->SetCellTypeToLine();
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case VTK_POLY_LINE:
      cell->SetCellTypeToPolyLine();
      this->Lines->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE:
      cell->SetCellTypeToTriangle();
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_QUAD:
      cell->SetCellTypeToQuad();
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_POLYGON:
      cell->SetCellTypeToPolygon();
      this->Polys->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE_STRIP:
      cell->SetCellTypeToTriangleStrip();
      this->Strips->GetCell(loc,numPts,pts);
      cell->PointIds->SetNumberOfIds(numPts); //reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    default:
      cell->SetCellTypeToEmptyCell();
      numPts = 0;
    }

  for (i=0; i < numPts; i++)
    {
    cell->PointIds->SetId(i,pts[i]);
    this->Points->GetPoint(pts[i], x);
    cell->Points->SetPoint(i, x);
    }
}

void vtkPolyData::CopyCells(vtkPolyData *pd, vtkIdList *idList,
                            vtkPointLocator *locator)
{
  vtkIdType cellId, ptId, newId, newCellId, locatorPtId;
  int numPts, numCellPts, i;
  vtkPoints *newPoints;
  vtkIdList *pointMap = vtkIdList::New(); //maps old pt ids into new
  vtkIdList *cellPts, *newCellPts = vtkIdList::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  float x[3];
  vtkPointData *outPD = this->GetPointData();
  vtkCellData *outCD = this->GetCellData();
  
  numPts = pd->GetNumberOfPoints();

  if (this->GetPoints() == NULL)
    {
    this->Points = vtkPoints::New();
    }
  
  newPoints = this->GetPoints();
  
  pointMap->SetNumberOfIds(numPts);
  for (i=0; i < numPts; i++)
    {
    pointMap->SetId(i,-1);
    }

  // Filter the cells
  for (cellId=0; cellId < idList->GetNumberOfIds(); cellId++)
    {
    pd->GetCell(idList->GetId(cellId), cell);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();
    
    for (i=0; i < numCellPts; i++)
      {
      ptId = cellPts->GetId(i);
      if ( (newId = pointMap->GetId(ptId)) < 0 )
        {
        pd->GetPoint(ptId, x);
        if (locator != NULL)
          {
          if ((locatorPtId = locator->IsInsertedPoint(x)) == -1)
            {
            newId = newPoints->InsertNextPoint(x);
            locator->InsertNextPoint(x);
            pointMap->SetId(ptId, newId);
            outPD->CopyData(pd->GetPointData(), ptId, newId);
            }
          else
            {
            newId = locatorPtId;
            }
          }
        else
          {
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId, newId);
          outPD->CopyData(pd->GetPointData(), ptId, newId);
          }
        }
      newCellPts->InsertId(i,newId);
      }
    newCellId = this->InsertNextCell(cell->GetCellType(), newCellPts);
    outCD->CopyData(pd->GetCellData(), idList->GetId(cellId), newCellId);
    newCellPts->Reset();
    } // for all cells
  newCellPts->Delete();
  pointMap->Delete();
  cell->Delete();
}

//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkPolyData::GetCellBounds(vtkIdType cellId, float bounds[6])
{
  int i, loc;
  vtkIdType *pts, numPts;
  unsigned char type;
  float x[3];

  if ( !this->Cells )
    {
    this->BuildCells();
    }

  type = this->Cells->GetCellType(cellId);
  loc = this->Cells->GetCellLocation(cellId);

  switch (type)
    {
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      this->Verts->GetCell(loc,numPts,pts);
      break;

    case VTK_LINE: 
    case VTK_POLY_LINE:
      this->Lines->GetCell(loc,numPts,pts);
      break;

    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_POLYGON:
      this->Polys->GetCell(loc,numPts,pts);
      break;

    case VTK_TRIANGLE_STRIP:
      this->Strips->GetCell(loc,numPts,pts);
      break;

    default:
      bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5]
        = 0.0;
      return;
    }

  bounds[0] = bounds[2] = bounds[4] =  VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;

  for (i=0; i < numPts; i++)
    {
    this->Points->GetPoint( pts[i], x );

    if (x[0] < bounds[0])
      {
      bounds[0] = x[0];
      }
    if (x[0] > bounds[1])
      {
      bounds[1] = x[0];
      }
    if (x[1] < bounds[2])
      {
      bounds[2] = x[1];
      }
    if (x[1] > bounds[3])
      {
      bounds[3] = x[1];
      }
    if (x[2] < bounds[4])
      {
      bounds[4] = x[2];
      }
    if (x[2] > bounds[5])
      {
      bounds[5] = x[2];
      }
    }
}


//----------------------------------------------------------------------------
void vtkPolyData::ComputeBounds()
{
  if (this->GetMTime() > this->ComputeTime)
    {
    // If there are no cells, but there are points, back to the
    // bounds of the points set.
    if (this->GetNumberOfCells() == 0 && this->GetNumberOfPoints()) 
      {
      vtkPointSet::ComputeBounds();
      return;
      } 

    int t, i;
    vtkIdType *pts, npts;
    float x[3];

    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;

    vtkCellArray *cella[4];

    cella[0] = this->GetVerts();
    cella[1] = this->GetLines();
    cella[2] = this->GetPolys();
    cella[3] = this->GetStrips();

    // Iterate over cells's points
    for (t = 0; t < 4; t++) 
      {
      for (cella[t]->InitTraversal(); cella[t]->GetNextCell(npts,pts); )
        {
        for (i = 0;  i < npts; i++)
          {
          this->Points->GetPoint( pts[i], x );

          if (x[0] < this->Bounds[0])
            {
            this->Bounds[0] = x[0];
            }
          if (x[0] > this->Bounds[1])
            {
            this->Bounds[1] = x[0];
            }
          if (x[1] < this->Bounds[2])
            {
            this->Bounds[2] = x[1];
            }
          if (x[1] > this->Bounds[3])
            {
            this->Bounds[3] = x[1];
            }
          if (x[2] < this->Bounds[4])
            {
            this->Bounds[4] = x[2];
            }
          if (x[2] > this->Bounds[5])
            {
            this->Bounds[5] = x[2];
            }
          }
        }
      }

    this->ComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
// Set the cell array defining vertices.
void vtkPolyData::SetVerts (vtkCellArray* v) 
{
  if (v == this->Dummy)
    {
    v = NULL;
    }
  if ( v != this->Verts)
    {
    if (this->Verts)
      {
      this->Verts->UnRegister(this);
      }
    this->Verts = v;
    if (this->Verts)
      {
      this->Verts->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the cell array defining vertices. If there are no vertices, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetVerts()
{
  if ( !this->Verts )
    {
    return this->Dummy;
    }
  else
    {
    return this->Verts;
    }
}

//----------------------------------------------------------------------------
// Set the cell array defining lines.
void vtkPolyData::SetLines (vtkCellArray* l) 
{
  if (l == this->Dummy)
    {
    l = NULL;
    }
  if ( l != this->Lines)
    {
    if (this->Lines)
      {
      this->Lines->UnRegister(this);
      }
    this->Lines = l;
    if (this->Lines)
      {
      this->Lines->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the cell array defining lines. If there are no lines, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetLines()
{
  if ( !this->Lines )
    {
    return this->Dummy;
    }
  else
    {
    return this->Lines;
    }
}

//----------------------------------------------------------------------------
// Set the cell array defining polygons.
void vtkPolyData::SetPolys (vtkCellArray* p) 
{
  if(p == this->Dummy) 
    {
    p = NULL;
    }
  if ( p != this->Polys)
    {
    if (this->Polys)
      {
      this->Polys->UnRegister(this);
      }
    this->Polys = p;
    if (this->Polys)
      {
      this->Polys->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the cell array defining polygons. If there are no polygons, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetPolys()
{
  if ( !this->Polys )
    {
    return this->Dummy;
    }
  else
    {
    return this->Polys;
    }
}

//----------------------------------------------------------------------------
// Set the cell array defining triangle strips.
void vtkPolyData::SetStrips (vtkCellArray* s) 
{
  if ( s == this->Dummy)
    {
    s = NULL;
    }
  if ( s != this->Strips)
    {
    if (this->Strips)
      {
      this->Strips->UnRegister(this);
      }
    this->Strips = s;
    if (this->Strips)
      {
      this->Strips->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the cell array defining triangle strips. If there are no
// triangle strips, an empty array will be returned (convenience to 
// simplify traversal).
vtkCellArray* vtkPolyData::GetStrips()
{
  if ( !this->Strips )
    {
    return this->Dummy;
    }
  else
    {
    return this->Strips;
    }
}

//----------------------------------------------------------------------------
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
}

//----------------------------------------------------------------------------
int vtkPolyData::GetMaxCellSize() 
{
  int maxCellSize=0, cellSize;

  if ( this->Verts ) 
    {
    cellSize = this->Verts->GetMaxCellSize();
    if ( cellSize > maxCellSize )
      {
      maxCellSize = cellSize;
      }
    }

  if ( this->Lines ) 
    {
    cellSize = this->Lines->GetMaxCellSize();
    if ( cellSize > maxCellSize )
      {
      maxCellSize = cellSize;
      }
    }

  if ( this->Polys ) 
    {
    cellSize = this->Polys->GetMaxCellSize();
    if ( cellSize > maxCellSize )
      {
      maxCellSize = cellSize;
      }
    }

  if ( this->Strips ) 
    {
    cellSize = this->Strips->GetMaxCellSize();
    if ( cellSize > maxCellSize )
      {
      maxCellSize = cellSize;
      }
    }

  return maxCellSize;
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyData::GetNumberOfCells() 
{
  return this->GetNumberOfVerts() + this->GetNumberOfLines() + 
         this->GetNumberOfPolys() + this->GetNumberOfStrips();
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyData::GetNumberOfVerts() 
{
  return (this->Verts ? this->Verts->GetNumberOfCells() : 0);
}

vtkIdType vtkPolyData::GetNumberOfLines() 
{
  return (this->Lines ? this->Lines->GetNumberOfCells() : 0);
}

vtkIdType vtkPolyData::GetNumberOfPolys() 
{
  return (this->Polys ? this->Polys->GetNumberOfCells() : 0);
}

vtkIdType vtkPolyData::GetNumberOfStrips() 
{
  return (this->Strips ? this->Strips->GetNumberOfCells() : 0);
}


//----------------------------------------------------------------------------
void vtkPolyData::DeleteCells()
{
  // if we have Links, we need to delete them (they are no longer valid)
  if (this->Links)
    {
    this->Links->UnRegister( this );
    this->Links = NULL;
    }
   
  if (this->Cells)
    {
    this->Cells->UnRegister( this );
    this->Cells = NULL;
    }
}

//----------------------------------------------------------------------------
// Create data structure that allows random access of cells.
void vtkPolyData::BuildCells()
{
  vtkIdType numCells;
  vtkCellArray *inVerts=this->GetVerts();
  vtkCellArray *inLines=this->GetLines();
  vtkCellArray *inPolys=this->GetPolys();
  vtkCellArray *inStrips=this->GetStrips();
  vtkIdType npts;
  vtkIdType *pts;
  vtkCellTypes *cells;

  vtkDebugMacro (<< "Building PolyData cells.");

  if ( (numCells = this->GetNumberOfCells()) < 1 )
    {
    numCells = 1000; //may be allocating empty list to begin with
    }

  if (this->Cells)
    {
    this->DeleteCells();
    }
  
  this->Cells = cells = vtkCellTypes::New();
  this->Cells->Allocate(numCells,3*numCells);
  this->Cells->Register(this);
  cells->Delete();
  //
  // Traverse various lists to create cell array
  //
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
    {
    if ( npts > 1 )
      {
      cells->InsertNextCell(VTK_POLY_VERTEX,
                            inVerts->GetTraversalLocation(npts));
      }
    else
      {
      cells->InsertNextCell(VTK_VERTEX,inVerts->GetTraversalLocation(npts));
      }
    }

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    if ( npts > 2 )
      {
      cells->InsertNextCell(VTK_POLY_LINE,inLines->GetTraversalLocation(npts));
      }
    else
      {
      cells->InsertNextCell(VTK_LINE,inLines->GetTraversalLocation(npts));
      } 
    }

  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    if ( npts == 3 )
      {
      cells->InsertNextCell(VTK_TRIANGLE,inPolys->GetTraversalLocation(npts));
      }
    else if ( npts == 4 )
      {
      cells->InsertNextCell(VTK_QUAD,inPolys->GetTraversalLocation(npts));
      }
    else
      {
      cells->InsertNextCell(VTK_POLYGON,inPolys->GetTraversalLocation(npts));
      }
    }

  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    cells->InsertNextCell(VTK_TRIANGLE_STRIP,
                          inStrips->GetTraversalLocation(npts));
    }
}

//----------------------------------------------------------------------------
void vtkPolyData::DeleteLinks()
{
  if (this->Links)
    {
    this->Links->UnRegister( this );
    this->Links = NULL;
    }
}

//----------------------------------------------------------------------------
// Create upward links from points to cells that use each point. Enables
// topologically complex queries.
void vtkPolyData::BuildLinks()
{
  if ( this->Links )
    {
    this->DeleteLinks();
    }
  
  if ( this->Cells == NULL )
    {
    this->BuildCells();
    }

  this->Links = vtkCellLinks::New();
  this->Links->Allocate(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->Delete();

  this->Links->BuildLinks(this);
}

//----------------------------------------------------------------------------
// Copy a cells point ids into list provided. (Less efficient.)
void vtkPolyData::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  vtkIdType i;
  vtkIdType *pts, npts;
  
  ptIds->Reset();
  if ( this->Cells == NULL )
    {
    this->BuildCells();
    }

  this->vtkPolyData::GetCellPoints(cellId, npts, pts);
  ptIds->InsertId (npts-1,pts[npts-1]);
  for (i=0; i<npts-1; i++)
    {
    ptIds->SetId(i,pts[i]);
    }
}

//----------------------------------------------------------------------------
// Return a pointer to a list of point ids defining cell. (More efficient.)
// Assumes that cells have been built (with BuildCells()).
void vtkPolyData::GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                                vtkIdType* &pts)
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

    default:
      npts = 0;
      pts = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPolyData::GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
{
  vtkIdType *cells;
  vtkIdType numCells;
  vtkIdType i;

  if ( ! this->Links )
    {
    this->BuildLinks();
    }
  cellIds->Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  for (i=0; i < numCells; i++)
    {
    cellIds->InsertId(i,cells[i]);
    }
}

//----------------------------------------------------------------------------
// Method allocates initial storage for vertex, line, polygon, and 
// triangle strip arrays. Use this method before the method 
// PolyData::InsertNextCell(). (Or, provide vertex, line, polygon, and
// triangle strip cell arrays.)
void vtkPolyData::Allocate(vtkIdType numCells, int extSize)
{
  vtkCellArray *cells;

  if (!this->Cells)
    {
    this->Cells = vtkCellTypes::New();
    this->Cells->Allocate(numCells,3*numCells);
    }

  cells = vtkCellArray::New();
  cells->Allocate(numCells,extSize);
  this->SetVerts(cells);
  cells->Delete();

  cells = vtkCellArray::New();
  cells->Allocate(numCells,extSize);
  this->SetLines(cells);
  cells->Delete();

  cells = vtkCellArray::New();
  cells->Allocate(numCells,extSize);
  this->SetPolys(cells);
  cells->Delete();

  cells = vtkCellArray::New();
  cells->Allocate(numCells,extSize);
  this->SetStrips(cells);
  cells->Delete();
}

//----------------------------------------------------------------------------
// Insert a cell of type vtkVERTEX, vtkPOLY_VERTEX, vtkLINE, vtkPOLY_LINE,
// vtkTRIANGLE, vtkQUAD, vtkPOLYGON, or vtkTRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert vtkPIXEL, but converts it to vtkQUAD.
int vtkPolyData::InsertNextCell(int type, int npts, vtkIdType *pts)
{
  int id;

  if ( !this->Cells ) 
    {
    // if we get to this point, the user has not made any guess at the
    // number of cells, so this guess is as good as any
    this->Cells = vtkCellTypes::New();
    this->Cells->Allocate(5000,10000);
    }

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
      this->Verts->InsertNextCell(npts,pts);
      id = this->Cells->InsertNextCell(type, 
                                       this->Verts->GetInsertLocation(npts));
      break;

    case VTK_LINE: case VTK_POLY_LINE:
      this->Lines->InsertNextCell(npts,pts);
      id = this->Cells->InsertNextCell(type, 
                                       this->Lines->GetInsertLocation(npts));
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      this->Polys->InsertNextCell(npts,pts);
      id = this->Cells->InsertNextCell(type, 
                                       this->Polys->GetInsertLocation(npts));
      break;

    case VTK_PIXEL: //need to rearrange vertices
      {
      static vtkIdType pixPts[4];
      pixPts[0] = pts[0];
      pixPts[1] = pts[1];
      pixPts[2] = pts[3];
      pixPts[3] = pts[2];
      this->Polys->InsertNextCell(npts,pixPts);
      id = this->Cells->InsertNextCell(VTK_QUAD, 
                                       this->Polys->GetInsertLocation(npts));
      break;
      }

    case VTK_TRIANGLE_STRIP:
      this->Strips->InsertNextCell(npts,pts);
      id = this->Cells->InsertNextCell(type, 
                                       this->Strips->GetInsertLocation(npts));
      break;

    default:
      id = -1;
      vtkErrorMacro(<<"Bad cell type! Can't insert!");
    }
  return id;
}

//----------------------------------------------------------------------------
// Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
// VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
int vtkPolyData::InsertNextCell(int type, vtkIdList *pts)
{
  int id;
  int npts=pts->GetNumberOfIds();

  if ( !this->Cells ) 
    {
    this->Cells = vtkCellTypes::New();
    this->Cells->Allocate(5000,10000);
    }

  switch (type)
    {
    case VTK_VERTEX: case VTK_POLY_VERTEX:
      this->Verts->InsertNextCell(pts);
      id = this->Cells->InsertNextCell(type, this->Verts->GetInsertLocation(npts));
      break;

    case VTK_LINE: case VTK_POLY_LINE:
      this->Lines->InsertNextCell(pts);
      id = this->Cells->InsertNextCell(type, this->Lines->GetInsertLocation(npts));
      break;

    case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
      this->Polys->InsertNextCell(pts);
      id = this->Cells->InsertNextCell(type, this->Polys->GetInsertLocation(npts));
      break;

    case VTK_PIXEL: //need to rearrange vertices
      {
      static vtkIdType pixPts[4];
      pixPts[0] = pts->GetId(0);
      pixPts[1] = pts->GetId(1);
      pixPts[2] = pts->GetId(3);
      pixPts[3] = pts->GetId(2);
      this->Polys->InsertNextCell(4,pixPts);
      id = this->Cells->InsertNextCell(VTK_QUAD, this->Polys->GetInsertLocation(npts));
      break;
      }

    case VTK_TRIANGLE_STRIP:
      this->Strips->InsertNextCell(pts);
      id = this->Cells->InsertNextCell(type, this->Strips->GetInsertLocation(npts));
      break;

    default:
      id = -1;
      vtkErrorMacro(<<"Bad cell type! Can't insert!");
    }

  return id;
}

//----------------------------------------------------------------------------
// Recover extra allocated memory when creating data whose initial size
// is unknown. Examples include using the InsertNextCell() method, or
// when using the CellArray::EstimateSize() method to create vertices,
// lines, polygons, or triangle strips.
void vtkPolyData::Squeeze()
{
  if ( this->Verts != NULL )
    {
    this->Verts->Squeeze();
    }
  if ( this->Lines != NULL )
    {
    this->Lines->Squeeze();
    }
  if ( this->Polys != NULL )
    {
    this->Polys->Squeeze();
    }
  if ( this->Strips != NULL )
    {
    this->Strips->Squeeze();
    }

  vtkPointSet::Squeeze();
}

//----------------------------------------------------------------------------
// Begin inserting data all over again. Memory is not freed but otherwise
// objects are returned to their initial state.
void vtkPolyData::Reset()
{
  if ( this->Verts != NULL )
    {
    this->Verts->Reset();
    }
  if ( this->Lines != NULL )
    {
    this->Lines->Reset();
    }
  if ( this->Polys != NULL )
    {
    this->Polys->Reset();
    }
  if ( this->Strips != NULL )
    {
    this->Strips->Reset();
    }
}

//----------------------------------------------------------------------------
// Reverse the order of point ids defining the cell.
void vtkPolyData::ReverseCell(vtkIdType cellId)
{
  int loc, type;

  if ( this->Cells == NULL )
    {
    this->BuildCells();
    }
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

    default:
      break;
    }
}

//----------------------------------------------------------------------------
// Add a point to the cell data structure (after cell pointers have been
// built). This method adds the point and then allocates memory for the
// links to the cells.  (To use this method, make sure points are available
// and BuildLinks() has been invoked.)
int vtkPolyData::InsertNextLinkedPoint(float x[3], int numLinks)
{
  this->Links->InsertNextPoint(numLinks);
  return this->Points->InsertNextPoint(x);
}

//----------------------------------------------------------------------------
// Add a new cell to the cell data structure (after cell pointers have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.)
int vtkPolyData::InsertNextLinkedCell(int type, int npts, vtkIdType *pts)
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

//----------------------------------------------------------------------------
// Remove a reference to a cell in a particular point's link list. You may also
// consider using RemoveCellReference() to remove the references from all the 
// cell's points to the cell. This operator does not reallocate memory; use the
// operator ResizeCellList() to do this if necessary.
void vtkPolyData::RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  this->Links->RemoveCellReference(cellId, ptId);  
}

//----------------------------------------------------------------------------
// Add a reference to a cell in a particular point's link list. (You may also
// consider using AddCellReference() to add the references from all the 
// cell's points to the cell.) This operator does not realloc memory; use the
// operator ResizeCellList() to do this if necessary.
void vtkPolyData::AddReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  this->Links->AddCellReference(cellId, ptId);  
}

//----------------------------------------------------------------------------
// Replace the points defining cell "cellId" with a new set of points. This
// operator is (typically) used when links from points to cells have not been 
// built (i.e., BuildLinks() has not been executed). Use the operator 
// ReplaceLinkedCell() to replace a cell when cell structure has been built.
void vtkPolyData::ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts)
{
  int loc, type;

  if ( this->Cells == NULL )
    {
    this->BuildCells();
    }
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

    default:
      break;
    }
}

//----------------------------------------------------------------------------
// Replace one cell with another in cell structure. This operator updates the
// connectivity list and the point's link list. It does not delete references
// to the old cell in the point's link list. Use the operator 
// RemoveCellReference() to delete all references from points to (old) cell.
// You may also want to consider using the operator ResizeCellList() if the 
// link list is changing size.
void vtkPolyData::ReplaceLinkedCell(vtkIdType cellId, int npts, vtkIdType *pts)
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

    default:
      npts = 0;
    }

  for (int i=0; i < npts; i++)
    {
    this->Links->InsertNextCellReference(pts[i],cellId);
    }
}

//----------------------------------------------------------------------------
// Get the neighbors at an edge. More efficient than the general 
// GetCellNeighbors(). Assumes links have been built (with BuildLinks()), 
// and looks specifically for edge neighbors.
void vtkPolyData::GetCellEdgeNeighbors(vtkIdType cellId, int p1, int p2,
                                       vtkIdList *cellIds)
{
  vtkIdType *cells;
  vtkIdType numCells;
  vtkIdType i,j;
  vtkIdType *pts, npts;
  
  cellIds->Reset();

  numCells = this->Links->GetNcells(p1);
  cells = this->Links->GetCells(p1);

  for (i=0; i < numCells; i++)
    {
    if ( cells[i] != cellId )
      {
      this->GetCellPoints(cells[i],npts,pts);
      for (j=0; j < npts; j++)
        {
        if ( pts[j] == p2 )
          {
          break;
          }
        }
      if ( j < npts )
        {
        cellIds->InsertNextId(cells[i]);
        }
      }
    }
}


static int idsortcompare (const void *arg1, const void *arg2)
{
  int *i1 = (int *) arg1;
  int *i2 = (int *) arg2;

  if (i1[0] < i2[0])
    {
      return -1;
    }
  if (i1[0] > i2[0])
    {
      return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPolyData::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                   vtkIdList *cellIds)
{
  vtkIdType i, numPts, cellNum;
  vtkIdType ptId, numPrime;
  int listsize;
  vtkIdType *primeCells;
  
  if ( ! this->Links )
    {
    this->BuildLinks();
    }  
  
  cellIds->Reset();

  // Compute the id list size
  numPts=ptIds->GetNumberOfIds();
  listsize = 0;
  for (i = 0; i < numPts; i++)
    {
      ptId       = ptIds->GetId(i);
      numPrime   = this->Links->GetNcells(ptId);
      listsize  += numPrime;
    }

  // Create an int array for storage and sorting
  int *allCells, index;
  allCells = new int [listsize];
  
  // Load list with candidate cells, remove current cell
  for (index = 0, i = 0; i < numPts; i++)
    {
      ptId       = ptIds->GetId(i);
      numPrime   = this->Links->GetNcells(ptId);
      primeCells = this->Links->GetCells(ptId);

      for (cellNum = 0; cellNum < numPrime; cellNum++)
	{
	  // ignore the original cell
	  if (primeCells[cellNum] != cellId)
	    {
	      allCells[index++] = primeCells[cellNum];
	    }
	}
    }

  // Sort the list and remove duplications
  qsort (allCells, index, sizeof(int), idsortcompare);

  // Cull duplicates
  if (index > 0)
    {
      int currid = allCells[0];
      for (i = 1; i < index; i++)
	{
	  if (allCells[i] == currid)
	    {
	    continue;
	    }
	  cellIds->InsertNextId(currid);
	  currid = allCells[i];
	}
      cellIds->InsertNextId(currid);
    }

  delete [] allCells;

}


//----------------------------------------------------------------------------

void vtkPolyData::SetUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numPieces;
  this->UpdateGhostLevel = ghostLevel;
  this->UpdateExtentInitialized = 1;
}

//----------------------------------------------------------------------------

void vtkPolyData::GetUpdateExtent(int &piece, int &numPieces, int &ghostLevel)
{
  piece = this->UpdatePiece;
  numPieces = this->UpdateNumberOfPieces;
  ghostLevel = this->UpdateGhostLevel;
}

//----------------------------------------------------------------------------

unsigned long vtkPolyData::GetActualMemorySize()
{
  unsigned long size=this->vtkPointSet::GetActualMemorySize();
  if ( this->Verts ) 
    {
    size += this->Verts->GetActualMemorySize();
    }
  if ( this->Lines ) 
    {
    size += this->Lines->GetActualMemorySize();
    }
  if ( this->Polys ) 
    {
    size += this->Polys->GetActualMemorySize();
    }
  if ( this->Strips ) 
    {
    size += this->Strips->GetActualMemorySize();
    }
  if ( this->Cells )
    {
    size += this->Cells->GetActualMemorySize();
    }
  if ( this->Links )
    {
    size += this->Links->GetActualMemorySize();
    }
  return size;
}

//----------------------------------------------------------------------------
void vtkPolyData::ShallowCopy(vtkDataObject *dataObject)
{
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(dataObject);

  if ( polyData != NULL )
    {
    this->SetVerts(polyData->GetVerts());
    this->SetLines(polyData->GetLines());
    this->SetPolys(polyData->GetPolys());
    this->SetStrips(polyData->GetStrips());
    
    // I do not know if this is correct but.
    if (this->Cells)
      {
      this->Cells->Delete();
      }
    this->Cells = polyData->Cells;
    if (this->Cells)
      {
      this->Cells->Register(this);
      }

    if (this->Links)
      {
      this->Links->Delete();
      }
    this->Links = polyData->Links;
    if (this->Links)
      {
      this->Links->Register(this);
      }
    }

  // Do superclass
  this->vtkPointSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkPolyData::DeepCopy(vtkDataObject *dataObject)
{
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(dataObject);

  if ( polyData != NULL )
    {
    vtkCellArray *ca;
    ca = vtkCellArray::New();
    ca->DeepCopy(polyData->GetVerts());
    this->SetVerts(ca);
    ca->Delete();

    ca = vtkCellArray::New();
    ca->DeepCopy(polyData->GetLines());
    this->SetLines(ca);
    ca->Delete();

    ca = vtkCellArray::New();
    ca->DeepCopy(polyData->GetPolys());
    this->SetPolys(ca);
    ca->Delete();

    ca = vtkCellArray::New();
    ca->DeepCopy(polyData->GetStrips());
    this->SetStrips(ca);
    ca->Delete();

    if ( this->Cells )
      {
      this->Cells->UnRegister(this);
      this->Cells = NULL;
      }
    if (polyData->Cells)
      {
      this->Cells = vtkCellTypes::New();
      this->Cells->DeepCopy(polyData->Cells);
      this->Cells->Register(this);
      this->Cells->Delete();
      }

    if ( this->Links )
      {
      this->Links->UnRegister(this);
      this->Links = NULL;
      }
    if (polyData->Links)
      {
      this->Links = vtkCellLinks::New();
      this->Links->DeepCopy(polyData->Links);
      this->Links->Register(this);
      this->Links->Delete();
      }
    }

  // Do superclass
  this->vtkPointSet::DeepCopy(dataObject);
}


void vtkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);

  os << indent << "Number Of Vertices: " << this->GetNumberOfVerts() << "\n";
  os << indent << "Number Of Lines: " << this->GetNumberOfLines() << "\n";
  os << indent << "Number Of Polygons: " << this->GetNumberOfPolys() << "\n";
  os << indent << "Number Of Triangle Strips: " << this->GetNumberOfStrips() << "\n";

  os << indent << "Number Of Pieces: " << this->NumberOfPieces << endl;
  os << indent << "Piece: " << this->Piece << endl;
  os << indent << "Ghost Level: " << this->GhostLevel << endl;
  
  os << indent << "UpdateExtent: " << this->UpdateExtent[0] << ", "
     << this->UpdateExtent[1] << ", " << this->UpdateExtent[2] << ", "
     << this->UpdateExtent[3] << ", " << this->UpdateExtent[4] << ", "
     << this->UpdateExtent[5] << endl;
}






