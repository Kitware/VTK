// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyData.h"

#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLinks.h"
#include "vtkUnsignedCharArray.h"

#include <stdexcept>

// vtkPolyDataInternals.h methods:
namespace vtkPolyData_detail
{
VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(CellMap);

CellMap::CellMap() = default;
CellMap::~CellMap() = default;

VTK_ABI_NAMESPACE_END
} // end namespace vtkPolyData_detail

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyData);
vtkStandardExtendedNewMacro(vtkPolyData);

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
vtkPolyData::vtkPolyData()
{
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
  vtkMath::UninitializeBounds(this->CellsBounds);
}

//------------------------------------------------------------------------------
vtkPolyData::~vtkPolyData() = default;

//------------------------------------------------------------------------------
int vtkPolyData::GetPiece()
{
  return this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
}

//------------------------------------------------------------------------------
int vtkPolyData::GetNumberOfPieces()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
}

//------------------------------------------------------------------------------
int vtkPolyData::GetGhostLevel()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
}

//------------------------------------------------------------------------------
// Copy the geometric and topological structure of an input poly data object.
void vtkPolyData::CopyStructure(vtkDataSet* ds)
{
  vtkPolyData* pd = vtkPolyData::SafeDownCast(ds);
  if (!pd)
  {
    vtkErrorMacro("Input dataset is not a " << this->GetClassName());
    return;
  }
  this->Superclass::CopyStructure(ds);

  this->Verts = pd->Verts;
  this->Lines = pd->Lines;
  this->Polys = pd->Polys;
  this->Strips = pd->Strips;
  this->Cells = pd->Cells;
}

//------------------------------------------------------------------------------
vtkIdType vtkPolyData::GetCellIdRelativeToCellArray(vtkIdType cellId)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }
  return this->Cells->GetTag(cellId).GetCellId();
}

//------------------------------------------------------------------------------
vtkCell* vtkPolyData::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->GenericCell);
  return this->GenericCell->GetRepresentativeCell();
}

//------------------------------------------------------------------------------
void vtkPolyData::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  switch (tag.GetCellType())
  {
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
    case VTK_LINE:
    case VTK_POLY_LINE:
    case VTK_TRIANGLE:
    case VTK_QUAD:
    case VTK_POLYGON:
    case VTK_TRIANGLE_STRIP:
      cell->SetCellType(tag.GetCellType());
      break;

    default:
      cell->SetCellTypeToEmptyCell();
      return;
  }

  auto cells = this->GetCellArrayInternal(tag);
  assert(cells != nullptr);
  cells->GetCellAtId(tag.GetCellId(), cell->PointIds);
  this->Points->GetPoints(cell->PointIds, cell->Points);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Support GetCellBounds()
namespace
{ // anonymous
struct ComputeCellBoundsVisitor
{
  // vtkCellArray::Visit entry point:
  template <typename CellStateT>
  void operator()(CellStateT& state, vtkPoints* points, vtkIdType cellId, double bounds[6]) const
  {
    const vtkIdType beginOffset = state.GetBeginOffset(cellId);
    const vtkIdType endOffset = state.GetEndOffset(cellId);
    const vtkIdType numPts = endOffset - beginOffset;

    const auto pointIds = state.GetConnectivity()->GetPointer(beginOffset);
    vtkBoundingBox::ComputeBounds(points, pointIds, numPts, bounds);
  }
};
} // anonymous

//------------------------------------------------------------------------------
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

  vtkCellArray* cells = this->GetCellArrayInternal(tag);
  const vtkIdType localCellId = tag.GetCellId();
  cells->Visit(ComputeCellBoundsVisitor{}, this->Points, localCellId, bounds);
}

