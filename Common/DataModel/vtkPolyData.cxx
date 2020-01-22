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

#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
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
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertex.h"

#include <stdexcept>

// vtkPolyDataInternals.h methods:
namespace vtkPolyData_detail
{

vtkStandardNewMacro(CellMap);
CellMap::CellMap() = default;
CellMap::~CellMap() = default;

} // end namespace vtkPolyData_detail

vtkStandardNewMacro(vtkPolyData);

//----------------------------------------------------------------------------
// Initialize static member.  This member is used to simplify traversal
// of verts, lines, polygons, and triangle strips lists.  It basically
// "marks" empty lists so that the traversal method "GetNextCell"
// works properly.

struct vtkPolyDataDummyContainter
{
  vtkSmartPointer<vtkCellArray> Dummy;

  vtkPolyDataDummyContainter() { this->Dummy.TakeReference(vtkCellArray::New()); }
};

vtkPolyDataDummyContainter vtkPolyData::DummyContainer;

//----------------------------------------------------------------------------
unsigned char vtkPolyData::GetCell(vtkIdType cellId, vtkIdType const*& cell)
{
  vtkIdType npts;
  const vtkIdType* pts;
  const auto type = this->GetCellPoints(cellId, npts, pts);

  if (type == VTK_EMPTY_CELL)
  { // Cell is deleted
    cell = nullptr;
  }
  else
  {
    this->LegacyBuffer->SetNumberOfIds(npts + 1);
    this->LegacyBuffer->SetId(0, npts);
    for (vtkIdType i = 0; i < npts; ++i)
    {
      this->LegacyBuffer->SetId(i, pts[i]);
    }

    cell = this->LegacyBuffer->GetPointer(0);
  }

  return type;
}

//----------------------------------------------------------------------------
vtkPolyData::vtkPolyData()
{
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkPolyData::~vtkPolyData() = default;

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
void vtkPolyData::CopyStructure(vtkDataSet* ds)
{
  vtkPolyData* pd = vtkPolyData::SafeDownCast(ds);
  if (!pd)
  {
    vtkErrorMacro("Input dataset is not a polydata!");
    return;
  }

  vtkPointSet::CopyStructure(ds);

  this->Verts = pd->Verts;
  this->Lines = pd->Lines;
  this->Polys = pd->Polys;
  this->Strips = pd->Strips;

  this->Cells = nullptr;
  this->Links = nullptr;
}

//----------------------------------------------------------------------------
vtkCell* vtkPolyData::GetCell(vtkIdType cellId)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);

  vtkIdType numPts;
  const vtkIdType* pts;
  vtkCell* cell = nullptr;
  switch (tag.GetCellType())
  {
    case VTK_VERTEX:
      if (!this->Vertex)
      {
        this->Vertex = vtkSmartPointer<vtkVertex>::New();
      }
      cell = this->Vertex;
      this->Verts->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 1);
      break;

    case VTK_POLY_VERTEX:
      if (!this->PolyVertex)
      {
        this->PolyVertex = vtkSmartPointer<vtkPolyVertex>::New();
      }
      cell = this->PolyVertex;
      this->Verts->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts);
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_LINE:
      if (!this->Line)
      {
        this->Line = vtkSmartPointer<vtkLine>::New();
      }
      cell = this->Line;
      this->Lines->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 2);
      break;

    case VTK_POLY_LINE:
      if (!this->PolyLine)
      {
        this->PolyLine = vtkSmartPointer<vtkPolyLine>::New();
      }
      cell = this->PolyLine;
      this->Lines->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts);
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE:
      if (!this->Triangle)
      {
        this->Triangle = vtkSmartPointer<vtkTriangle>::New();
      }
      cell = this->Triangle;
      this->Polys->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 3);
      break;

    case VTK_QUAD:
      if (!this->Quad)
      {
        this->Quad = vtkSmartPointer<vtkQuad>::New();
      }
      cell = this->Quad;
      this->Polys->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 4);
      break;

    case VTK_POLYGON:
      if (!this->Polygon)
      {
        this->Polygon = vtkSmartPointer<vtkPolygon>::New();
      }
      cell = this->Polygon;
      this->Polys->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts);
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE_STRIP:
      if (!this->TriangleStrip)
      {
        this->TriangleStrip = vtkSmartPointer<vtkTriangleStrip>::New();
      }
      cell = this->TriangleStrip;
      this->Strips->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts);
      cell->Points->SetNumberOfPoints(numPts);
      break;

    default:
      if (!this->EmptyCell)
      {
        this->EmptyCell = vtkSmartPointer<vtkEmptyCell>::New();
      }
      cell = this->EmptyCell;
      return cell;
  }

  for (vtkIdType i = 0; i < numPts; ++i)
  {
    cell->PointIds->SetId(i, pts[i]);
    cell->Points->SetPoint(i, this->Points->GetPoint(pts[i]));
  }

  return cell;
}

//----------------------------------------------------------------------------
void vtkPolyData::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);

  vtkIdType numPts;
  const vtkIdType* pts;
  switch (tag.GetCellType())
  {
    case VTK_VERTEX:
      cell->SetCellTypeToVertex();
      this->Verts->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 1);
      break;

    case VTK_POLY_VERTEX:
      cell->SetCellTypeToPolyVertex();
      this->Verts->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts); // reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_LINE:
      cell->SetCellTypeToLine();
      this->Lines->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 2);
      break;

    case VTK_POLY_LINE:
      cell->SetCellTypeToPolyLine();
      this->Lines->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts); // reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE:
      cell->SetCellTypeToTriangle();
      this->Polys->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 3);
      break;

    case VTK_QUAD:
      cell->SetCellTypeToQuad();
      this->Polys->GetCellAtId(tag.GetCellId(), numPts, pts);
      assert(numPts == 4);
      break;

    case VTK_POLYGON:
      cell->SetCellTypeToPolygon();
      this->Polys->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts); // reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    case VTK_TRIANGLE_STRIP:
      cell->SetCellTypeToTriangleStrip();
      this->Strips->GetCellAtId(tag.GetCellId(), numPts, pts);
      cell->PointIds->SetNumberOfIds(numPts); // reset number of points
      cell->Points->SetNumberOfPoints(numPts);
      break;

    default:
      cell->SetCellTypeToEmptyCell();
      numPts = 0;
      return;
  }

  double x[3];
  for (vtkIdType i = 0; i < numPts; ++i)
  {
    cell->PointIds->SetId(i, pts[i]);
    this->Points->GetPoint(pts[i], x);
    cell->Points->SetPoint(i, x);
  }
}

