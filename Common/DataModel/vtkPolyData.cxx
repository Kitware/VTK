/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCriticalSection.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkVertex.h"

#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPolyData);

//----------------------------------------------------------------------------
// Initialize static member.  This member is used to simplify traversal
// of verts, lines, polygons, and triangle strips lists.  It basically
// "marks" empty lists so that the traveral method "GetNextCell"
// works properly.

struct vtkPolyDataDummyContainter
{
  vtkSmartPointer<vtkCellArray> Dummy;

  vtkPolyDataDummyContainter()
  {
      this->Dummy.TakeReference(vtkCellArray::New());
  }
};

vtkPolyDataDummyContainter vtkPolyData::DummyContainer;

vtkPolyData::vtkPolyData () :
  Vertex(NULL), PolyVertex(NULL), Line(NULL), PolyLine(NULL),
  Triangle(NULL), Quad(NULL), Polygon(NULL), TriangleStrip(NULL),
  EmptyCell(NULL), Verts(NULL), Lines(NULL), Polys(NULL),
  Strips(NULL), Cells(NULL), Links(NULL)
{
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkPolyData::~vtkPolyData()
{
  this->Cleanup();

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
int vtkPolyData::GetPiece()
{
  return this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
}

//----------------------------------------------------------------------------
int vtkPolyData::GetNumberOfPieces()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
int vtkPolyData::GetGhostLevel()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input poly data object.
void vtkPolyData::CopyStructure(vtkDataSet *ds)
{
  vtkPolyData *pd=static_cast<vtkPolyData *>(ds);
  vtkPointSet::CopyStructure(ds);

  if (this->Verts != pd->Verts)
  {
    if (this->Verts)
    {
      this->Verts->UnRegister(this);
    }
    this->Verts = pd->Verts;
    if (this->Verts)
    {
      this->Verts->Register(this);
    }
  }

  if (this->Lines != pd->Lines)
  {
    if (this->Lines)
    {
      this->Lines->UnRegister(this);
    }
    this->Lines = pd->Lines;
    if (this->Lines)
    {
      this->Lines->Register(this);
    }
  }

  if (this->Polys != pd->Polys)
  {
    if (this->Polys)
    {
      this->Polys->UnRegister(this);
    }
    this->Polys = pd->Polys;
    if (this->Polys)
    {
      this->Polys->Register(this);
    }
  }

  if (this->Strips != pd->Strips)
  {
    if (this->Strips)
    {
      this->Strips->UnRegister(this);
    }
    this->Strips = pd->Strips;
    if (this->Strips)
    {
      this->Strips->Register(this);
    }
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
  vtkIdType       *pts=0;
  vtkIdType       numPts;
  unsigned char   type;
  double           x[3];

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

//----------------------------------------------------------------------------
void vtkPolyData::CopyCells(vtkPolyData *pd, vtkIdList *idList,
                            vtkPointLocator *locator)
{
  vtkIdType cellId, ptId, newId, newCellId, locatorPtId;
  int numPts, numCellPts, i;
  vtkPoints *newPoints;
  vtkIdList *pointMap = vtkIdList::New(); //maps old pt ids into new
  vtkIdList *cellPts, *newCellPts = vtkIdList::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  double x[3];
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
void vtkPolyData::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  int i, loc;
  vtkIdType *pts, numPts;
  unsigned char type;
  double x[3];

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

  // carefully compute the bounds
  if (numPts)
  {
    this->Points->GetPoint( pts[0], x );
    bounds[0] = x[0];
    bounds[2] = x[1];
    bounds[4] = x[2];
    bounds[1] = x[0];
    bounds[3] = x[1];
    bounds[5] = x[2];
    for (i=1; i < numPts; i++)
    {
      this->Points->GetPoint( pts[i], x );
      bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
      bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
      bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
      bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
      bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
      bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
    }
  }
  else
  {
    vtkMath::UninitializeBounds(bounds);
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
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    double x[3];

    vtkCellArray *cella[4];

    cella[0] = this->GetVerts();
    cella[1] = this->GetLines();
    cella[2] = this->GetPolys();
    cella[3] = this->GetStrips();

    // carefully compute the bounds
    int doneOne = 0;
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;

    // Iterate over cells's points
    for (t = 0; t < 4; t++)
    {
      for (cella[t]->InitTraversal(); cella[t]->GetNextCell(npts,pts); )
      {
        for (i = 0;  i < npts; i++)
        {
          this->Points->GetPoint( pts[i], x );
          this->Bounds[0] = (x[0] < this->Bounds[0] ? x[0] : this->Bounds[0]);
          this->Bounds[1] = (x[0] > this->Bounds[1] ? x[0] : this->Bounds[1]);
          this->Bounds[2] = (x[1] < this->Bounds[2] ? x[1] : this->Bounds[2]);
          this->Bounds[3] = (x[1] > this->Bounds[3] ? x[1] : this->Bounds[3]);
          this->Bounds[4] = (x[2] < this->Bounds[4] ? x[2] : this->Bounds[4]);
          this->Bounds[5] = (x[2] > this->Bounds[5] ? x[2] : this->Bounds[5]);
          doneOne = 1;
        }
      }
    }
    if (!doneOne)
    {
      vtkMath::UninitializeBounds(this->Bounds);
    }
    this->ComputeTime.Modified();
  }
}

//----------------------------------------------------------------------------
// Set the cell array defining vertices.
void vtkPolyData::SetVerts (vtkCellArray* v)
{
  if (v == this->DummyContainer.Dummy.GetPointer())
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
    return this->DummyContainer.Dummy.GetPointer();
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
  if (l == this->DummyContainer.Dummy.GetPointer())
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
    return this->DummyContainer.Dummy.GetPointer();
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
  if(p == this->DummyContainer.Dummy.GetPointer())
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
    return this->DummyContainer.Dummy.GetPointer();
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
  if ( s == this->DummyContainer.Dummy.GetPointer())
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
    return this->DummyContainer.Dummy.GetPointer();
  }
  else
  {
    return this->Strips;
  }
}

//----------------------------------------------------------------------------
void vtkPolyData::Cleanup()
{
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
// Restore object to initial state. Release memory back to system.
void vtkPolyData::Initialize()
{
  vtkPointSet::Initialize();

  this->Cleanup();

  if(this->Information)
  {
    this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 0);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
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
  if (this->Cells)
  {
    this->DeleteCells();
  }

  vtkCellArray *vertCells = this->GetVerts();
  vtkCellArray *lineCells = this->GetLines();
  vtkCellArray *polyCells = this->GetPolys();
  vtkCellArray *stripCells = this->GetStrips();

  // here are the number of cells we have
  vtkIdType nVerts = vertCells->GetNumberOfCells();
  vtkIdType nLines = lineCells->GetNumberOfCells();
  vtkIdType nPolys = polyCells->GetNumberOfCells();
  vtkIdType nStrips = stripCells->GetNumberOfCells();

  // pre-allocate the space we need
  vtkIdType nCells = nVerts + nLines + nPolys + nStrips;

  vtkUnsignedCharArray *types = vtkUnsignedCharArray::New();
  unsigned char *pTypes = types->WritePointer(0, nCells);

  vtkIntArray *locs = vtkIntArray::New();
  int *pLocs = locs->WritePointer(0, nCells);

  // record locations and type of each cell.
  // verts
  vtkIdType numCellPts;
  vtkIdType nextCellPts;
  if (nVerts)
  {
    vtkIdType *pVerts = vertCells->GetData()->GetPointer(0);
    numCellPts = pVerts[0];
    nextCellPts = numCellPts + 1;
    pLocs[0] = 0;
    pTypes[0] = numCellPts > 1 ? VTK_POLY_VERTEX : VTK_VERTEX;
    for (vtkIdType i = 1; i < nVerts; ++i)
    {
      numCellPts = pVerts[nextCellPts];
      pLocs[i] = nextCellPts;
      pTypes[i] = numCellPts > 1 ? VTK_POLY_VERTEX : VTK_VERTEX;
      nextCellPts += numCellPts + 1;
    }
    pLocs += nVerts;
    pTypes += nVerts;
  }

  // lines
  if (nLines)
  {
    vtkIdType *pLines = lineCells->GetData()->GetPointer(0);
    numCellPts = pLines[0];
    pLocs[0] = 0;
    pTypes[0] = numCellPts > 2 ? VTK_POLY_LINE : VTK_LINE;
    if (numCellPts == 1)
    {
      vtkWarningMacro("Building VTK_LINE 0 with only one point, but VTK_LINE "
      "needs at least two points. Check the input.");
    }
    nextCellPts = numCellPts + 1;
    for (vtkIdType i = 1; i < nLines; ++i)
    {
      numCellPts = pLines[nextCellPts];
      pLocs[i] = nextCellPts;
      pTypes[i] = numCellPts > 2 ? VTK_POLY_LINE : VTK_LINE;
      if (numCellPts == 1)
      {
        vtkWarningMacro("Building VTK_LINE " << i <<" with only one point, but "
        "VTK_LINE needs at least two points. Check the input.");
      }
      nextCellPts += numCellPts + 1;
    }
    pLocs += nLines;
    pTypes += nLines;
  }

  // polys
  if (nPolys)
  {
    vtkIdType *pPolys = polyCells->GetData()->GetPointer(0);
    numCellPts = pPolys[0];
    pLocs[0] = 0;
    if (numCellPts < 3)
    {
      vtkWarningMacro("Building VTK_TRIANGLE 0 with less than three points, but"
      " VTK_TRIANGLE needs at least three points. Check the input.");
    }
    pTypes[0] = numCellPts == 3 ? VTK_TRIANGLE :
      numCellPts == 4 ? VTK_QUAD : VTK_POLYGON;
    nextCellPts = numCellPts + 1;
    for (vtkIdType i = 1; i < nPolys; ++i)
    {
      numCellPts = pPolys[nextCellPts];
      pLocs[i] = nextCellPts;
      if (numCellPts < 3)
      {
        vtkWarningMacro("Building VTK_TRIANGLE "<< i << " with less than three "
        "points, but VTK_TRIANGLE needs at least three points. "
        "Check the input.");
      }
      pTypes[i] = numCellPts == 3 ? VTK_TRIANGLE :
        numCellPts == 4 ? VTK_QUAD : VTK_POLYGON;
      nextCellPts += numCellPts + 1;
    }
    pLocs += nPolys;
    pTypes += nPolys;
  }

  // strips
  if (nStrips)
  {
    std::fill_n(pTypes, nStrips, VTK_TRIANGLE_STRIP);
    vtkIdType *pStrips = stripCells->GetData()->GetPointer(0);
    numCellPts = pStrips[0];
    pLocs[0] = 0;
    nextCellPts = numCellPts + 1;
    for (vtkIdType i = 1; i < nStrips; ++i)
    {
      numCellPts = pStrips[nextCellPts];
      pLocs[i] = nextCellPts;
      nextCellPts += numCellPts + 1;
    }
  }

  // set up the cell types data structure
  this->Cells = vtkCellTypes::New();
  this->Cells->SetCellTypes(nCells, types, locs);
  this->Cells->Register(this);
  this->Cells->Delete();
  types->Delete();
  locs->Delete();
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
void vtkPolyData::BuildLinks(int initialSize)
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
  if ( initialSize > 0 )
  {
    this->Links->Allocate(initialSize);
  }
  else
  {
    this->Links->Allocate(this->GetNumberOfPoints());
  }
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
    // Consistent Register/UnRegister. (ShallowCopy).
    this->Cells->Register(this);
    this->Cells->Delete();
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

void vtkPolyData::Allocate(vtkPolyData *inPolyData, vtkIdType numCells,
                           int extSize)
{
  vtkCellArray *cells;
  int numVerts=inPolyData->GetVerts()->GetNumberOfCells();
  int numLines=inPolyData->GetLines()->GetNumberOfCells();
  int numPolys=inPolyData->GetPolys()->GetNumberOfCells();
  int numStrips=inPolyData->GetStrips()->GetNumberOfCells();
  int total=numVerts+numLines+numPolys+numStrips;

  if ( total <= 0 )
  {
    return;
  }

  if (!this->Cells)
  {
    this->Cells = vtkCellTypes::New();
    this->Cells->Allocate(numCells,3*numCells);
    // Consistent Register/UnRegister. (ShallowCopy).
    this->Cells->Register(this);
    this->Cells->Delete();
  }

  if ( numVerts > 0 )
  {
    cells = vtkCellArray::New();
    cells->Allocate(
      static_cast<int>(static_cast<double>(numVerts)/total*numCells),extSize);
    this->SetVerts(cells);
    cells->Delete();
  }
  if ( numLines > 0 )
  {
    cells = vtkCellArray::New();
    cells->Allocate(
      static_cast<int>(static_cast<double>(numLines)/total*numCells),extSize);
    this->SetLines(cells);
    cells->Delete();
  }
  if ( numPolys > 0 )
  {
    cells = vtkCellArray::New();
    cells->Allocate(
      static_cast<int>(static_cast<double>(numPolys)/total*numCells),extSize);
    this->SetPolys(cells);
    cells->Delete();
  }
  if ( numStrips > 0 )
  {
    cells = vtkCellArray::New();
    cells->Allocate(
      static_cast<int>(static_cast<double>(numStrips)/total*numCells),extSize);
    this->SetStrips(cells);
    cells->Delete();
  }
}

//----------------------------------------------------------------------------
// Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
// VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
vtkIdType vtkPolyData::InsertNextCell(int type, int npts, vtkIdType *pts)
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
      vtkIdType pixPts[4];
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
vtkIdType vtkPolyData::InsertNextCell(int type, vtkIdList *pts)
{
  vtkIdType id;
  vtkIdType npts = pts->GetNumberOfIds();

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
      vtkIdType pixPts[4];
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

    case VTK_EMPTY_CELL:
      id = -1;
      // do nothing
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

  // discard Links and Cells
  this->DeleteLinks();
  this->DeleteCells();
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
// built). This method allocates memory for the links to the cells.  (To
// use this method, make sure points are available and BuildLinks() has been invoked.)
vtkIdType vtkPolyData::InsertNextLinkedPoint(int numLinks)
{
  return this->Links->InsertNextPoint(numLinks);
}

//----------------------------------------------------------------------------
// Add a point to the cell data structure (after cell pointers have been
// built). This method adds the point and then allocates memory for the
// links to the cells.  (To use this method, make sure points are available
// and BuildLinks() has been invoked.)
vtkIdType vtkPolyData::InsertNextLinkedPoint(double x[3], int numLinks)
{
  this->Links->InsertNextPoint(numLinks);
  return this->Points->InsertNextPoint(x);
}

//----------------------------------------------------------------------------
// Add a new cell to the cell data structure (after cell pointers have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.)
vtkIdType vtkPolyData::InsertNextLinkedCell(int type, int npts, vtkIdType *pts)
{
  vtkIdType i, id;

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
void vtkPolyData::GetCellEdgeNeighbors(vtkIdType cellId, vtkIdType p1,
                                       vtkIdType p2, vtkIdList *cellIds)
{
  cellIds->Reset();

  const vtkCellLinks::Link &link1(this->Links->GetLink(p1));
  const vtkCellLinks::Link &link2(this->Links->GetLink(p2));

  const vtkIdType *cells1 = link1.cells;
  const vtkIdType *cells1End = cells1 + link1.ncells;

  const vtkIdType *cells2 = link2.cells;
  const vtkIdType *cells2End = cells2 + link2.ncells;

  while (cells1 != cells1End)
  {
    if (*cells1 != cellId)
    {
      const vtkIdType *cells2Cur(cells2);
      while (cells2Cur != cells2End)
      {
        if (*cells1 == *cells2Cur)
        {
          cellIds->InsertNextId(*cells1);
          break;
        }
        ++cells2Cur;
      }
    }
    ++cells1;
  }
}

//----------------------------------------------------------------------------
void vtkPolyData::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                   vtkIdList *cellIds)
{
  vtkIdType i, j, numPts, cellNum;
  int allFound, oneFound;

  if ( ! this->Links )
  {
    this->BuildLinks();
  }

  cellIds->Reset();

  // load list with candidate cells, remove current cell
  vtkIdType ptId = ptIds->GetId(0);
  int numPrime = this->Links->GetNcells(ptId);
  vtkIdType *primeCells = this->Links->GetCells(ptId);
  numPts = ptIds->GetNumberOfIds();

  // for each potential cell
  for (cellNum = 0; cellNum < numPrime; cellNum++)
  {
    // ignore the original cell
    if (primeCells[cellNum] != cellId)
    {
      // are all the remaining points in the cell ?
      for (allFound=1, i=1; i < numPts && allFound; i++)
      {
        ptId = ptIds->GetId(i);
        int numCurrent = this->Links->GetNcells(ptId);
        vtkIdType *currentCells = this->Links->GetCells(ptId);
        oneFound = 0;
        for (j = 0; j < numCurrent; j++)
        {
          if (primeCells[cellNum] == currentCells[j])
          {
            oneFound = 1;
            break;
          }
        }
        if (!oneFound)
        {
          allFound = 0;
        }
      }
      if (allFound)
      {
        cellIds->InsertNextId(primeCells[cellNum]);
      }
    }
  }
}

int vtkPolyData::IsEdge(vtkIdType p1, vtkIdType p2)
{
  unsigned short int ncells;
  vtkIdType cellType;
  vtkIdType npts;
  vtkIdType i, j;
  vtkIdType *cells, *pts;

  vtkIdType nbPoints = this->GetNumberOfPoints();
  if (p1 >= nbPoints || p2 >= nbPoints)
  {
    return 0;
  }

  this->GetPointCells(p1,ncells,cells);
  for (i=0; i<ncells; i++)
  {
    cellType = this->GetCellType(cells[i]);
    switch (cellType)
    {
      case VTK_EMPTY_CELL: case VTK_VERTEX: case VTK_POLY_VERTEX: case VTK_LINE: case VTK_POLY_LINE:
        break;
      case VTK_TRIANGLE:
        if ( this->IsPointUsedByCell(p2,cells[i]) )
        {
          return 1;
        }
        break;
      case VTK_QUAD:
        this->GetCellPoints(cells[i],npts,pts);
        for (j=0; j<npts-1; j++)
        {
          if (((pts[j]==p1)&&(pts[j+1]==p2))||((pts[j]==p2)&&(pts[j+1]==p1)))
          {
            return 1;
          }
        }
        if (((pts[0]==p1)&&(pts[npts-1]==p2))||((pts[0]==p2)&&(pts[npts-1]==p1)))
        {
          return 1;
        }
        break;
      case VTK_TRIANGLE_STRIP:
        this->GetCellPoints(cells[i],npts,pts);
        for (j=0; j<npts-2; j++)
        {
          if ((((pts[j]==p1)&&(pts[j+1]==p2))||((pts[j]==p2)&&(pts[j+1]==p1)))||
              (((pts[j]==p1)&&(pts[j+2]==p2))||((pts[j]==p2)&&(pts[j+2]==p1))))
          {
            return 1;
          }
        }
        if (((pts[npts-2]==p1)&&(pts[npts-1]==p2))||((pts[npts-2]==p2)&&(pts[npts-1]==p1)))
        {
          return 1;
        }
        break;
      default:
        this->GetCellPoints(cells[i],npts,pts);
        for (j=0; j<npts; j++)
        {
          if (p1==pts[j])
          {
            if ((pts[(j-1+npts)%npts]==p2)||(pts[(j+1)%npts]==p2))
            {
              return 1;
            }
          }
        }
    }
  }
  return 0;
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
  if (this == polyData)
     return;

  if ( polyData != NULL )
  {
    this->SetVerts(polyData->GetVerts());
    this->SetLines(polyData->GetLines());
    this->SetPolys(polyData->GetPolys());
    this->SetStrips(polyData->GetStrips());

    // I do not know if this is correct but.
    if (this->Cells)
    {
      this->Cells->UnRegister(this);
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
  // Do superclass
  // We have to do this BEFORE we call BuildLinks, else there are no points
  // to build the links on (the parent DeepCopy copies the points)
  this->vtkPointSet::DeepCopy(dataObject);

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
      this->BuildCells();
    }

    if ( this->Links )
    {
      this->Links->UnRegister(this);
      this->Links = NULL;
    }
    if (polyData->Links)
    {
      this->BuildLinks();
    }
  }
}

void vtkPolyData::RemoveGhostCells()
{
  // Get a pointer to the cell ghost level array.
  vtkUnsignedCharArray* temp = this->GetCellGhostArray();
  if (temp == NULL)
  {
    vtkDebugMacro("Could not find cell ghost array.");
    return;
  }
  if (temp->GetNumberOfComponents() != 1 ||
      temp->GetNumberOfTuples() < this->GetNumberOfCells())
  {
    vtkErrorMacro("Poorly formed ghost array.");
    return;
  }
  unsigned char* cellGhosts = temp->GetPointer(0);

  vtkIdType numCells = this->GetNumberOfCells();

  vtkIntArray *types = vtkIntArray::New();
  types->SetNumberOfValues(numCells);

  for (vtkIdType i = 0; i < numCells; i++)
  {
    types->SetValue(i, this->GetCellType(i));
  }

  this->DeleteCells();

  // we have to make new copies of Verts, Lines, Polys
  // and Strips since they may be shared with other polydata
  vtkSmartPointer<vtkCellArray> verts;
  if(this->Verts)
  {
    verts = this->Verts;
    verts->InitTraversal();
    this->Verts->UnRegister(this);
    this->Verts = vtkCellArray::New();
  }

  vtkSmartPointer<vtkCellArray> lines;
  if(this->Lines)
  {
    lines = this->Lines;
    lines->InitTraversal();
    this->Lines->UnRegister(this);
    this->Lines = vtkCellArray::New();
  }

  vtkSmartPointer<vtkCellArray> polys;
  if(this->Polys)
  {
    polys = this->Polys;
    polys->InitTraversal();
    this->Polys->UnRegister(this);
    this->Polys = vtkCellArray::New();
  }

  vtkSmartPointer<vtkCellArray> strips;
  if(this->Strips)
  {
    strips = this->Strips;
    strips->InitTraversal();
    this->Strips->UnRegister(this);
    this->Strips = vtkCellArray::New();
  }

  vtkCellData *newCellData = vtkCellData::New();
  newCellData->CopyAllocate(this->CellData, numCells);

  vtkIdType *pts, n;

  int cellId;

  for (vtkIdType i = 0; i < numCells; i++)
  {
    int type = types->GetValue(i);

    if (type == VTK_VERTEX || type == VTK_POLY_VERTEX)
    {
      verts->GetNextCell(n, pts);

      if (! (cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
    else if (type == VTK_LINE || type == VTK_POLY_LINE)
    {
      lines->GetNextCell(n, pts);

      if (! (cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
    else if (type == VTK_POLYGON || type == VTK_TRIANGLE || type == VTK_QUAD)
    {
      polys->GetNextCell(n, pts);

      if (! (cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
    else if (type == VTK_TRIANGLE_STRIP)
    {
      strips->GetNextCell(n, pts);

      if (! (cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
  }

  newCellData->Squeeze();

  this->CellData->ShallowCopy(newCellData);
  newCellData->Delete();

  types->Delete();

  // If there are no more ghost levels, then remove all arrays.
  this->CellData->RemoveArray(vtkDataSetAttributes::GhostArrayName());

  this->Squeeze();
}
//----------------------------------------------------------------------------
void vtkPolyData::RemoveDeletedCells()
{
  if (!this->Cells)
  {
      return;
  }

  vtkNew<vtkPolyData> oldData;
  oldData->ShallowCopy(this);
  this->DeleteCells();

  if(this->Verts)
  {
    this->Verts->UnRegister(this);
    this->Verts = vtkCellArray::New();
  }
  if(this->Lines)
  {
    this->Lines->UnRegister(this);
    this->Lines = vtkCellArray::New();
  }
  if(this->Polys)
  {
    this->Polys->UnRegister(this);
    this->Polys = vtkCellArray::New();
  }
  if(this->Strips)
  {
    this->Strips->UnRegister(this);
    this->Strips = vtkCellArray::New();
  }

  this->CellData->CopyAllocate(oldData->GetCellData());

  const vtkIdType numCells = oldData->GetNumberOfCells();
  vtkCell *cell;
  vtkIdType cellId;
  vtkIdList *pointIds;
  int type;
  for (vtkIdType i = 0; i < numCells; i++)
  {
    type = oldData->GetCellType(i);

    if (type != VTK_EMPTY_CELL)
    {
      cell = oldData->GetCell(i);
      pointIds = cell->GetPointIds();
      cellId = this->InsertNextCell(type, pointIds);
      this->CellData->CopyData(oldData->GetCellData(), i, cellId);
    }
  }

  this->CellData->Squeeze();
}
//----------------------------------------------------------------------------
vtkPolyData* vtkPolyData::GetData(vtkInformation* info)
{
  return info? vtkPolyData::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyData::GetData(vtkInformationVector* v, int i)
{
  return vtkPolyData::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Vertices: " << this->GetNumberOfVerts() << "\n";
  os << indent << "Number Of Lines: " << this->GetNumberOfLines() << "\n";
  os << indent << "Number Of Polygons: " << this->GetNumberOfPolys() << "\n";
  os << indent << "Number Of Triangle Strips: " << this->GetNumberOfStrips() << "\n";

  os << indent << "Number Of Pieces: " << this->GetNumberOfPieces() << endl;
  os << indent << "Piece: " << this->GetPiece() << endl;
  os << indent << "Ghost Level: " << this->GetGhostLevel() << endl;
}


//----------------------------------------------------------------------------
int vtkPolyData::GetScalarFieldCriticalIndex (vtkIdType pointId,
                                              vtkDataArray *scalarField)
{
  /*
   * implements scalar field critical point classification for manifold
   * 2D meshes.
   */

  /*
   * returned value:
   *   -4: no such field
   *   -3: attribute check failed
   *   -2: non 2-manifold star
   *   -1: regular point
   *   0: minimum
   *   1: saddle
   *   2: maximum
   */

  bool  is_min = true, is_max = true;
  vtkIdList *starTriangleList = vtkIdList::New(),
            *lowerLinkPointList = vtkIdList::New(),
            *upperLinkPointList = vtkIdList::New(),
            *pointList = NULL;
  double pointFieldValue = scalarField->GetComponent(pointId, 0),
         neighborFieldValue = 0;

  if(this->GetNumberOfPoints() != scalarField->GetSize())
    return vtkPolyData::ERR_INCORRECT_FIELD;

  /* make sure the connectivity is built */
  if(!this->Links) this->BuildLinks();

  /* build the lower and upper links */
  this->GetPointCells(pointId, starTriangleList);
  int starNb = starTriangleList->GetNumberOfIds();
  for(int i = 0; i < starNb; i++)
  {
    vtkCell *c = this->GetCell(starTriangleList->GetId(i));
    pointList = c->GetPointIds();
    int pointNb = pointList->GetNumberOfIds();
    if(pointNb != 3)
    {
      starTriangleList->Delete();
      lowerLinkPointList->Delete();
      upperLinkPointList->Delete();
      return vtkPolyData::ERR_NON_MANIFOLD_STAR;
    }

    for(int j = 0; j < pointNb; j++)
    {
      vtkIdType  currentPointId = pointList->GetId(j);

      /* quick check for extrema */
      neighborFieldValue = scalarField->GetComponent(currentPointId, 0);
      if((currentPointId != pointId)&&(neighborFieldValue == pointFieldValue))
      {
        /* simulation of simplicity (Edelsbrunner et al. ACM ToG 1990) */
        if(currentPointId > pointId)
        {
          is_max = false;
          upperLinkPointList->InsertUniqueId(currentPointId);
        }
        if(currentPointId < pointId)
        {
          is_min = false;
          lowerLinkPointList->InsertUniqueId(currentPointId);
        }
      }
      else
      {
        if(neighborFieldValue > pointFieldValue)
        {
          is_max = false;
          upperLinkPointList->InsertUniqueId(currentPointId);
        }
        if(neighborFieldValue < pointFieldValue)
        {
          is_min = false;
          lowerLinkPointList->InsertUniqueId(currentPointId);
        }
      }
    }
  }

  if((is_max)||(is_min))
  {
    starTriangleList->Delete();
    lowerLinkPointList->Delete();
    upperLinkPointList->Delete();
    if(is_max) return vtkPolyData::MAXIMUM;
    if(is_min) return vtkPolyData::MINIMUM;
  }

  /*
   * is the vertex really regular?
   * (lower and upper links are BOTH simply connected)
   */
  int visitedPointNb = 0, stackBottom = 0,
      lowerLinkPointNb = lowerLinkPointList->GetNumberOfIds(),
      upperLinkPointNb = upperLinkPointList->GetNumberOfIds();

  /* first, check lower link's simply connectedness */
  vtkIdList *stack = vtkIdList::New();
  stack->InsertUniqueId(lowerLinkPointList->GetId(0));
  vtkIdType currentPointId = stack->GetId(stackBottom), nextPointId = -1;
  do
  {
    stackBottom++;
    vtkIdList *triangleList = vtkIdList::New();
    this->GetPointCells(currentPointId, triangleList);
    int triangleNb = triangleList->GetNumberOfIds();

    for(int i = 0; i < triangleNb; i++)
    {
      vtkCell *c = this->GetCell(triangleList->GetId(i));
      pointList = c->GetPointIds();;
      int pointNb = pointList->GetNumberOfIds();

      if(pointList->IsId(pointId) >= 0)
      {
        // those two triangles are in the star of pointId
        int j = 0;
        do
        {
          nextPointId = pointList->GetId(j);
          j++;
        }while(((nextPointId == pointId)
          ||(nextPointId == currentPointId))&&(j < pointNb ));
      }

      if(lowerLinkPointList->IsId(nextPointId) >= 0)
      {
        stack->InsertUniqueId(nextPointId);
      }

    }

    triangleList->Delete();
    visitedPointNb++;

    currentPointId = stack->GetId(stackBottom);
    nextPointId = -1;

  }while(stackBottom < stack->GetNumberOfIds());

  if(visitedPointNb != lowerLinkPointNb)
  {
    // the lower link is not simply connected, then it's a saddle
    stack->Delete();
    starTriangleList->Delete();
    lowerLinkPointList->Delete();
    upperLinkPointList->Delete();
    return vtkPolyData::SADDLE;
  }

  /*
   * then, check upper link's simply connectedness.
   * BOTH need to be checked if the 2-manifold has boundary components.
   */
  stackBottom = 0;
  visitedPointNb = 0;
  stack->Delete();
  stack = vtkIdList::New();
  stack->InsertUniqueId(upperLinkPointList->GetId(0));
  currentPointId = stack->GetId(stackBottom);
  nextPointId = -1;
  do
  {
    stackBottom++;
    vtkIdList *triangleList = vtkIdList::New();
    this->GetPointCells(currentPointId, triangleList);
    int triangleNb = triangleList->GetNumberOfIds();

    for(int i = 0; i < triangleNb; i++)
    {
      vtkCell *c = this->GetCell(triangleList->GetId(i));
      pointList = c->GetPointIds();
      int pointNb = pointList->GetNumberOfIds();

      if(pointList->IsId(pointId) >= 0)
      {
        // those two triangles are in the star of pointId
        int j = 0;
        do
        {
          nextPointId = pointList->GetId(j);
          j++;
        }while(((nextPointId == pointId)
          ||(nextPointId == currentPointId))&&(j < pointNb));
      }

      if(upperLinkPointList->IsId(nextPointId) >= 0)
      {
        stack->InsertUniqueId(nextPointId);
      }
    }

    triangleList->Delete();
    visitedPointNb++;

    currentPointId = stack->GetId(stackBottom);
    nextPointId = -1;
  }while(stackBottom < stack->GetNumberOfIds());

  if(visitedPointNb != upperLinkPointNb)
  {
    // the upper link is not simply connected, then it's a saddle
    stack->Delete();
    starTriangleList->Delete();
    lowerLinkPointList->Delete();
    upperLinkPointList->Delete();
    return vtkPolyData::SADDLE;
  }

  /* else it's necessarily a regular point (only 4 cases in 2D)*/
  stack->Delete();
  starTriangleList->Delete();
  lowerLinkPointList->Delete();
  upperLinkPointList->Delete();
  return vtkPolyData::REGULAR_POINT;
}

//----------------------------------------------------------------------------
int vtkPolyData::GetScalarFieldCriticalIndex (vtkIdType pointId,
                                              const char* fieldName)
{
  /*
   * returned value:
   *   -4: no such field
   *   -3: attribute check failed
   *   -2: non 2-manifold star
   *   -1: regular point
   *   0: minimum
   *   1: saddle
   *   2: maximum
   */

  int fieldId = 0;

  vtkPointData *pointData = this->GetPointData();
  vtkDataArray *scalarField = pointData->GetArray(fieldName, fieldId);

  if(!scalarField) return vtkPolyData::ERR_NO_SUCH_FIELD;

  return this->GetScalarFieldCriticalIndex(pointId, scalarField);

}

//----------------------------------------------------------------------------
int vtkPolyData::GetScalarFieldCriticalIndex (vtkIdType pointId, int fieldId)
{
  /*
   * returned value:
   *   -4: no such field
   *   -3: attribute check failed
   *   -2: non 2-manifold star
   *   -1: regular point
   *   0: minimum
   *   1: saddle
   *   2: maximum
   */

  vtkPointData *pointData = this->GetPointData();
  vtkDataArray *scalarField = pointData->GetArray(fieldId);

  if(!scalarField) return vtkPolyData::ERR_NO_SUCH_FIELD;

  return this->GetScalarFieldCriticalIndex(pointId, scalarField);
}