//------------------------------------------------------------------------------
// This method only considers points that are used by one or more cells. Thus
// unused points make no contribution to the bounding box computation. This
// is more costly to compute than using just the points, but for rendering
// and historical reasons, produces preferred results.
void vtkPolyData::ComputeCellsBounds()
{
  if (this->GetMeshMTime() > this->CellsBoundsTime)
  {
    // If there are no cells, uninitialize the bounds.
    const vtkIdType numPts = this->GetNumberOfPoints();
    const vtkIdType numPDCells = this->GetNumberOfCells();
    if (numPDCells <= 0)
    {
      vtkMath::UninitializeBounds(this->CellsBounds);
      return;
    }

    // We are going to compute the bounds
    this->CellsBoundsTime.Modified();

    // Make sure this vtkPolyData has points.
    if (this->Points == nullptr || numPts <= 0 || numPDCells <= 0)
    {
      vtkMath::UninitializeBounds(this->CellsBounds);
      return;
    }

    // With cells available, loop over the cells of the polydata.
    // Mark points that are used by one or more cells. Unmarked
    // points do not contribute.
    vtkCellArray* cellArrays[4] = { this->GetVerts(), this->GetLines(), this->GetPolys(),
      this->GetStrips() };

    // Process each cell array separately. Note that threading is only used
    // if the model is big enough (since there is a cost to spinning up the
    // thread pool).

    // Create uses array initialized to 0
    vtkSMPThreadLocalObject<vtkIdList> tlCellPointIds;
    if (numPDCells > vtkSMPTools::THRESHOLD)
    {
      // Create uses array initialized to 0 and supporting threaded access
      std::atomic<unsigned char>* ptUses = new std::atomic<unsigned char>[numPts]();
      for (const auto& cellArray : cellArrays)
      {
        const auto numCells = cellArray->GetNumberOfCells();
        if (numCells <= 0)
        {
          continue;
        }
        // Lambda to threaded mark used points
        vtkSMPTools::For(0, numCells,
          [&](vtkIdType beginCellId, vtkIdType endCellId)
          {
            auto cellPointIds = tlCellPointIds.Local();
            vtkIdType npts, ptIdx;
            const vtkIdType* pts;
            for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
            {
              cellArray->GetCellAtId(cellId, npts, pts, cellPointIds);
              for (ptIdx = 0; ptIdx < npts; ++ptIdx)
              {
                // memory_order_relaxed is safe here, since we're not using the atomics for
                // synchronization.
                ptUses[pts[ptIdx]].store(1, std::memory_order_relaxed);
              }
            }
          }); // end lambda
      }
      vtkBoundingBox::ComputeBounds(this->Points, ptUses, this->CellsBounds);
      delete[] ptUses;
    }
    else
    {
      // Create point uses array initialized to 0
      unsigned char* ptUses = new unsigned char[numPts]();
      for (const auto& cellArray : cellArrays)
      {
        const auto numCells = cellArray->GetNumberOfCells();
        if (numCells <= 0)
        {
          continue;
        }
        auto cellPointIds = tlCellPointIds.Local();
        vtkIdType npts, ptIdx;
        const vtkIdType* pts;
        for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
        {
          cellArray->GetCellAtId(cellId, npts, pts, cellPointIds);
          for (ptIdx = 0; ptIdx < npts; ++ptIdx)
          {
            ptUses[pts[ptIdx]] = 1;
          }
        }
      } // for all cell arrays
      vtkBoundingBox::ComputeBounds(this->Points, ptUses, this->CellsBounds);
      delete[] ptUses;
    } // serial
  }   // if modified mesh
}

//------------------------------------------------------------------------------
void vtkPolyData::GetCellsBounds(double bounds[6])
{
  this->ComputeCellsBounds();
  for (int i = 0; i < 6; i++)
  {
    bounds[i] = this->CellsBounds[i];
  }
}