//----------------------------------------------------------------------------
void vtkPolyData::CopyCells(vtkPolyData* pd, vtkIdList* idList, vtkIncrementalPointLocator* locator)
{
  vtkIdType cellId, ptId, newId, newCellId, locatorPtId;
  vtkIdType numPts, numCellPts, i;
  vtkPoints* newPoints;
  vtkIdList* pointMap = vtkIdList::New(); // maps old pt ids into new
  vtkIdList *cellPts, *newCellPts = vtkIdList::New();
  vtkGenericCell* cell = vtkGenericCell::New();
  double x[3];
  vtkPointData* outPD = this->GetPointData();
  vtkCellData* outCD = this->GetCellData();

  numPts = pd->GetNumberOfPoints();

  if (this->GetPoints() == nullptr)
  {
    this->Points = vtkPoints::New();
  }

  newPoints = this->GetPoints();

  pointMap->SetNumberOfIds(numPts);
  for (i = 0; i < numPts; i++)
  {
    pointMap->SetId(i, -1);
  }

  // Filter the cells
  for (cellId = 0; cellId < idList->GetNumberOfIds(); cellId++)
  {
    pd->GetCell(idList->GetId(cellId), cell);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    for (i = 0; i < numCellPts; i++)
    {
      ptId = cellPts->GetId(i);
      if ((newId = pointMap->GetId(ptId)) < 0)
      {
        pd->GetPoint(ptId, x);
        if (locator != nullptr)
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
      newCellPts->InsertId(i, newId);
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
// constructing a cell. This method is expected to be thread-safe.
void vtkPolyData::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  if (tag.IsDeleted())
  {
    std::fill_n(bounds, 6, 0.);
    return;
  }

  vtkIdType numPts;
  const vtkIdType* pts;
  vtkCellArray* cells = this->GetCellArrayInternal(tag);
  vtkSmartPointer<vtkCellArrayIterator> iter;
  if (cells->IsStorageShareable())
  {
    // much faster and thread-safe if storage is shareable
    cells->GetCellAtId(tag.GetCellId(), numPts, pts);
  }
  else
  {
    // guaranteed thread safe
    iter = vtk::TakeSmartPointer(cells->NewIterator());
    iter->GetCellAtId(tag.GetCellId(), numPts, pts);
  }

  // carefully compute the bounds
  double x[3];
  if (numPts)
  {
    this->Points->GetPoint(pts[0], x);
    bounds[0] = x[0];
    bounds[2] = x[1];
    bounds[4] = x[2];
    bounds[1] = x[0];
    bounds[3] = x[1];
    bounds[5] = x[2];
    for (vtkIdType i = 1; i < numPts; ++i)
    {
      this->Points->GetPoint(pts[i], x);
      bounds[0] = std::min(x[0], bounds[0]);
      bounds[1] = std::max(x[0], bounds[1]);
      bounds[2] = std::min(x[1], bounds[2]);
      bounds[3] = std::max(x[1], bounds[3]);
      bounds[4] = std::min(x[2], bounds[4]);
      bounds[5] = std::max(x[2], bounds[5]);
    }
  }
  else
  {
    vtkMath::UninitializeBounds(bounds);
  }
}

//----------------------------------------------------------------------------
// This method only considers points that are used by one or more cells. Thus
// unused points make no contribution to the bounding box computation. This
// is more costly to compute than using just the points, but for rendering
// and historical reasons, produces preferred results.
void vtkPolyData::ComputeBounds()
{
  if (this->GetMeshMTime() > this->ComputeTime)
  {
    // If there are no cells, but there are points, compute the bounds from the
    // parent class vtkPointSet (which just examines points).
    vtkIdType numPts = this->GetNumberOfPoints();
    vtkIdType numCells = this->GetNumberOfCells();
    if (numCells <= 0 && numPts > 0)
    {
      vtkPointSet::ComputeBounds();
      return;
    }

    // We are going to compute the bounds
    this->ComputeTime.Modified();

    // Make sure this vtkPolyData has points.
    if (this->Points == nullptr || numPts <= 0)
    {
      vtkMath::UninitializeBounds(this->Bounds);
      return;
    }

    // With cells available, loop over the cells of the polydata.
    // Mark points that are used by one or more cells. Unmarked
    // points do not contribute.
    unsigned char* ptUses = new unsigned char[numPts];
    std::fill_n(ptUses, numPts, 0); // initially unvisited

    vtkCellArray* cellA[4];
    cellA[0] = this->GetVerts();
    cellA[1] = this->GetLines();
    cellA[2] = this->GetPolys();
    cellA[3] = this->GetStrips();

    // Process each cell array separately. Note that threading is only used
    // if the model is big enough (since there is a cost to spinning up the
    // thread pool).
    for (auto ca = 0; ca < 4; ca++)
    {
      if ((numCells = cellA[ca]->GetNumberOfCells()) > 250000)
      {
        // Lambda to threaded compute bounds
        vtkSMPTools::For(0, numCells, [&](vtkIdType cellId, vtkIdType endCellId) {
          vtkIdType npts, ptIdx;
          const vtkIdType* pts;
          auto iter = vtk::TakeSmartPointer(cellA[ca]->NewIterator());
          for (; cellId < endCellId; ++cellId)
          {
            iter->GetCellAtId(cellId, npts, pts); // thread-safe
            for (ptIdx = 0; ptIdx < npts; ++ptIdx)
            {
              ptUses[pts[ptIdx]] = 1;
            }
          }
        }); // end lambda
      }
      else if (numCells > 0) // serial
      {
        vtkIdType npts, ptIdx;
        const vtkIdType* pts;
        for (auto cellId = 0; cellId < numCells; ++cellId)
        {
          cellA[ca]->GetCellAtId(cellId, npts, pts);
          for (ptIdx = 0; ptIdx < npts; ++ptIdx)
          {
            ptUses[pts[ptIdx]] = 1;
          }
        }
      }
    } // for all cell arrays

    // Perform the bounding box computation
    vtkBoundingBox::ComputeBounds(this->Points, ptUses, this->Bounds);
    delete[] ptUses;
  }
}

//----------------------------------------------------------------------------
// Set the cell array defining vertices.
void vtkPolyData::SetVerts(vtkCellArray* v)
{
  if (v == this->DummyContainer.Dummy)
  {
    v = nullptr;
  }

  if (v != this->Verts)
  {
    this->Verts = v;

    // Reset the cell table:
    this->Cells = nullptr;

    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Get the cell array defining vertices. If there are no vertices, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetVerts()
{
  if (!this->Verts)
  {
    return this->DummyContainer.Dummy;
  }
  else
  {
    return this->Verts;
  }
}

//----------------------------------------------------------------------------
// Set the cell array defining lines.
void vtkPolyData::SetLines(vtkCellArray* l)
{
  if (l == this->DummyContainer.Dummy)
  {
    l = nullptr;
  }

  if (l != this->Lines)
  {
    this->Lines = l;

    // Reset the cell table:
    this->Cells = nullptr;

    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Get the cell array defining lines. If there are no lines, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetLines()
{
  if (!this->Lines)
  {
    return this->DummyContainer.Dummy;
  }
  else
  {
    return this->Lines;
  }
}

//----------------------------------------------------------------------------
// Set the cell array defining polygons.
void vtkPolyData::SetPolys(vtkCellArray* p)
{
  if (p == this->DummyContainer.Dummy)
  {
    p = nullptr;
  }

  if (p != this->Polys)
  {
    this->Polys = p;

    // Reset the cell table:
    this->Cells = nullptr;

    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Get the cell array defining polygons. If there are no polygons, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetPolys()
{
  if (!this->Polys)
  {
    return this->DummyContainer.Dummy;
  }
  else
  {
    return this->Polys;
  }
}

//----------------------------------------------------------------------------
// Set the cell array defining triangle strips.
void vtkPolyData::SetStrips(vtkCellArray* s)
{
  if (s == this->DummyContainer.Dummy)
  {
    s = nullptr;
  }

  if (s != this->Strips)
  {
    this->Strips = s;

    // Reset the cell table:
    this->Cells = nullptr;

    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Get the cell array defining triangle strips. If there are no
// triangle strips, an empty array will be returned (convenience to
// simplify traversal).
vtkCellArray* vtkPolyData::GetStrips()
{
  if (!this->Strips)
  {
    return this->DummyContainer.Dummy;
  }
  else
  {
    return this->Strips;
  }
}

//----------------------------------------------------------------------------
void vtkPolyData::Cleanup()
{
  this->Vertex = nullptr;
  this->PolyVertex = nullptr;
  this->Line = nullptr;
  this->PolyLine = nullptr;
  this->Triangle = nullptr;
  this->Quad = nullptr;
  this->Polygon = nullptr;
  this->TriangleStrip = nullptr;
  this->EmptyCell = nullptr;

  this->Verts = nullptr;
  this->Lines = nullptr;
  this->Polys = nullptr;
  this->Strips = nullptr;

  this->Cells = nullptr;
  this->Links = nullptr;
}

//----------------------------------------------------------------------------
// Restore object to initial state. Release memory back to system.
void vtkPolyData::Initialize()
{
  vtkPointSet::Initialize();

  this->Cleanup();

  if (this->Information)
  {
    this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 0);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
  }
}

//----------------------------------------------------------------------------
int vtkPolyData::GetMaxCellSize()
{
  int maxCellSize = 0;

  if (this->Verts)
  {
    maxCellSize = std::max(maxCellSize, this->Verts->GetMaxCellSize());
  }

  if (this->Lines)
  {
    maxCellSize = std::max(maxCellSize, this->Lines->GetMaxCellSize());
  }

  if (this->Polys)
  {
    maxCellSize = std::max(maxCellSize, this->Polys->GetMaxCellSize());
  }

  if (this->Strips)
  {
    maxCellSize = std::max(maxCellSize, this->Strips->GetMaxCellSize());
  }

  return maxCellSize;
}

//----------------------------------------------------------------------------
bool vtkPolyData::AllocateEstimate(vtkIdType numCells, vtkIdType maxCellSize)
{
  return this->AllocateExact(numCells, numCells * maxCellSize);
}

//----------------------------------------------------------------------------
bool vtkPolyData::AllocateEstimate(vtkIdType numVerts, vtkIdType maxVertSize, vtkIdType numLines,
  vtkIdType maxLineSize, vtkIdType numPolys, vtkIdType maxPolySize, vtkIdType numStrips,
  vtkIdType maxStripSize)
{
  return this->AllocateExact(numVerts, maxVertSize * numVerts, numLines, maxLineSize * numLines,
    numPolys, maxPolySize * numPolys, numStrips, maxStripSize * numStrips);
}

//----------------------------------------------------------------------------
bool vtkPolyData::AllocateExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  return this->AllocateExact(numCells, connectivitySize, numCells, connectivitySize, numCells,
    connectivitySize, numCells, connectivitySize);
}

//----------------------------------------------------------------------------
bool vtkPolyData::AllocateExact(vtkIdType numVerts, vtkIdType vertConnSize, vtkIdType numLines,
  vtkIdType lineConnSize, vtkIdType numPolys, vtkIdType polyConnSize, vtkIdType numStrips,
  vtkIdType stripConnSize)
{
  auto initCellArray = [](vtkSmartPointer<vtkCellArray>& cellArray, vtkIdType numCells,
                         vtkIdType connSize) -> bool {
    cellArray = nullptr;
    if (numCells == 0 && connSize == 0)
    {
      return true;
    }
    cellArray = vtkSmartPointer<vtkCellArray>::New();
    return cellArray->AllocateExact(numCells, connSize);
  };

  // Reset the cell table:
  this->Cells = nullptr;

  return (initCellArray(this->Verts, numVerts, vertConnSize) &&
    initCellArray(this->Lines, numLines, lineConnSize) &&
    initCellArray(this->Polys, numPolys, polyConnSize) &&
    initCellArray(this->Strips, numStrips, stripConnSize));
}

//----------------------------------------------------------------------------
bool vtkPolyData::AllocateCopy(vtkPolyData* pd)
{
  return this->AllocateProportional(pd, 1.);
}

//----------------------------------------------------------------------------
bool vtkPolyData::AllocateProportional(vtkPolyData* pd, double ratio)
{

  auto* verts = pd->GetVerts();
  auto* lines = pd->GetLines();
  auto* polys = pd->GetPolys();
  auto* strips = pd->GetStrips();

  return this->AllocateExact(static_cast<vtkIdType>(verts->GetNumberOfCells() * ratio),
    static_cast<vtkIdType>(verts->GetNumberOfConnectivityIds() * ratio),
    static_cast<vtkIdType>(lines->GetNumberOfCells() * ratio),
    static_cast<vtkIdType>(lines->GetNumberOfConnectivityIds() * ratio),
    static_cast<vtkIdType>(polys->GetNumberOfCells() * ratio),
    static_cast<vtkIdType>(polys->GetNumberOfConnectivityIds() * ratio),
    static_cast<vtkIdType>(strips->GetNumberOfCells() * ratio),
    static_cast<vtkIdType>(strips->GetNumberOfConnectivityIds() * ratio));
}

//----------------------------------------------------------------------------
void vtkPolyData::DeleteCells()
{
  // if we have Links, we need to delete them (they are no longer valid)
  this->Links = nullptr;
  this->Cells = nullptr;
}

namespace
{

struct BuildCellsImpl
{
  // Typer functor must take a vtkIdType cell size and convert it into a
  // VTKCellType. The functor must ensure that the input size and returned cell
  // type are valid for the target cell array or throw a std::runtime_error.
  template <typename CellStateT, typename SizeToTypeFunctor>
  void operator()(CellStateT& state, vtkPolyData_detail::CellMap* map, SizeToTypeFunctor&& typer)
  {
    const vtkIdType numCells = state.GetNumberOfCells();
    if (numCells == 0)
    {
      return;
    }

    if (!map->ValidateCellId(numCells - 1))
    {
      throw std::runtime_error("Cell map storage capacity exceeded.");
    }

    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      map->InsertNextCell(cellId, typer(state.GetCellSize(cellId)));
    }
  }
};

} // end anon namespace

//----------------------------------------------------------------------------
// Create data structure that allows random access of cells.
void vtkPolyData::BuildCells()
{
  vtkCellArray* verts = this->GetVerts();
  vtkCellArray* lines = this->GetLines();
  vtkCellArray* polys = this->GetPolys();
  vtkCellArray* strips = this->GetStrips();

  // here are the number of cells we have
  const vtkIdType nVerts = verts->GetNumberOfCells();
  const vtkIdType nLines = lines->GetNumberOfCells();
  const vtkIdType nPolys = polys->GetNumberOfCells();
  const vtkIdType nStrips = strips->GetNumberOfCells();

  // pre-allocate the space we need
  const vtkIdType nCells = nVerts + nLines + nPolys + nStrips;

  this->Cells = vtkSmartPointer<CellMap>::New();
  this->Cells->SetCapacity(nCells);

  try
  {
    if (nVerts > 0)
    {
      verts->Visit(BuildCellsImpl{}, this->Cells, [](vtkIdType size) -> VTKCellType {
        if (size < 1)
        {
          throw std::runtime_error("Invalid cell size for verts.");
        }
        return size == 1 ? VTK_VERTEX : VTK_POLY_VERTEX;
      });
    }

    if (nLines > 0)
    {
      lines->Visit(BuildCellsImpl{}, this->Cells, [](vtkIdType size) -> VTKCellType {
        if (size < 2)
        {
          throw std::runtime_error("Invalid cell size for lines.");
        }
        return size == 2 ? VTK_LINE : VTK_POLY_LINE;
      });
    }

    if (nPolys > 0)
    {
      polys->Visit(BuildCellsImpl{}, this->Cells, [](vtkIdType size) -> VTKCellType {
        if (size < 3)
        {
          throw std::runtime_error("Invalid cell size for polys.");
        }

        switch (size)
        {
          case 3:
            return VTK_TRIANGLE;
          case 4:
            return VTK_QUAD;
          default:
            return VTK_POLYGON;
        }
      });
    }

    if (nStrips > 0)
    {
      strips->Visit(BuildCellsImpl{}, this->Cells, [](vtkIdType size) -> VTKCellType {
        if (size < 3)
        {
          throw std::runtime_error("Invalid cell size for polys.");
        }
        return VTK_TRIANGLE_STRIP;
      });
    }
  }
  catch (std::runtime_error& e)
  {
    this->Cells = nullptr;
    vtkErrorMacro("Error while constructing cell map: " << e.what());
  }
}
//----------------------------------------------------------------------------
void vtkPolyData::DeleteLinks()
{
  this->Links = nullptr;
}

//----------------------------------------------------------------------------
// Create upward links from points to cells that use each point. Enables
// topologically complex queries.
void vtkPolyData::BuildLinks(int initialSize)
{
  if (this->Cells == nullptr)
  {
    this->BuildCells();
  }

  this->Links = vtkSmartPointer<vtkCellLinks>::New();
  if (initialSize > 0)
  {
    this->Links->Allocate(initialSize);
  }
  else
  {
    this->Links->Allocate(this->GetNumberOfPoints());
  }

  this->Links->BuildLinks(this);
}

//----------------------------------------------------------------------------
// Copy a cells point ids into list provided. (Less efficient.)
void vtkPolyData::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  if (this->Cells == nullptr)
  {
    this->BuildCells();
  }

  vtkIdType npts;
  const vtkIdType* pts;
  this->GetCellPoints(cellId, npts, pts);

  ptIds->SetNumberOfIds(npts);
  for (vtkIdType i = 0; i < npts; ++i)
  {
    ptIds->SetId(i, pts[i]);
  }
}

//----------------------------------------------------------------------------
void vtkPolyData::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  vtkIdType* cells;
  vtkIdType numCells;
  vtkIdType i;

  if (!this->Links)
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  numCells = this->Links->GetNcells(ptId);
  cells = this->Links->GetCells(ptId);

  for (i = 0; i < numCells; i++)
  {
    cellIds->InsertId(i, cells[i]);
  }
}

//----------------------------------------------------------------------------
// Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
// VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
vtkIdType vtkPolyData::InsertNextCell(int type, int npts, const vtkIdType ptsIn[])
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const vtkIdType* pts = ptsIn;
  vtkIdType pixPts[4];

  // Docs say we need to handle VTK_PIXEL:
  if (type == VTK_PIXEL)
  {
    // need to rearrange vertices
    pixPts[0] = pts[0];
    pixPts[1] = pts[1];
    pixPts[2] = pts[3];
    pixPts[3] = pts[2];

    type = VTK_QUAD;
    pts = pixPts;
  }

  // Make sure the type is supported by the dataset (and thus safe to use with
  // the TaggedCellId):
  if (!CellMap::ValidateCellType(VTKCellType(type)))
  {
    vtkErrorMacro("Invalid cell type: " << type);
    return -1;
  }

  // Insert next cell into the lookup map:
  TaggedCellId& tag = this->Cells->InsertNextCell(VTKCellType(type));
  vtkCellArray* cells = this->GetCellArrayInternal(tag);

  // Validate and update the internal cell id:
  const vtkIdType internalCellId = cells->InsertNextCell(npts, pts);
  if (internalCellId < 0)
  {
    vtkErrorMacro("Internal error: Invalid cell id (" << internalCellId << ").");
    return -1;
  }
  if (!CellMap::ValidateCellId(internalCellId))
  {
    vtkErrorMacro("Internal cell array storage exceeded.");
    return -1;
  }
  tag.SetCellId(internalCellId);

  // Return the dataset cell id:
  return this->Cells->GetNumberOfCells() - 1;
}

//----------------------------------------------------------------------------
// Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
// VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
vtkIdType vtkPolyData::InsertNextCell(int type, vtkIdList* pts)
{
  return this->InsertNextCell(type, static_cast<int>(pts->GetNumberOfIds()), pts->GetPointer(0));
}

//----------------------------------------------------------------------------
// Recover extra allocated memory when creating data whose initial size
// is unknown. Examples include using the InsertNextCell() method, or
// when using the CellArray::EstimateSize() method to create vertices,
// lines, polygons, or triangle strips.
void vtkPolyData::Squeeze()
{
  if (this->Verts != nullptr)
  {
    this->Verts->Squeeze();
  }
  if (this->Lines != nullptr)
  {
    this->Lines->Squeeze();
  }
  if (this->Polys != nullptr)
  {
    this->Polys->Squeeze();
  }
  if (this->Strips != nullptr)
  {
    this->Strips->Squeeze();
  }
  if (this->Cells != nullptr)
  {
    this->Cells->Squeeze();
  }

  vtkPointSet::Squeeze();
}

//----------------------------------------------------------------------------
// Begin inserting data all over again. Memory is not freed but otherwise
// objects are returned to their initial state.
void vtkPolyData::Reset()
{
  if (this->Verts != nullptr)
  {
    this->Verts->Reset();
  }
  if (this->Lines != nullptr)
  {
    this->Lines->Reset();
  }
  if (this->Polys != nullptr)
  {
    this->Polys->Reset();
  }
  if (this->Strips != nullptr)
  {
    this->Strips->Reset();
  }

  if (this->GetPoints() != nullptr)
  {
    this->GetPoints()->Reset();
  }

  // discard Links and Cells
  this->DeleteLinks();
  this->DeleteCells();
}

//----------------------------------------------------------------------------
// Reverse the order of point ids defining the cell.
void vtkPolyData::ReverseCell(vtkIdType cellId)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  vtkCellArray* cells = this->GetCellArrayInternal(tag);
  cells->ReverseCellAtId(tag.GetCellId());
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
vtkIdType vtkPolyData::InsertNextLinkedCell(int type, int npts, const vtkIdType pts[])
{
  vtkIdType i, id;

  id = this->InsertNextCell(type, npts, pts);

  for (i = 0; i < npts; i++)
  {
    this->Links->ResizeCellList(pts[i], 1);
    this->Links->AddCellReference(id, pts[i]);
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
void vtkPolyData::ReplaceCell(vtkIdType cellId, vtkIdList* ids)
{
  this->ReplaceCell(cellId, static_cast<int>(ids->GetNumberOfIds()), ids->GetPointer(0));
}

//----------------------------------------------------------------------------
// Replace the points defining cell "cellId" with a new set of points. This
// operator is (typically) used when links from points to cells have not been
// built (i.e., BuildLinks() has not been executed). Use the operator
// ReplaceLinkedCell() to replace a cell when cell structure has been built.
void vtkPolyData::ReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[])
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  vtkCellArray* cells = this->GetCellArrayInternal(tag);
  cells->ReplaceCellAtId(tag.GetCellId(), npts, pts);
}

//----------------------------------------------------------------------------
// Replace one cell with another in cell structure. This operator updates the
// connectivity list and the point's link list. It does not delete references
// to the old cell in the point's link list. Use the operator
// RemoveCellReference() to delete all references from points to (old) cell.
// You may also want to consider using the operator ResizeCellList() if the
// link list is changing size.
void vtkPolyData::ReplaceLinkedCell(vtkIdType cellId, int npts, const vtkIdType pts[])
{
  this->ReplaceCell(cellId, npts, pts);
  for (int i = 0; i < npts; i++)
  {
    this->Links->InsertNextCellReference(pts[i], cellId);
  }
}

//----------------------------------------------------------------------------
// Get the neighbors at an edge. More efficient than the general
// GetCellNeighbors(). Assumes links have been built (with BuildLinks()),
// and looks specifically for edge neighbors.
void vtkPolyData::GetCellEdgeNeighbors(
  vtkIdType cellId, vtkIdType p1, vtkIdType p2, vtkIdList* cellIds)
{
  cellIds->Reset();

  const vtkCellLinks::Link& link1(this->Links->GetLink(p1));
  const vtkCellLinks::Link& link2(this->Links->GetLink(p2));

  const vtkIdType* cells1 = link1.cells;
  const vtkIdType* cells1End = cells1 + link1.ncells;

  const vtkIdType* cells2 = link2.cells;
  const vtkIdType* cells2End = cells2 + link2.ncells;

  while (cells1 != cells1End)
  {
    if (*cells1 != cellId)
    {
      const vtkIdType* cells2Cur(cells2);
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
void vtkPolyData::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  vtkIdType i, j, numPts, cellNum;
  int allFound, oneFound;

  if (!this->Links)
  {
    this->BuildLinks();
  }

  cellIds->Reset();

  // load list with candidate cells, remove current cell
  vtkIdType ptId = ptIds->GetId(0);
  int numPrime = this->Links->GetNcells(ptId);
  vtkIdType* primeCells = this->Links->GetCells(ptId);
  numPts = ptIds->GetNumberOfIds();

  // for each potential cell
  for (cellNum = 0; cellNum < numPrime; cellNum++)
  {
    // ignore the original cell
    if (primeCells[cellNum] != cellId)
    {
      // are all the remaining points in the cell ?
      for (allFound = 1, i = 1; i < numPts && allFound; i++)
      {
        ptId = ptIds->GetId(i);
        int numCurrent = this->Links->GetNcells(ptId);
        vtkIdType* currentCells = this->Links->GetCells(ptId);
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

//----------------------------------------------------------------------------
int vtkPolyData::IsEdge(vtkIdType p1, vtkIdType p2)
{
  vtkIdType ncells;
  vtkIdType cellType;
  vtkIdType npts;
  vtkIdType i, j;
  vtkIdType* cells;
  const vtkIdType* pts;

  vtkIdType nbPoints = this->GetNumberOfPoints();
  if (p1 >= nbPoints || p2 >= nbPoints)
  {
    return 0;
  }

  this->GetPointCells(p1, ncells, cells);
  for (i = 0; i < ncells; i++)
  {
    cellType = this->GetCellType(cells[i]);
    switch (cellType)
    {
      case VTK_EMPTY_CELL:
      case VTK_VERTEX:
      case VTK_POLY_VERTEX:
      case VTK_LINE:
      case VTK_POLY_LINE:
        break;
      case VTK_TRIANGLE:
        if (this->IsPointUsedByCell(p2, cells[i]))
        {
          return 1;
        }
        break;
      case VTK_QUAD:
        this->GetCellPoints(cells[i], npts, pts);
        for (j = 0; j < npts - 1; j++)
        {
          if (((pts[j] == p1) && (pts[j + 1] == p2)) || ((pts[j] == p2) && (pts[j + 1] == p1)))
          {
            return 1;
          }
        }
        if (((pts[0] == p1) && (pts[npts - 1] == p2)) || ((pts[0] == p2) && (pts[npts - 1] == p1)))
        {
          return 1;
        }
        break;
      case VTK_TRIANGLE_STRIP:
        this->GetCellPoints(cells[i], npts, pts);
        for (j = 0; j < npts - 2; j++)
        {
          if ((((pts[j] == p1) && (pts[j + 1] == p2)) || ((pts[j] == p2) && (pts[j + 1] == p1))) ||
            (((pts[j] == p1) && (pts[j + 2] == p2)) || ((pts[j] == p2) && (pts[j + 2] == p1))))
          {
            return 1;
          }
        }
        if (((pts[npts - 2] == p1) && (pts[npts - 1] == p2)) ||
          ((pts[npts - 2] == p2) && (pts[npts - 1] == p1)))
        {
          return 1;
        }
        break;
      default:
        this->GetCellPoints(cells[i], npts, pts);
        for (j = 0; j < npts; j++)
        {
          if (p1 == pts[j])
          {
            if ((pts[(j - 1 + npts) % npts] == p2) || (pts[(j + 1) % npts] == p2))
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
  unsigned long size = this->vtkPointSet::GetActualMemorySize();
  if (this->Verts)
  {
    size += this->Verts->GetActualMemorySize();
  }
  if (this->Lines)
  {
    size += this->Lines->GetActualMemorySize();
  }
  if (this->Polys)
  {
    size += this->Polys->GetActualMemorySize();
  }
  if (this->Strips)
  {
    size += this->Strips->GetActualMemorySize();
  }
  if (this->Cells)
  {
    size += this->Cells->GetActualMemorySize();
  }
  if (this->Links)
  {
    size += this->Links->GetActualMemorySize();
  }
  return size;
}

//----------------------------------------------------------------------------
void vtkPolyData::ShallowCopy(vtkDataObject* dataObject)
{
  vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataObject);
  if (this == polyData)
    return;

  if (polyData != nullptr)
  {
    this->SetVerts(polyData->GetVerts());
    this->SetLines(polyData->GetLines());
    this->SetPolys(polyData->GetPolys());
    this->SetStrips(polyData->GetStrips());

    // I do not know if this is correct but.
    // Me either! But it's been 20 years so I think it'll be ok.
    this->Cells = polyData->Cells;
    this->Links = polyData->Links;
  }

  // Do superclass
  this->vtkPointSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkPolyData::DeepCopy(vtkDataObject* dataObject)
{
  // Do superclass
  // We have to do this BEFORE we call BuildLinks, else there are no points
  // to build the links on (the parent DeepCopy copies the points)
  this->vtkPointSet::DeepCopy(dataObject);

  vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataObject);

  if (polyData != nullptr)
  {
    this->Verts = vtkSmartPointer<vtkCellArray>::New();
    this->Verts->DeepCopy(polyData->GetVerts());

    this->Lines = vtkSmartPointer<vtkCellArray>::New();
    this->Lines->DeepCopy(polyData->GetLines());

    this->Polys = vtkSmartPointer<vtkCellArray>::New();
    this->Polys->DeepCopy(polyData->GetPolys());

    this->Strips = vtkSmartPointer<vtkCellArray>::New();
    this->Strips->DeepCopy(polyData->GetStrips());

    // only instantiate this if the input dataset has one
    if (polyData->Cells)
    {
      this->Cells = vtkSmartPointer<CellMap>::New();
      this->Cells->DeepCopy(polyData->Cells);
    }
    else
    {
      this->Cells = nullptr;
    }

    if (this->Links)
    {
      this->Links = nullptr;
    }
    if (polyData->Links)
    {
      this->BuildLinks();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPolyData::RemoveGhostCells()
{
  // Get a pointer to the cell ghost level array.
  vtkUnsignedCharArray* temp = this->GetCellGhostArray();
  if (temp == nullptr)
  {
    vtkDebugMacro("Could not find cell ghost array.");
    return;
  }
  if (temp->GetNumberOfComponents() != 1 || temp->GetNumberOfTuples() < this->GetNumberOfCells())
  {
    vtkErrorMacro("Poorly formed ghost array.");
    return;
  }
  unsigned char* cellGhosts = temp->GetPointer(0);

  vtkIdType numCells = this->GetNumberOfCells();

  vtkIntArray* types = vtkIntArray::New();
  types->SetNumberOfValues(numCells);

  for (vtkIdType i = 0; i < numCells; i++)
  {
    types->SetValue(i, this->GetCellType(i));
  }

  this->DeleteCells();

  // we have to make new copies of Verts, Lines, Polys
  // and Strips since they may be shared with other polydata
  vtkSmartPointer<vtkCellArray> verts;
  if (this->Verts)
  {
    verts = this->Verts;
    verts->InitTraversal();
    this->Verts = vtkSmartPointer<vtkCellArray>::New();
  }

  vtkSmartPointer<vtkCellArray> lines;
  if (this->Lines)
  {
    lines = this->Lines;
    lines->InitTraversal();
    this->Lines = vtkSmartPointer<vtkCellArray>::New();
  }

  vtkSmartPointer<vtkCellArray> polys;
  if (this->Polys)
  {
    polys = this->Polys;
    polys->InitTraversal();
    this->Polys = vtkSmartPointer<vtkCellArray>::New();
  }

  vtkSmartPointer<vtkCellArray> strips;
  if (this->Strips)
  {
    strips = this->Strips;
    strips->InitTraversal();
    this->Strips = vtkSmartPointer<vtkCellArray>::New();
  }

  vtkCellData* newCellData = vtkCellData::New();
  // ensure that all attributes are copied over, including global ids.
  newCellData->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  newCellData->CopyAllocate(this->CellData, numCells);

  const vtkIdType* pts;
  vtkIdType n;

  vtkIdType cellId;

  for (vtkIdType i = 0; i < numCells; i++)
  {
    int type = types->GetValue(i);

    if (type == VTK_VERTEX || type == VTK_POLY_VERTEX)
    {
      verts->GetNextCell(n, pts);

      if (!(cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
    else if (type == VTK_LINE || type == VTK_POLY_LINE)
    {
      lines->GetNextCell(n, pts);

      if (!(cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
    else if (type == VTK_POLYGON || type == VTK_TRIANGLE || type == VTK_QUAD)
    {
      polys->GetNextCell(n, pts);

      if (!(cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
      {
        cellId = this->InsertNextCell(type, n, pts);
        newCellData->CopyData(this->CellData, i, cellId);
      }
    }
    else if (type == VTK_TRIANGLE_STRIP)
    {
      strips->GetNextCell(n, pts);

      if (!(cellGhosts[i] & vtkDataSetAttributes::DUPLICATECELL))
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

  if (this->Verts)
  {
    this->Verts = vtkSmartPointer<vtkCellArray>::New();
  }
  if (this->Lines)
  {
    this->Lines = vtkSmartPointer<vtkCellArray>::New();
  }
  if (this->Polys)
  {
    this->Polys = vtkSmartPointer<vtkCellArray>::New();
  }
  if (this->Strips)
  {
    this->Strips = vtkSmartPointer<vtkCellArray>::New();
  }

  this->CellData->CopyAllocate(oldData->GetCellData());

  const vtkIdType numCells = oldData->GetNumberOfCells();
  vtkCell* cell;
  vtkIdType cellId;
  vtkIdList* pointIds;
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
  return info ? vtkPolyData::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyData::GetData(vtkInformationVector* v, int i)
{
  return vtkPolyData::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Vertices: " << this->GetNumberOfVerts() << "\n";
  os << indent << "Number Of Lines: " << this->GetNumberOfLines() << "\n";
  os << indent << "Number Of Polygons: " << this->GetNumberOfPolys() << "\n";
  os << indent << "Number Of Triangle Strips: " << this->GetNumberOfStrips() << "\n";

  os << indent << "Number Of Pieces: " << this->GetNumberOfPieces() << endl;
  os << indent << "Piece: " << this->GetPiece() << endl;
  os << indent << "Ghost Level: " << this->GetGhostLevel() << endl;
}

//----------------------------------------------------------------------------
int vtkPolyData::GetScalarFieldCriticalIndex(vtkIdType pointId, vtkDataArray* scalarField)
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

  bool is_min = true, is_max = true;
  vtkIdList *starTriangleList = vtkIdList::New(), *lowerLinkPointList = vtkIdList::New(),
            *upperLinkPointList = vtkIdList::New(), *pointList = nullptr;
  double pointFieldValue = scalarField->GetComponent(pointId, 0);

  if (this->GetNumberOfPoints() != scalarField->GetSize())
  {
    return vtkPolyData::ERR_INCORRECT_FIELD;
  }

  /* make sure the connectivity is built */
  if (!this->Links)
  {
    this->BuildLinks();
  }

  /* build the lower and upper links */
  this->GetPointCells(pointId, starTriangleList);
  int starNb = starTriangleList->GetNumberOfIds();
  for (int i = 0; i < starNb; i++)
  {
    vtkCell* c = this->GetCell(starTriangleList->GetId(i));
    pointList = c->GetPointIds();
    int pointNb = pointList->GetNumberOfIds();
    if (pointNb != 3)
    {
      starTriangleList->Delete();
      lowerLinkPointList->Delete();
      upperLinkPointList->Delete();
      return vtkPolyData::ERR_NON_MANIFOLD_STAR;
    }

    for (int j = 0; j < pointNb; j++)
    {
      vtkIdType currentPointId = pointList->GetId(j);

      /* quick check for extrema */
      double neighborFieldValue = scalarField->GetComponent(currentPointId, 0);
      if ((currentPointId != pointId) && (neighborFieldValue == pointFieldValue))
      {
        /* simulation of simplicity (Edelsbrunner et al. ACM ToG 1990) */
        if (currentPointId > pointId)
        {
          is_max = false;
          upperLinkPointList->InsertUniqueId(currentPointId);
        }
        if (currentPointId < pointId)
        {
          is_min = false;
          lowerLinkPointList->InsertUniqueId(currentPointId);
        }
      }
      else
      {
        if (neighborFieldValue > pointFieldValue)
        {
          is_max = false;
          upperLinkPointList->InsertUniqueId(currentPointId);
        }
        if (neighborFieldValue < pointFieldValue)
        {
          is_min = false;
          lowerLinkPointList->InsertUniqueId(currentPointId);
        }
      }
    }
  }

  if ((is_max) || (is_min))
  {
    starTriangleList->Delete();
    lowerLinkPointList->Delete();
    upperLinkPointList->Delete();
    if (is_max)
      return vtkPolyData::MAXIMUM;
    if (is_min)
      return vtkPolyData::MINIMUM;
  }

  /*
   * is the vertex really regular?
   * (lower and upper links are BOTH simply connected)
   */
  int visitedPointNb = 0, stackBottom = 0, lowerLinkPointNb = lowerLinkPointList->GetNumberOfIds(),
      upperLinkPointNb = upperLinkPointList->GetNumberOfIds();

  /* first, check lower link's simply connectedness */
  vtkIdList* stack = vtkIdList::New();
  stack->InsertUniqueId(lowerLinkPointList->GetId(0));
  do
  {
    vtkIdType currentPointId = stack->GetId(stackBottom);
    vtkIdType nextPointId = -1;

    stackBottom++;
    vtkIdList* triangleList = vtkIdList::New();
    this->GetPointCells(currentPointId, triangleList);
    int triangleNb = triangleList->GetNumberOfIds();

    for (int i = 0; i < triangleNb; i++)
    {
      vtkCell* c = this->GetCell(triangleList->GetId(i));
      pointList = c->GetPointIds();
      int pointNb = pointList->GetNumberOfIds();

      if (pointList->IsId(pointId) >= 0)
      {
        // those two triangles are in the star of pointId
        int j = 0;
        do
        {
          nextPointId = pointList->GetId(j);
          j++;
        } while (((nextPointId == pointId) || (nextPointId == currentPointId)) && (j < pointNb));
      }

      if (lowerLinkPointList->IsId(nextPointId) >= 0)
      {
        stack->InsertUniqueId(nextPointId);
      }
    }

    triangleList->Delete();
    visitedPointNb++;
  } while (stackBottom < stack->GetNumberOfIds());

  if (visitedPointNb != lowerLinkPointNb)
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
  do
  {
    vtkIdType currentPointId = stack->GetId(stackBottom);
    vtkIdType nextPointId = -1;
    stackBottom++;
    vtkIdList* triangleList = vtkIdList::New();
    this->GetPointCells(currentPointId, triangleList);
    int triangleNb = triangleList->GetNumberOfIds();

    for (int i = 0; i < triangleNb; i++)
    {
      vtkCell* c = this->GetCell(triangleList->GetId(i));
      pointList = c->GetPointIds();
      int pointNb = pointList->GetNumberOfIds();

      if (pointList->IsId(pointId) >= 0)
      {
        // those two triangles are in the star of pointId
        int j = 0;
        do
        {
          nextPointId = pointList->GetId(j);
          j++;
        } while (((nextPointId == pointId) || (nextPointId == currentPointId)) && (j < pointNb));
      }

      if (upperLinkPointList->IsId(nextPointId) >= 0)
      {
        stack->InsertUniqueId(nextPointId);
      }
    }

    triangleList->Delete();
    visitedPointNb++;
  } while (stackBottom < stack->GetNumberOfIds());

  if (visitedPointNb != upperLinkPointNb)
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
int vtkPolyData::GetScalarFieldCriticalIndex(vtkIdType pointId, const char* fieldName)
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

  vtkPointData* pointData = this->GetPointData();
  vtkDataArray* scalarField = pointData->GetArray(fieldName, fieldId);

  if (!scalarField)
    return vtkPolyData::ERR_NO_SUCH_FIELD;

  return this->GetScalarFieldCriticalIndex(pointId, scalarField);
}

//----------------------------------------------------------------------------
int vtkPolyData::GetScalarFieldCriticalIndex(vtkIdType pointId, int fieldId)
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

  vtkPointData* pointData = this->GetPointData();
  vtkDataArray* scalarField = pointData->GetArray(fieldId);

  if (!scalarField)
    return vtkPolyData::ERR_NO_SUCH_FIELD;

  return this->GetScalarFieldCriticalIndex(pointId, scalarField);
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPolyData::GetMeshMTime()
{
  vtkMTimeType time = this->Points ? this->Points->GetMTime() : 0;
  if (this->Verts)
  {
    time = vtkMath::Max(this->Verts->GetMTime(), time);
  }
  if (this->Lines)
  {
    time = vtkMath::Max(this->Lines->GetMTime(), time);
  }
  if (this->Polys)
  {
    time = vtkMath::Max(this->Polys->GetMTime(), time);
  }
  if (this->Strips)
  {
    time = vtkMath::Max(this->Strips->GetMTime(), time);
  }
  return time;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPolyData::GetMTime()
{
  vtkMTimeType time = this->Superclass::GetMTime();
  if (this->Verts)
  {
    time = vtkMath::Max(this->Verts->GetMTime(), time);
  }
  if (this->Lines)
  {
    time = vtkMath::Max(this->Lines->GetMTime(), time);
  }
  if (this->Polys)
  {
    time = vtkMath::Max(this->Polys->GetMTime(), time);
  }
  if (this->Strips)
  {
    time = vtkMath::Max(this->Strips->GetMTime(), time);
  }
  return time;
}