//------------------------------------------------------------------------------
// Set the cell array defining vertices.
void vtkPolyData::SetVerts(vtkCellArray* v)
{
  if (v == vtkPolyData::DummyContainer.Dummy)
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

//------------------------------------------------------------------------------
// Get the cell array defining vertices. If there are no vertices, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetVerts()
{
  if (!this->Verts)
  {
    return vtkPolyData::DummyContainer.Dummy;
  }
  else
  {
    return this->Verts;
  }
}

//------------------------------------------------------------------------------
// Set the cell array defining lines.
void vtkPolyData::SetLines(vtkCellArray* l)
{
  if (l == vtkPolyData::DummyContainer.Dummy)
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

//------------------------------------------------------------------------------
// Get the cell array defining lines. If there are no lines, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetLines()
{
  if (!this->Lines)
  {
    return vtkPolyData::DummyContainer.Dummy;
  }
  else
  {
    return this->Lines;
  }
}

//------------------------------------------------------------------------------
// Set the cell array defining polygons.
void vtkPolyData::SetPolys(vtkCellArray* p)
{
  if (p == vtkPolyData::DummyContainer.Dummy)
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

//------------------------------------------------------------------------------
// Get the cell array defining polygons. If there are no polygons, an
// empty array will be returned (convenience to simplify traversal).
vtkCellArray* vtkPolyData::GetPolys()
{
  if (!this->Polys)
  {
    return vtkPolyData::DummyContainer.Dummy;
  }
  else
  {
    return this->Polys;
  }
}

//------------------------------------------------------------------------------
// Set the cell array defining triangle strips.
void vtkPolyData::SetStrips(vtkCellArray* s)
{
  if (s == vtkPolyData::DummyContainer.Dummy)
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

//------------------------------------------------------------------------------
// Get the cell array defining triangle strips. If there are no
// triangle strips, an empty array will be returned (convenience to
// simplify traversal).
vtkCellArray* vtkPolyData::GetStrips()
{
  if (!this->Strips)
  {
    return vtkPolyData::DummyContainer.Dummy;
  }
  else
  {
    return this->Strips;
  }
}

//------------------------------------------------------------------------------
void vtkPolyData::Cleanup()
{
  this->Verts = nullptr;
  this->Lines = nullptr;
  this->Polys = nullptr;
  this->Strips = nullptr;

  this->Cells = nullptr;
  this->Links = nullptr;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkPolyData::GetMaxSpatialDimension()
{
  int maxDim = 0;

  if (this->Verts && this->Verts->GetNumberOfCells() > 0)
  {
    maxDim = std::max(maxDim, 0);
  }

  if (this->Lines && this->Lines->GetNumberOfCells() > 0)
  {
    maxDim = std::max(maxDim, 1);
  }

  if (this->Polys && this->Polys->GetNumberOfCells() > 0)
  {
    maxDim = std::max(maxDim, 2);
  }

  if (this->Strips && this->Strips->GetNumberOfCells() > 0)
  {
    maxDim = std::max(maxDim, 2);
  }
  return maxDim;
}

//------------------------------------------------------------------------------
int vtkPolyData::GetMinSpatialDimension()
{
  int minDim = 3;

  if (this->Verts && this->Verts->GetNumberOfCells() > 0)
  {
    minDim = std::min(minDim, 0);
  }

  if (this->Lines && this->Lines->GetNumberOfCells() > 0)
  {
    minDim = std::min(minDim, 1);
  }

  if (this->Polys && this->Polys->GetNumberOfCells() > 0)
  {
    minDim = std::min(minDim, 2);
  }

  if (this->Strips && this->Strips->GetNumberOfCells() > 0)
  {
    minDim = std::min(minDim, 2);
  }
  return minDim;
}

//------------------------------------------------------------------------------
bool vtkPolyData::AllocateEstimate(vtkIdType numCells, vtkIdType maxCellSize)
{
  return this->AllocateExact(numCells, numCells * maxCellSize);
}

//------------------------------------------------------------------------------
bool vtkPolyData::AllocateEstimate(vtkIdType numVerts, vtkIdType maxVertSize, vtkIdType numLines,
  vtkIdType maxLineSize, vtkIdType numPolys, vtkIdType maxPolySize, vtkIdType numStrips,
  vtkIdType maxStripSize)
{
  return this->AllocateExact(numVerts, maxVertSize * numVerts, numLines, maxLineSize * numLines,
    numPolys, maxPolySize * numPolys, numStrips, maxStripSize * numStrips);
}

//------------------------------------------------------------------------------
bool vtkPolyData::AllocateExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  return this->AllocateExact(numCells, connectivitySize, numCells, connectivitySize, numCells,
    connectivitySize, numCells, connectivitySize);
}

//------------------------------------------------------------------------------
bool vtkPolyData::AllocateExact(vtkIdType numVerts, vtkIdType vertConnSize, vtkIdType numLines,
  vtkIdType lineConnSize, vtkIdType numPolys, vtkIdType polyConnSize, vtkIdType numStrips,
  vtkIdType stripConnSize)
{
  auto initCellArray = [](vtkSmartPointer<vtkCellArray>& cellArray, vtkIdType numCells,
                         vtkIdType connSize) -> bool
  {
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

//------------------------------------------------------------------------------
bool vtkPolyData::AllocateCopy(vtkPolyData* pd)
{
  return this->AllocateProportional(pd, 1.);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPolyData::DeleteCells()
{
  // if we have Links, we need to delete them (they are no longer valid)
  this->Links = nullptr;
  this->Cells = nullptr;
}

VTK_ABI_NAMESPACE_END

namespace
{

struct BuildCellsImpl
{
  // Typer functor must take a vtkIdType cell size and convert it into a
  // VTKCellType. The functor must ensure that the input size and returned cell
  // type are valid for the target cell array or throw a std::runtime_error.
  template <typename CellStateT, typename SizeToTypeFunctor>
  void operator()(CellStateT& state, vtkPolyData_detail::CellMap* map, vtkIdType beginCellId,
    SizeToTypeFunctor&& typer)
  {
    const vtkIdType numCells = state.GetNumberOfCells();
    if (numCells == 0)
    {
      return;
    }

    if (!vtkPolyData_detail::CellMap::ValidateCellId(numCells - 1))
    {
      throw std::runtime_error("Cell map storage capacity exceeded.");
    }

    auto buildCellsOperator = [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin, globalCellId = beginCellId + begin; cellId < end;
           ++cellId, ++globalCellId)
      {
        map->InsertCell(globalCellId, cellId, typer(state.GetCellSize(cellId)));
      }
    };
    // We use Threshold to test if the data size is small enough
    // to execute the functor serially. This is faster.
    // and also potentially avoids nested multithreading which creates race conditions.
    vtkSMPTools::For(0, numCells, vtkSMPTools::THRESHOLD, buildCellsOperator);
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
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
  this->Cells->SetNumberOfCells(nCells);

  vtkIdType beginCellId = 0;
  if (nVerts > 0)
  {
    verts->Visit(BuildCellsImpl{}, this->Cells, beginCellId,
      [](vtkIdType size) -> VTKCellType { return size == 1 ? VTK_VERTEX : VTK_POLY_VERTEX; });
    beginCellId += nVerts;
  }

  if (nLines > 0)
  {
    lines->Visit(BuildCellsImpl{}, this->Cells, beginCellId,
      [](vtkIdType size) -> VTKCellType { return size == 2 ? VTK_LINE : VTK_POLY_LINE; });
    beginCellId += nLines;
  }

  if (nPolys > 0)
  {
    polys->Visit(BuildCellsImpl{}, this->Cells, beginCellId,
      [](vtkIdType size) -> VTKCellType
      {
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
    beginCellId += nPolys;
  }

  if (nStrips > 0)
  {
    strips->Visit(BuildCellsImpl{}, this->Cells, beginCellId,
      [](vtkIdType vtkNotUsed(size)) -> VTKCellType { return VTK_TRIANGLE_STRIP; });
  }
}
//------------------------------------------------------------------------------
void vtkPolyData::DeleteLinks()
{
  this->Links = nullptr;
}

//------------------------------------------------------------------------------
// Create upward links from points to cells that use each point. Enables
// topologically complex queries.
void vtkPolyData::BuildLinks(int initialSize)
{
  if (this->Cells == nullptr)
  {
    this->BuildCells();
  }
  if (!this->Points)
  {
    return;
  }
  if (!this->Links)
  {
    if (!this->Editable)
    {
      this->Links = vtkSmartPointer<vtkStaticCellLinks>::New();
    }
    else
    {
      this->Links = vtkSmartPointer<vtkCellLinks>::New();
      if (initialSize > 0)
      {
        static_cast<vtkCellLinks*>(this->Links.Get())->Allocate(initialSize);
      }
    }
    this->Links->SetDataSet(this);
  }
  else if (initialSize > 0 && this->Links->IsA("vtkCellLinks"))
  {
    static_cast<vtkCellLinks*>(this->Links.Get())->Allocate(initialSize);
    this->Links->SetDataSet(this);
  }
  else if (this->Points->GetMTime() > this->Links->GetMTime())
  {
    this->Links->SetDataSet(this);
  }
  this->Links->BuildLinks();
}

//------------------------------------------------------------------------------
// Copy a cells point ids into list provided. (Less efficient.)
void vtkPolyData::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  if (this->Cells == nullptr)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  if (tag.IsDeleted())
  {
    ptIds->SetNumberOfIds(0);
  }
  else
  {
    auto cells = this->GetCellArrayInternal(tag);
    cells->GetCellAtId(tag.GetCellId(), ptIds);
  }
}

//------------------------------------------------------------------------------
void vtkPolyData::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  if (!this->Links)
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  vtkIdType numCells, *cells;
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }

  cellIds->SetNumberOfIds(numCells);
  for (auto i = 0; i < numCells; i++)
  {
    cellIds->SetId(i, cells[i]);
  }
}

//------------------------------------------------------------------------------
void vtkPolyData::GetPointCells(vtkIdType ptId, vtkIdType& ncells, vtkIdType*& cells)
{
  if (!this->Editable)
  {
    auto links = static_cast<vtkStaticCellLinks*>(this->Links.Get());
    ncells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    auto links = static_cast<vtkCellLinks*>(this->Links.Get());
    ncells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
// VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
// the PolyData::Allocate() function has been called first or that vertex,
// line, polygon, and triangle strip arrays have been supplied.
// Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
vtkIdType vtkPolyData::InsertNextCell(int type, vtkIdList* pts)
{
  return this->InsertNextCell(type, static_cast<int>(pts->GetNumberOfIds()), pts->GetPointer(0));
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Add a point to the cell data structure (after cell pointers have been
// built). This method allocates memory for the links to the cells.  (To
// use this method, make sure points are available and BuildLinks() has been invoked.)
vtkIdType vtkPolyData::InsertNextLinkedPoint(int numLinks)
{
  return static_cast<vtkCellLinks*>(this->Links.Get())->InsertNextPoint(numLinks);
}

//------------------------------------------------------------------------------
// Add a point to the cell data structure (after cell pointers have been
// built). This method adds the point and then allocates memory for the
// links to the cells.  (To use this method, make sure points are available
// and BuildLinks() has been invoked.)
vtkIdType vtkPolyData::InsertNextLinkedPoint(double x[3], int numLinks)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->InsertNextPoint(numLinks);
  return this->Points->InsertNextPoint(x);
}

//------------------------------------------------------------------------------
// Add a new cell to the cell data structure (after cell pointers have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.)
vtkIdType vtkPolyData::InsertNextLinkedCell(int type, int npts, const vtkIdType pts[])
{
  vtkIdType i, id;

  id = this->InsertNextCell(type, npts, pts);

  vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());
  for (i = 0; i < npts; i++)
  {
    links->ResizeCellList(pts[i], 1);
    links->AddCellReference(id, pts[i]);
  }

  return id;
}

//------------------------------------------------------------------------------
// Remove a reference to a cell in a particular point's link list. You may also
// consider using RemoveCellReference() to remove the references from all the
// cell's points to the cell. This operator does not reallocate memory; use the
// operator ResizeCellList() to do this if necessary.
void vtkPolyData::RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->RemoveCellReference(cellId, ptId);
}

//------------------------------------------------------------------------------
// Add a reference to a cell in a particular point's link list. (You may also
// consider using AddCellReference() to add the references from all the
// cell's points to the cell.) This operator does not realloc memory; use the
// operator ResizeCellList() to do this if necessary.
void vtkPolyData::AddReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->AddCellReference(cellId, ptId);
}

//------------------------------------------------------------------------------
void vtkPolyData::ReplaceCell(vtkIdType cellId, vtkIdList* ids)
{
  this->ReplaceCell(cellId, static_cast<int>(ids->GetNumberOfIds()), ids->GetPointer(0));
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Replace one cell with another in cell structure. This operator updates the
// connectivity list and the point's link list. It does not delete references
// to the old cell in the point's link list. Use the operator
// RemoveCellReference() to delete all references from points to (old) cell.
// You may also want to consider using the operator ResizeCellList() if the
// link list is changing size.
void vtkPolyData::ReplaceLinkedCell(vtkIdType cellId, int npts, const vtkIdType pts[])
{
  this->ReplaceCell(cellId, npts, pts);
  auto links = static_cast<vtkCellLinks*>(this->Links.Get());
  for (int i = 0; i < npts; i++)
  {
    links->InsertNextCellReference(pts[i], cellId);
  }
}

namespace
{
// Identify the neighbors to the specified cell, where the neighbors
// use all the points in the points list (pts).
template <class TLinks>
inline void GetCellEdgeNeighborsImpl(
  TLinks* links, vtkIdType cellId, vtkIdType p1, vtkIdType p2, vtkIdList* cellIds)
{
  const vtkIdType nCells1 = links->GetNcells(p1);
  const vtkIdType* cells1 = links->GetCells(p1);
  const vtkIdType nCells2 = links->GetNcells(p2);
  const vtkIdType* cells2 = links->GetCells(p2);

  for (vtkIdType i = 0; i < nCells1; ++i)
  {
    if (cells1[i] != cellId)
    {
      for (vtkIdType j = 0; j < nCells2; ++j)
      {
        if (cells1[i] == cells2[j])
        {
          // For degenerate cells, the same cells are linked several times to the degenerate
          // point. So InsertUniqueId is used to prevent duplicates. This is not impacting
          // performances compared to InsertNextId, because most of the time, cellIds is empty.
          cellIds->InsertUniqueId(cells1[i]);
          break;
        }
      }
    }
  }
}
} // end anonymous namespace

//------------------------------------------------------------------------------
// Get the neighbors at an edge. More efficient than the general GetCellNeighbors()
// and looks specifically for edge neighbors.
void vtkPolyData::GetCellEdgeNeighbors(
  vtkIdType cellId, vtkIdType p1, vtkIdType p2, vtkIdList* cellIds)
{
  cellIds->Reset();
  if (!this->Links)
  {
    this->BuildLinks();
  }

  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());
    GetCellEdgeNeighborsImpl<vtkStaticCellLinks>(links, cellId, p1, p2, cellIds);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());
    GetCellEdgeNeighborsImpl<vtkCellLinks>(links, cellId, p1, p2, cellIds);
  }
}

namespace
{
// Identify the neighbors to the specified cell, where the neighbors
// use all the points in the points list (pts).
template <class TLinks>
inline void GetCellNeighborsImpl(
  TLinks* links, vtkIdType cellId, vtkIdType numPts, const vtkIdType* pts, vtkIdList* cellIds)
{
  const vtkIdType nCells0 = links->GetNcells(pts[0]);
  const vtkIdType* cells0 = links->GetCells(pts[0]);

  // for each potential cell
  for (vtkIdType j = 0; j < nCells0; ++j)
  {
    // ignore the original cell
    if (cells0[j] != cellId)
    {
      // are all the remaining points in the cell ?
      bool match = true;
      for (vtkIdType i = 1; i < numPts && match; i++)
      {
        const vtkIdType nCellsI = links->GetNcells(pts[i]);
        const vtkIdType* cellsI = links->GetCells(pts[i]);

        match = false;
        for (vtkIdType k = 0; k < nCellsI; k++)
        {
          if (cells0[j] == cellsI[k])
          {
            match = true;
            break;
          }
        }
      }
      if (match)
      {
        // For degenerate cells, the same cells are linked several times to the degenerate
        // point. So InsertUniqueId is used to prevent duplicates. This is not impacting
        // performances compared to InsertNextId, because most of the time, cellIds is empty.
        cellIds->InsertUniqueId(cells0[j]);
      }
    }
  }
}
} // end anonymous namespace

//------------------------------------------------------------------------------
void vtkPolyData::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  cellIds->Reset();
  if (!this->Links)
  {
    this->BuildLinks();
  }

  // Get the cell links based on the current state.
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());
    GetCellNeighborsImpl<vtkStaticCellLinks>(
      links, cellId, ptIds->GetNumberOfIds(), ptIds->GetPointer(0), cellIds);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());
    GetCellNeighborsImpl<vtkCellLinks>(
      links, cellId, ptIds->GetNumberOfIds(), ptIds->GetPointer(0), cellIds);
  }
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkPolyData::IsTriangle(int v1, int v2, int v3)
{
  vtkIdType n1;
  int i, j, tVerts[3];
  vtkIdType* cells;
  const vtkIdType* tVerts2;
  vtkIdType n2;

  tVerts[0] = v1;
  tVerts[1] = v2;
  tVerts[2] = v3;

  for (i = 0; i < 3; i++)
  {
    this->GetPointCells(tVerts[i], n1, cells);
    for (j = 0; j < n1; j++)
    {
      this->GetCellPoints(cells[j], n2, tVerts2);
      if ((tVerts[0] == tVerts2[0] || tVerts[0] == tVerts2[1] || tVerts[0] == tVerts2[2]) &&
        (tVerts[1] == tVerts2[0] || tVerts[1] == tVerts2[1] || tVerts[1] == tVerts2[2]) &&
        (tVerts[2] == tVerts2[0] || tVerts[2] == tVerts2[1] || tVerts[2] == tVerts2[2]))
      {
        return 1;
      }
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkPolyData::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Links, "Links");
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPolyData::ShallowCopy(vtkDataObject* dataObject)
{
  vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataObject);
  if (this == polyData)
  {
    return;
  }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
  if (polyData)
  {
    this->SetVerts(polyData->GetVerts());
    this->SetLines(polyData->GetLines());
    this->SetPolys(polyData->GetPolys());
    this->SetStrips(polyData->GetStrips());

    // I do not know if this is correct but.
    // Me either! But it's been 20 years so I think it'll be ok.
    this->Cells = polyData->Cells;

    if (polyData->Links)
    {
      this->Links = vtkSmartPointer<vtkAbstractCellLinks>::Take(polyData->Links->NewInstance());
      this->Links->SetDataSet(this);
      this->Links->ShallowCopy(polyData->Links);
    }
    else
    {
      this->Links = nullptr;
    }
  }
}

//------------------------------------------------------------------------------
void vtkPolyData::DeepCopy(vtkDataObject* dataObject)
{
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());

  vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataObject);
  // Do superclass We have to do this BEFORE we call BuildLinks, else
  // there are no points to build the links on (the parent DeepCopy
  // copies the points)
  this->Superclass::DeepCopy(dataObject);

  if (polyData)
  {
    if (polyData->Verts)
    {
      this->Verts = vtkSmartPointer<vtkCellArray>::New();
      this->Verts->DeepCopy(polyData->Verts);
    }
    else
    {
      this->Verts = nullptr;
    }
    if (polyData->Lines)
    {
      this->Lines = vtkSmartPointer<vtkCellArray>::New();
      this->Lines->DeepCopy(polyData->Lines);
    }
    else
    {
      this->Lines = nullptr;
    }
    if (polyData->Polys)
    {
      this->Polys = vtkSmartPointer<vtkCellArray>::New();
      this->Polys->DeepCopy(polyData->Polys);
    }
    else
    {
      this->Polys = nullptr;
    }
    if (polyData->Strips)
    {
      this->Strips = vtkSmartPointer<vtkCellArray>::New();
      this->Strips->DeepCopy(polyData->Strips);
    }
    else
    {
      this->Strips = nullptr;
    }
    if (polyData->Cells)
    {
      this->Cells = vtkSmartPointer<CellMap>::New();
      this->Cells->DeepCopy(polyData->Cells);
    }
    else
    {
      this->Cells = nullptr;
    }
    if (polyData->Links)
    {
      this->Links = vtkSmartPointer<vtkAbstractCellLinks>::Take(polyData->Links->NewInstance());
      this->Links->SetDataSet(this);
      this->Links->DeepCopy(polyData->Links);
    }
    else
    {
      this->Links = nullptr;
    }

    this->CellsBoundsTime = polyData->CellsBoundsTime;
    for (int idx = 0; idx < 3; ++idx)
    {
      this->CellsBounds[2 * idx] = polyData->CellsBounds[2 * idx];
      this->CellsBounds[2 * idx + 1] = polyData->CellsBounds[2 * idx + 1];
    }
  }
}

static constexpr unsigned char MASKED_CELL_VALUE = vtkDataSetAttributes::HIDDENCELL |
  vtkDataSetAttributes::DUPLICATECELL | vtkDataSetAttributes::REFINEDCELL;

//------------------------------------------------------------------------------
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
  vtkIdType numCells = this->GetNumberOfCells();
  vtkIdType numPoints = this->GetNumberOfPoints();

  if (!numCells || !numPoints)
  {
    return;
  }

  // check if there are any cells to delete
  if (!this->CellData->HasAnyGhostBitSet(MASKED_CELL_VALUE))
  {
    return;
  }

  unsigned char* cellGhosts = temp->GetPointer(0);

  vtkNew<vtkPolyData> newPD;
  vtkNew<vtkCellArray> newVerts, newLines, newPolys, newStrips;
  vtkPointData* newPointData = newPD->GetPointData();
  vtkCellData* newCellData = newPD->GetCellData();

  if (this->Verts)
  {
    this->Verts->IsStorage64Bit() ? newVerts->Use64BitStorage() : newVerts->Use32BitStorage();
  }
  if (this->Lines)
  {
    this->Lines->IsStorage64Bit() ? newLines->Use64BitStorage() : newLines->Use32BitStorage();
  }
  if (this->Polys)
  {
    this->Polys->IsStorage64Bit() ? newPolys->Use64BitStorage() : newPolys->Use32BitStorage();
  }
  if (this->Strips)
  {
    this->Strips->IsStorage64Bit() ? newStrips->Use64BitStorage() : newStrips->Use32BitStorage();
  }

  newVerts->Allocate(this->GetNumberOfVerts());
  newLines->Allocate(this->GetNumberOfLines());
  newPolys->Allocate(this->GetNumberOfPolys());
  newStrips->Allocate(this->GetNumberOfStrips());

  newCellData->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  newCellData->CopyAllocate(this->CellData, numCells);

  newPointData->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  newPointData->CopyAllocate(this->PointData, numCells);

  vtkNew<vtkPoints> newPoints;
  newPoints->SetDataType(this->GetPoints()->GetDataType());
  newPoints->Allocate(numPoints);

  vtkNew<vtkIdList> pointMap;
  pointMap->SetNumberOfIds(numPoints);
  vtkSMPTools::Fill(pointMap->begin(), pointMap->end(), -1);

  const vtkIdType* pts;
  double* x;
  vtkIdType n;

  vtkIdType cellId;
  vtkNew<vtkIdList> newCellPoints;

  newPD->SetPoints(newPoints);
  newPD->SetVerts(newVerts);
  newPD->SetLines(newLines);
  newPD->SetPolys(newPolys);
  newPD->SetStrips(newStrips);

  if (this->Verts)
  {
    this->Verts->InitTraversal();
  }
  if (this->Lines)
  {
    this->Lines->InitTraversal();
  }
  if (this->Polys)
  {
    this->Polys->InitTraversal();
  }
  if (this->Strips)
  {
    this->Strips->InitTraversal();
  }

  for (vtkIdType i = 0; i < numCells; i++)
  {
    int type = this->GetCellType(i);

    if (type == VTK_VERTEX || type == VTK_POLY_VERTEX)
    {
      this->Verts->GetNextCell(n, pts);

      if (!(cellGhosts[i] & MASKED_CELL_VALUE))
      {
        for (vtkIdType id = 0; id < n; ++id)
        {
          vtkIdType ptId = pts[id];
          vtkIdType newId;
          if ((newId = pointMap->GetId(ptId)) == -1)
          {
            x = this->GetPoint(ptId);
            newId = newPoints->InsertNextPoint(x);
            pointMap->SetId(ptId, newId);
            newPointData->CopyData(this->PointData, ptId, newId);
          }
          newCellPoints->InsertId(id, newId);
        }

        cellId = newPD->InsertNextCell(type, newCellPoints);
        newCellData->CopyData(this->CellData, i, cellId);
        newCellPoints->Reset();
      }
    }
    else if (type == VTK_LINE || type == VTK_POLY_LINE)
    {
      this->Lines->GetNextCell(n, pts);

      if (!(cellGhosts[i] & MASKED_CELL_VALUE))
      {
        for (vtkIdType id = 0; id < n; ++id)
        {
          vtkIdType newId;
          vtkIdType ptId = pts[id];
          if ((newId = pointMap->GetId(ptId)) == -1)
          {
            x = this->GetPoint(ptId);
            newId = newPoints->InsertNextPoint(x);
            pointMap->SetId(ptId, newId);
            newPointData->CopyData(this->PointData, ptId, newId);
          }
          newCellPoints->InsertId(id, newId);
        }

        cellId = newPD->InsertNextCell(type, newCellPoints);
        newCellData->CopyData(this->CellData, i, cellId);
        newCellPoints->Reset();
      }
    }
    else if (type == VTK_POLYGON || type == VTK_TRIANGLE || type == VTK_QUAD)
    {
      this->Polys->GetNextCell(n, pts);

      if (!(cellGhosts[i] & MASKED_CELL_VALUE))
      {
        for (vtkIdType id = 0; id < n; ++id)
        {
          vtkIdType ptId = pts[id];
          vtkIdType newId;
          if ((newId = pointMap->GetId(ptId)) == -1)
          {
            x = this->GetPoint(ptId);
            newId = newPoints->InsertNextPoint(x);
            pointMap->SetId(ptId, newId);
            newPointData->CopyData(this->PointData, ptId, newId);
          }
          newCellPoints->InsertId(id, newId);
        }

        cellId = newPD->InsertNextCell(type, newCellPoints);
        newCellData->CopyData(this->CellData, i, cellId);
        newCellPoints->Reset();
      }
    }
    else if (type == VTK_TRIANGLE_STRIP)
    {
      this->Strips->GetNextCell(n, pts);

      if (!(cellGhosts[i] & MASKED_CELL_VALUE))
      {
        for (vtkIdType id = 0; id < n; ++id)
        {
          vtkIdType ptId = pts[id];
          vtkIdType newId;
          if ((newId = pointMap->GetId(ptId)) == -1)
          {
            x = this->GetPoint(ptId);
            newId = newPoints->InsertNextPoint(x);
            pointMap->SetId(ptId, newId);
            newPointData->CopyData(this->PointData, ptId, newId);
          }
          newCellPoints->InsertId(id, newId);
        }

        cellId = newPD->InsertNextCell(type, newCellPoints);
        newCellData->CopyData(this->CellData, i, cellId);
        newCellPoints->Reset();
      }
    }
  }

  newCellData->Squeeze();
  newPointData->Squeeze();

  newPD->GetFieldData()->ShallowCopy(this->GetFieldData());
  this->ShallowCopy(newPD);

  this->CellData->RemoveArray(vtkDataSetAttributes::GhostArrayName());
  this->PointData->RemoveArray(vtkDataSetAttributes::GhostArrayName());

  this->Squeeze();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
vtkPolyData* vtkPolyData::GetData(vtkInformation* info)
{
  return info ? vtkPolyData::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkPolyData* vtkPolyData::GetData(vtkInformationVector* v, int i)
{
  return vtkPolyData::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
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

  double bounds[6];
  this->GetCellsBounds(bounds);
  os << indent << "CellsBounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
  os << indent << "CellsBounds Time: " << this->CellsBoundsTime.GetMTime() << "\n";
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
  time = vtkMath::Max(this->GetGhostCellsTime(), time);

  return time;
}

//------------------------------------------------------------------------------
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
VTK_ABI_NAMESPACE_END
