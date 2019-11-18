/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitStructuredGrid.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkDataSetAttributes.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStaticCellLinks.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

static const unsigned char MASKED_CELL_VALUE =
  vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;

static int HEXAHEDRON_POINT_MAP[] = {
  0, 1, 3, 2, 4, 5, 7, 6, //
  1, 0, 2, 3, 5, 4, 6, 7, //
  0, 3, 1, 2, 4, 7, 5, 6, //
  3, 0, 2, 1, 7, 4, 6, 5, //
  0, 4, 1, 5, 2, 6, 3, 7, //
  4, 0, 5, 1, 6, 2, 7, 3  //
};

static int SWAP_HEXAHEDRON_POINT_MAP[] = {
  0, 1, 5, 4, 3, 2, 6, 7, //
  0, 4, 7, 3, 1, 5, 6, 2, //
  0, 3, 2, 1, 4, 7, 6, 5  //
};

static int MIRROR_HEXAHEDRON_POINT_MAP[] = {
  1, 0, 3, 2, 5, 4, 7, 6, //
  3, 2, 1, 0, 7, 6, 5, 4, //
  4, 5, 6, 7, 0, 1, 2, 3  //
};

vtkStandardNewMacro(vtkExplicitStructuredGrid);
vtkSetObjectImplementationMacro(vtkExplicitStructuredGrid, Cells, vtkCellArray);

#define vtkAdjustBoundsMacro(A, B)                                                                 \
  A[0] = (B[0] < A[0] ? B[0] : A[0]);                                                              \
  A[1] = (B[0] > A[1] ? B[0] : A[1]);                                                              \
  A[2] = (B[1] < A[2] ? B[1] : A[2]);                                                              \
  A[3] = (B[1] > A[3] ? B[1] : A[3]);                                                              \
  A[4] = (B[2] < A[4] ? B[2] : A[4]);                                                              \
  A[5] = (B[2] > A[5] ? B[2] : A[5])

//----------------------------------------------------------------------------
vtkExplicitStructuredGrid::vtkExplicitStructuredGrid()
{
  this->Cells = nullptr;
  this->Links = nullptr;

  this->FacesConnectivityFlagsArrayName = nullptr;

  int extent[6] = { 0, -1, 0, -1, 0, -1 };
  std::copy(this->Extent, this->Extent + 6, extent);

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//----------------------------------------------------------------------------
vtkExplicitStructuredGrid::~vtkExplicitStructuredGrid()
{
  this->SetFacesConnectivityFlagsArrayName(nullptr);
  this->SetCells(nullptr);
  if (this->Links)
  {
    this->Links->Delete();
    this->Links = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::Initialize()
{
  this->Superclass::Initialize();

  if (this->Information)
  {
    this->SetDimensions(0, 0, 0);
  }

  this->SetCells(nullptr);
  if (this->Links)
  {
    this->Links->Delete();
    this->Links = nullptr;
  }
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGrid::GetCellType(vtkIdType cellId)
{
  return this->IsCellVisible(cellId) ? VTK_HEXAHEDRON : VTK_EMPTY_CELL;
}

//----------------------------------------------------------------------------
vtkIdType vtkExplicitStructuredGrid::GetNumberOfCells()
{
  return vtkStructuredData::GetNumberOfCells(this->Extent);
}

//----------------------------------------------------------------------------
vtkCell* vtkExplicitStructuredGrid::GetCell(vtkIdType cellId)
{
  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    return this->EmptyCell;
  }
  this->GetCell(cellId, this->Hexahedron);
  return this->Hexahedron;
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  if (!this->IsCellVisible(cellId))
  {
    cell->SetCellTypeToEmptyCell();
    return;
  }
  cell->SetCellTypeToHexahedron();
  this->GetCell(cellId, static_cast<vtkCell*>(cell));
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::GetCell(vtkIdType cellId, vtkCell* cell)
{
  // Make sure data is defined
  if (!this->Points || !this->Cells)
  {
    vtkErrorMacro(<< "No geometry or topology found!");
    return;
  }

  // See whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    return;
  }

  // Extract point coordinates and point ids. NOTE: the ordering of the
  // vtkHexahedron cells are tricky.
  vtkIdType* indices = this->GetCellPoints(cellId);
  for (int i = 0; i < 8; i++)
  {
    vtkIdType idx = indices[i];
    double x[3];
    this->Points->GetPoint(idx, x);
    cell->Points->SetPoint(i, x);
    cell->PointIds->SetId(i, idx);
  }
}

//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().
// Bounds are calculated without constructing a cell.
void vtkExplicitStructuredGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  if (!this->Points)
  {
    vtkErrorMacro(<< "No data");
    return;
  }

  vtkIdType* indices = this->GetCellPoints(cellId);
  double x[3];

  this->Points->GetPoint(indices[0], x);
  bounds[0] = bounds[1] = x[0];
  bounds[2] = bounds[3] = x[1];
  bounds[4] = bounds[5] = x[2];

  this->Points->GetPoint(indices[1], x);
  vtkAdjustBoundsMacro(bounds, x);

  this->Points->GetPoint(indices[2], x);
  vtkAdjustBoundsMacro(bounds, x);

  this->Points->GetPoint(indices[3], x);
  vtkAdjustBoundsMacro(bounds, x);

  this->Points->GetPoint(indices[4], x);
  vtkAdjustBoundsMacro(bounds, x);

  this->Points->GetPoint(indices[5], x);
  vtkAdjustBoundsMacro(bounds, x);

  this->Points->GetPoint(indices[6], x);
  vtkAdjustBoundsMacro(bounds, x);

  this->Points->GetPoint(indices[7], x);
  vtkAdjustBoundsMacro(bounds, x);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  if (!this->Links)
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  // Use the correct cell links. Use an explicit cast for performance reasons
  // (virtuals, and templated functions were tested to be slow -- if you make
  // changes, please make sure to measure performance impacts).
  vtkIdType numCells, *cells;
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links);
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links);
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }

  cellIds->SetNumberOfIds(numCells);
  for (auto i = 0; i < numCells; i++)
  {
    cellIds->SetId(i, cells[i]);
  }
}

//----------------------------------------------------------------------------
vtkIdType* vtkExplicitStructuredGrid::GetCellPoints(vtkIdType cellId)
{
  vtkIdType unused;
  const vtkIdType* result;
  this->Cells->GetCellAtId(cellId, unused, result);
  return const_cast<vtkIdType*>(result);
}

//----------------------------------------------------------------------------
// Get the points defining a cell. (See vtkDataSet for more info.)
void vtkExplicitStructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  ptIds->Reset();
  ptIds->SetNumberOfIds(8);
  vtkIdType* indices = this->GetCellPoints(cellId);
  for (int i = 0; i < 8; i++)
  {
    ptIds->SetId(i, indices[i]);
  }
}

//----------------------------------------------------------------------------
// Return a pointer to a list of point ids defining cell.
// More efficient than alternative method.
void vtkExplicitStructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType*& pts)
{
  npts = 8;
  pts = this->GetCellPoints(cellId);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::GetCellNeighbors(
  vtkIdType cellId, vtkIdType neighbors[6], int* wholeExtent)
{
  int ci, cj, ck;
  this->ComputeCellStructuredCoords(cellId, ci, cj, ck, true);
  int* extent = wholeExtent;
  if (!wholeExtent)
  {
    // If the whole extent have not been defined, use own extent
    extent = new int[6];
    this->GetExtent(extent);
  }
  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent(extent, dims);
  dims[0]--;
  dims[1]--;
  dims[2]--;
  for (int faceId = 0; faceId < 6; faceId++)
  {
    int c[] = { ci - extent[0], cj - extent[2], ck - extent[4] };
    c[faceId / 2] += (faceId % 2) ? 1 : -1;
    bool invalidCellId =
      (c[0] < 0 || c[1] < 0 || c[2] < 0 || c[0] >= dims[0] || c[1] >= dims[1] || c[2] >= dims[2]);
    neighbors[faceId] = invalidCellId ? -1 : (c[0] + c[1] * dims[0] + c[2] * dims[0] * dims[1]);
  }
  if (!wholeExtent)
  {
    delete[] extent;
  }
}

//----------------------------------------------------------------------------
// Determine neighbors as follows. Find the (shortest) list of cells that
// uses one of the points in ptIds. For each cell, in the list, see whether
// it contains the other points in the ptIds list. If so, it's a neighbor.
void vtkExplicitStructuredGrid::GetCellNeighbors(
  vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  if (!this->Links)
  {
    this->BuildLinks();
  }

  cellIds->Reset();

  vtkIdType* minCells = nullptr;
  vtkIdType minPtId = 0;

  // Find the point used by the fewest number of cells

  vtkIdType numPts = ptIds->GetNumberOfIds();
  vtkIdType* pts = ptIds->GetPointer(0);
  vtkIdType minNumCells = VTK_INT_MAX;
  for (int i = 0; i < numPts; i++)
  {
    vtkIdType ptId = pts[i];
    vtkIdType numCells = 0;
    vtkIdType* cells = nullptr;
    //    vtkIdType numCells = this->Links->GetNcells(ptId);
    // vtkIdType* cells = this->Links->GetCells(ptId);
    if (numCells < minNumCells)
    {
      minNumCells = numCells;
      minCells = cells;
      minPtId = ptId;
    }
  }

  if (numPts == 0)
  {
    vtkErrorMacro("input point ids empty.");
    return;
  }

  // Now for each cell, see if it contains all the points
  // in the ptIds list.
  for (int i = 0; i < minNumCells; i++)
  {
    if (minCells[i] != cellId) // don't include current cell
    {
      vtkIdType* cellPts = this->GetCellPoints(minCells[i]);
      bool match = true;
      for (int j = 0; j < numPts && match; j++) // for all pts in input cell
      {
        if (pts[j] != minPtId) // of course minPtId is contained by cell
        {
          match = false;
          for (int k = 0; k < 8; k++) // for all points in candidate cell
          {
            if (pts[j] == cellPts[k])
            {
              match = true; // a match was found
              break;
            }
          } // for all points in current cell
        }   // if not guaranteed match
      }     // for all points in input cell
      if (match)
      {
        cellIds->InsertNextId(minCells[i]);
      }
    } // if not the reference cell
  }   // for all candidate cells attached to point
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::InternalCopy(vtkExplicitStructuredGrid* src)
{
  // Internal method used for copying specific members
  this->SetExtent(src->GetExtent());
  this->SetFacesConnectivityFlagsArrayName(src->GetFacesConnectivityFlagsArrayName());
}

//----------------------------------------------------------------------------
// Copy the topological structure of an input structured grid.
void vtkExplicitStructuredGrid::CopyStructure(vtkDataSet* ds)
{
  this->Superclass::CopyStructure(ds);

  vtkExplicitStructuredGrid* grid = vtkExplicitStructuredGrid::SafeDownCast(ds);
  if (grid)
  {
    this->InternalCopy(grid);
    this->SetCells(grid->GetCells());
    if (this->Links)
    {
      this->Links->Delete();
      this->Links = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::ShallowCopy(vtkDataObject* dataObject)
{
  this->Superclass::ShallowCopy(dataObject);

  if (this->Links)
  {
    this->Links->Delete();
    this->Links = nullptr;
  }

  vtkExplicitStructuredGrid* grid = vtkExplicitStructuredGrid::SafeDownCast(dataObject);
  if (grid)
  {
    this->InternalCopy(grid);

    this->SetCells(grid->GetCells());

    if (grid->Links)
    {
      this->BuildLinks();
    }
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::DeepCopy(vtkDataObject* dataObject)
{
  this->Superclass::DeepCopy(dataObject);

  if (this->Links)
  {
    this->Links->Delete();
    this->Links = nullptr;
  }

  vtkExplicitStructuredGrid* grid = vtkExplicitStructuredGrid::SafeDownCast(dataObject);
  if (grid)
  {
    this->InternalCopy(grid);

    vtkNew<vtkCellArray> cells;
    cells->DeepCopy(grid->GetCells());
    this->SetCells(cells.Get());
    if (grid->Links)
    {
      this->BuildLinks();
    }
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i - 1, 0, j - 1, 0, k - 1);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::SetDimensions(int dim[3])
{
  this->SetDimensions(dim[0], dim[1], dim[2]);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::SetExtent(int x0, int x1, int y0, int y1, int z0, int z1)
{
  this->Extent[0] = x0;
  this->Extent[1] = x1;
  this->Extent[2] = y0;
  this->Extent[3] = y1;
  this->Extent[4] = z0;
  this->Extent[5] = z1;

  if (this->Links)
  {
    this->Links->Delete();
    this->Links = nullptr;
  }

  vtkIdType expectedCells = (this->Extent[1] - this->Extent[0]) *
    (this->Extent[3] - this->Extent[2]) * (this->Extent[5] - this->Extent[4]);

  vtkNew<vtkCellArray> cells;
  this->SetCells(cells);

  // Initialize the cell array
  if (expectedCells > 0)
  {
    cells->AllocateEstimate(expectedCells, 8);
    vtkIdType ids[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    for (vtkIdType i = 0; i < expectedCells; i++)
    {
      cells->InsertNextCell(8, ids);
    }
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::SetExtent(int extent[6])
{
  this->SetExtent(extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::BuildLinks()
{
  // Remove the old links if they are already built
  if (this->Links)
  {
    this->Links->Delete();
  }

  // Different types of links depending on whether the data can be edited after
  // initial creation.
  if (this->Editable)
  {
    this->Links = vtkCellLinks::New();
    static_cast<vtkCellLinks*>(this->Links)->Allocate(this->GetNumberOfPoints());
  }
  else
  {
    this->Links = vtkStaticCellLinks::New();
  }
  this->Links->BuildLinks(this);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::GenerateGhostArray(int zeroExt[6], bool vtkNotUsed(cellOnly))
{
  int extent[6];
  this->Information->Get(vtkDataObject::DATA_EXTENT(), extent);

  this->AllocateCellGhostArray();
  this->AllocatePointGhostArray();

  bool sameExtent = true;
  for (int i = 0; i < 6; i++)
  {
    if (extent[i] != zeroExt[i])
    {
      sameExtent = false;
      break;
    }
  }
  if (sameExtent)
  {
    return;
  }

  vtkUnsignedCharArray* ghostCells = this->GetCellGhostArray();

  vtkIdType index = 0;

  // Loop through the cells in this image.
  // Cells may be 2d or 1d ... Treat all as 3D
  if (extent[0] == extent[1])
  {
    ++extent[1];
    ++zeroExt[1];
  }
  if (extent[2] == extent[3])
  {
    ++extent[3];
    ++zeroExt[3];
  }
  if (extent[4] == extent[5])
  {
    ++extent[5];
    ++zeroExt[5];
  }

  // Loop
  for (int k = extent[4]; k < extent[5]; ++k)
  { // Determine the Manhattan distances to zero extent.
    int dk = 0;
    if (k < zeroExt[4])
    {
      dk = zeroExt[4] - k;
    }
    if (k >= zeroExt[5])
    {
      dk = k - zeroExt[5] + 1;
    }
    for (int j = extent[2]; j < extent[3]; ++j)
    {
      int dj = 0;
      if (j < zeroExt[2])
      {
        dj = zeroExt[2] - j;
      }
      if (j >= zeroExt[3])
      {
        dj = j - zeroExt[3] + 1;
      }
      for (int i = extent[0]; i < extent[1]; ++i)
      {
        int di = 0;
        if (i < zeroExt[0])
        {
          di = zeroExt[0] - i;
        }
        if (i >= zeroExt[1])
        {
          di = i - zeroExt[1] + 1;
        }
        // Compute Manhattan distance.
        int dist = di;
        if (dj > dist)
        {
          dist = dj;
        }
        if (dk > dist)
        {
          dist = dk;
        }
        unsigned char value = ghostCells->GetValue(index);
        if (dist > 0)
        {
          value |= vtkDataSetAttributes::DUPLICATECELL;
        }
        ghostCells->SetValue(index, value);
        index++;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::ComputeFacesConnectivityFlagsArray()
{
  vtkIdType nbCells = this->GetNumberOfCells();

  const char* name = this->FacesConnectivityFlagsArrayName ? this->FacesConnectivityFlagsArrayName
                                                           : "ConnectivityFlags";
  this->SetFacesConnectivityFlagsArrayName(name);

  vtkNew<vtkUnsignedCharArray> connectivity;
  connectivity->SetName(name);
  connectivity->SetNumberOfComponents(1);
  connectivity->SetNumberOfTuples(nbCells);
  this->GetCellData()->AddArray(connectivity.GetPointer());

  for (vtkIdType c = 0; c < nbCells; c++)
  {
    vtkIdType* cellPtsIds = this->GetCellPoints(c);

    unsigned char mask = 0;
    vtkIdType neighbors[6];
    this->GetCellNeighbors(c, neighbors);
    for (int f = 0, n = 0; f < 6; f++, n += 8)
    {
      vtkIdType neighbor = neighbors[f];
      if (neighbor >= 0)
      {
        vtkIdType* neiCellPtsIds = this->GetCellPoints(neighbor);
        if (cellPtsIds[HEXAHEDRON_POINT_MAP[n + 0]] == neiCellPtsIds[HEXAHEDRON_POINT_MAP[n + 1]] &&
          cellPtsIds[HEXAHEDRON_POINT_MAP[n + 2]] == neiCellPtsIds[HEXAHEDRON_POINT_MAP[n + 3]] &&
          cellPtsIds[HEXAHEDRON_POINT_MAP[n + 4]] == neiCellPtsIds[HEXAHEDRON_POINT_MAP[n + 5]] &&
          cellPtsIds[HEXAHEDRON_POINT_MAP[n + 6]] == neiCellPtsIds[HEXAHEDRON_POINT_MAP[n + 7]])
        {
          mask |= (1 << f);
        }
      }
    }
    connectivity->SetValue(c, mask);
  }
}

//----------------------------------------------------------------------------
bool vtkExplicitStructuredGrid::HasAnyBlankCells()
{
  return this->IsAnyBitSet(this->GetCellGhostArray(), vtkDataSetAttributes::HIDDENCELL);
}

//----------------------------------------------------------------------------
// Turn off a particular data cell.
void vtkExplicitStructuredGrid::BlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  if (!ghosts)
  {
    ghosts = this->AllocateCellGhostArray();
  }
  ghosts->SetValue(cellId, ghosts->GetValue(cellId) | vtkDataSetAttributes::HIDDENCELL);
  assert(!this->IsCellVisible(cellId));
}

//----------------------------------------------------------------------------
// Turn on a particular data cell.
void vtkExplicitStructuredGrid::UnBlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  if (ghosts)
  {
    ghosts->SetValue(cellId, ghosts->GetValue(cellId) & ~vtkDataSetAttributes::HIDDENCELL);
  }
}

//----------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkExplicitStructuredGrid::IsCellVisible(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  return (ghosts && (ghosts->GetValue(cellId) & MASKED_CELL_VALUE)) ? 0 : 1;
}

//----------------------------------------------------------------------------
// Return non-zero if the specified cell is a ghost cell
unsigned char vtkExplicitStructuredGrid::IsCellGhost(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  return (ghosts && (ghosts->GetValue(cellId) & vtkDataSetAttributes::DUPLICATECELL)) ? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkExplicitStructuredGrid::HasAnyGhostCells()
{
  return this->IsAnyBitSet(this->GetCellGhostArray(), vtkDataSetAttributes::DUPLICATECELL);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::Crop(const int* updateExtent)
{
  this->Crop(this, updateExtent, false);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::Crop(
  vtkExplicitStructuredGrid* input, const int* updateExtent, bool generateOriginalCellIds)
{
  // The old extent
  int oldExtent[6];
  input->GetExtent(oldExtent);

  if (updateExtent[1] < updateExtent[0] || updateExtent[3] < updateExtent[2] ||
    updateExtent[5] < updateExtent[4])
  {
    return;
  }

  // The new extent
  int newExtent[6];
  for (int i = 0; i < 3; ++i)
  {
    newExtent[i * 2] = updateExtent[i * 2];
    if (newExtent[i * 2] < oldExtent[i * 2])
    {
      newExtent[i * 2] = oldExtent[i * 2];
    }
    newExtent[i * 2 + 1] = updateExtent[i * 2 + 1];
    if (newExtent[i * 2 + 1] > oldExtent[i * 2 + 1])
    {
      newExtent[i * 2 + 1] = oldExtent[i * 2 + 1];
    }
    if (newExtent[i * 2] == newExtent[i * 2 + 1])
    {
      if (newExtent[i * 2 + 1] == oldExtent[i * 2 + 1])
      {
        newExtent[i * 2] -= 1;
      }
      else
      {
        newExtent[i * 2 + 1] += 1;
      }
    }
  }

  // If extents already match, then we don't need to do anything.
  if (oldExtent[0] == newExtent[0] && oldExtent[1] == newExtent[1] &&
    oldExtent[2] == newExtent[2] && oldExtent[3] == newExtent[3] && oldExtent[4] == newExtent[4] &&
    oldExtent[5] == newExtent[5])
  {
    if (this != input)
    {
      this->ShallowCopy(input);
    }

    if (generateOriginalCellIds)
    {
      // CellArray which links the new cells ids with the old ones
      vtkNew<vtkIdTypeArray> originalCellIds;
      originalCellIds->SetName("vtkOriginalCellIds");
      originalCellIds->SetNumberOfComponents(1);
      this->GetCellData()->AddArray(originalCellIds.GetPointer());
      vtkIdType inSize = this->GetNumberOfCells();
      originalCellIds->Allocate(inSize);
      for (vtkIdType i = 0; i < inSize; i++)
      {
        originalCellIds->InsertValue(i, i);
      }
    }
  }
  else
  {
    // Check the points to avoid empty data objects.
    if (!input->GetPoints())
    {
      return;
    }

    // shallow copy points and point data to this ESG
    this->SetPoints(input->GetPoints());
    this->GetPointData()->ShallowCopy(input->GetPointData());

    vtkDebugMacro("Cropping Explicit Structured Grid");

    // Compute cells extent
    int oldCellExtent[6], newCellExtent[6];
    vtkStructuredData::GetCellExtentFromPointExtent(oldExtent, oldCellExtent);
    vtkStructuredData::GetCellExtentFromPointExtent(newExtent, newCellExtent);

    // Allocate necessary objects
    int outSize = (newCellExtent[1] - newCellExtent[0] + 1) *
      (newCellExtent[3] - newCellExtent[2] + 1) * (newCellExtent[5] - newCellExtent[4] + 1);
    this->SetExtent(newExtent);

    vtkCellData* inCD = input->GetCellData();
    vtkCellData* outCD = this->GetCellData();
    outCD->CopyAllocate(inCD, outSize, outSize);

    vtkNew<vtkCellArray> cells;
    cells->AllocateEstimate(outSize, 8);

    // CellArray which links the new cells ids with the old ones
    vtkNew<vtkIdTypeArray> originalCellIds;
    if (generateOriginalCellIds)
    {
      originalCellIds->SetName("vtkOriginalCellIds");
      originalCellIds->SetNumberOfComponents(1);
      originalCellIds->Allocate(outSize);
    }

    // Browse input data and copy cell attributes to output
    for (int k = newCellExtent[4]; k <= newCellExtent[5]; ++k)
    {
      for (int j = newCellExtent[2]; j <= newCellExtent[3]; ++j)
      {
        for (int i = newCellExtent[0]; i <= newCellExtent[1]; ++i)
        {
          int idx = input->ComputeCellId(i, j, k);
          vtkNew<vtkIdList> ptIds;
          input->GetCellPoints(idx, ptIds);

          // insert cell and copy cell data
          vtkIdType nCellId = cells->InsertNextCell(ptIds);
          outCD->CopyData(inCD, idx, nCellId);

          if (generateOriginalCellIds)
          {
            originalCellIds->InsertValue(nCellId, idx);
          }
        }
      }
    }

    if (generateOriginalCellIds)
    {
      outCD->AddArray(originalCellIds);
      originalCellIds->Squeeze();
    }
    cells->Squeeze();
    this->SetCells(cells);

    if (this->GetLinks())
    {
      this->BuildLinks();
    }

    this->ComputeFacesConnectivityFlagsArray();
  }
}

//----------------------------------------------------------------------------
unsigned long vtkExplicitStructuredGrid::GetActualMemorySize()
{
  unsigned long size = this->Superclass::GetActualMemorySize();

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
void vtkExplicitStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int dim[3];
  this->GetDimensions(dim);
  os << indent << "Dimensions: (" << dim[0] << ", " << dim[1] << ", " << dim[2] << ")\n";

  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] << ", "
     << this->Extent[2] << ", " << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << ")\n";
}

//----------------------------------------------------------------------------
// Override this method because of blanking
void vtkExplicitStructuredGrid::ComputeScalarRange()
{
  if (this->GetMTime() > this->ScalarRangeComputeTime)
  {
    vtkDataArray* ptScalars = this->PointData->GetScalars();
    vtkDataArray* cellScalars = this->CellData->GetScalars();

    double ptRange[2];
    ptRange[0] = VTK_DOUBLE_MAX;
    ptRange[1] = VTK_DOUBLE_MIN;
    if (ptScalars)
    {
      int num = this->GetNumberOfPoints();
      for (int id = 0; id < num; id++)
      {
        double s = ptScalars->GetComponent(id, 0);
        if (s < ptRange[0])
        {
          ptRange[0] = s;
        }
        if (s > ptRange[1])
        {
          ptRange[1] = s;
        }
      }
    }

    double cellRange[2];
    cellRange[0] = ptRange[0];
    cellRange[1] = ptRange[1];
    if (cellScalars)
    {
      int num = this->GetNumberOfCells();
      for (int id = 0; id < num; id++)
      {
        double s = cellScalars->GetComponent(id, 0);
        if (s < cellRange[0])
        {
          cellRange[0] = s;
        }
        if (s > cellRange[1])
        {
          cellRange[1] = s;
        }
      }
    }

    this->ScalarRange[0] = (cellRange[0] >= VTK_DOUBLE_MAX ? 0.0 : cellRange[0]);
    this->ScalarRange[1] = (cellRange[1] <= VTK_DOUBLE_MIN ? 1.0 : cellRange[1]);

    this->ScalarRangeComputeTime.Modified();
  }
}

//----------------------------------------------------------------------------
vtkExplicitStructuredGrid* vtkExplicitStructuredGrid::GetData(vtkInformation* info)
{
  return info ? vtkExplicitStructuredGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkExplicitStructuredGrid* vtkExplicitStructuredGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkExplicitStructuredGrid::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::CheckAndReorderFaces()
{
  // Check faces are on the correct axis
  this->InternalCheckAndReorderFaces(true);

  // Check if faces are mirrored or not
  this->InternalCheckAndReorderFaces(false);
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::InternalCheckAndReorderFaces(bool swapFlag)
{
  // Find connected faces
  int foundFaces[3] = { -1, -1, -1 };
  this->FindConnectedFaces(foundFaces);

  // Compute correcting transformation
  int* ptsMap;
  int transformFlag[3] = { 0, 0, 0 };
  if (swapFlag)
  {
    vtkExplicitStructuredGrid::ComputeSwapFlag(foundFaces, transformFlag);
    ptsMap = SWAP_HEXAHEDRON_POINT_MAP;
  }
  else
  {
    vtkExplicitStructuredGrid::ComputeMirrorFlag(foundFaces, transformFlag);
    ptsMap = MIRROR_HEXAHEDRON_POINT_MAP;
  }

  // Reorder cell points accordingly
  this->ReorderCellsPoints(ptsMap, transformFlag);
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGrid::FindConnectedFaces(int foundFaces[3])
{
  int extent[6];
  this->GetExtent(extent);
  int nFoundFaces = 0;
  int neiAxisMod[3] = { 0, 0, 0 };
  vtkIdType ijkId[3];
  vtkIdType id0, neiCellId;
  vtkIdType* cellPtsIds;
  vtkIdType* neiCellPtsIds;

  // Look for continuous connected visible cells for each axis in the whole dataset
  // And identify connected faces
  for (ijkId[0] = extent[0]; ijkId[0] < extent[1]; ijkId[0]++)
  {
    for (ijkId[1] = extent[2]; ijkId[1] < extent[3]; ijkId[1]++)
    {
      for (ijkId[2] = extent[4]; ijkId[2] < extent[5]; ijkId[2]++)
      {
        id0 = this->ComputeCellId(ijkId[0], ijkId[1], ijkId[2]);
        if (this->IsCellVisible(id0))
        {
          for (int axis = 0; axis < 3; axis++)
          {
            // A visible cell have been found
            if (foundFaces[axis] == -1 && ijkId[axis] + 1 < extent[axis * 2 + 1])
            {
              neiAxisMod[axis]++;

              // find it's neighbour in the current axis
              neiCellId = this->ComputeCellId(
                ijkId[0] + neiAxisMod[0], ijkId[1] + neiAxisMod[1], ijkId[2] + neiAxisMod[2]);
              if (this->IsCellVisible(neiCellId))
              {
                // Find if they are connected and by which faces they are connected
                cellPtsIds = this->GetCellPoints(id0);
                neiCellPtsIds = this->GetCellPoints(neiCellId);
                for (int n = 0; n < 6; n++)
                {
                  if (cellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 0]] ==
                      neiCellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 1]] &&
                    cellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 2]] ==
                      neiCellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 3]] &&
                    cellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 4]] ==
                      neiCellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 5]] &&
                    cellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 6]] ==
                      neiCellPtsIds[HEXAHEDRON_POINT_MAP[n * 8 + 7]])
                  {
                    // Correctly ordered faces would be the following
                    // Axis 0 -> face 1
                    // Axis 1 -> face 3
                    // Axis 2 -> face 5
                    // See vtkHexahedron.h for doc
                    foundFaces[axis] = n;
                    nFoundFaces++;
                    break;
                  }
                }
              }
              neiAxisMod[axis]--;
            }
          }
        }
      }
    }
  }
  vtkExplicitStructuredGrid::CheckConnectedFaces(nFoundFaces, foundFaces);
  return nFoundFaces;
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::CheckConnectedFaces(int& nFoundFaces, int foundFaces[3])
{
  // Check foundFaces coherence and extrapolate to find a missing faces if any
  switch (nFoundFaces)
  {
    case 1:
      // Only one face have been found, we will probably draw incorrect interior faces
      for (int axis = 0; axis < 3; axis++)
      {
        int foundFace = foundFaces[axis];
        // Check if the foundFace point to antoher axis
        if (foundFace != -1 && !(foundFace == 2 * axis) && !(foundFace == 2 * axis + 1))
        {
          // A single foundFace which changes face on multiple axis is incoherent and can't be
          // extrapolated from so we remove it. This means that incorrect interior faces will be
          // drawn.
          foundFaces[axis] = -1;
          nFoundFaces--;
        }
      }
      break;
    case 2:
    {
      // Two faces have been found, we can try to extapolate the last one
      int missingFaceAxis = -1;
      for (int axis = 0; axis < 3; axis++)
      {
        if (foundFaces[axis] == -1)
        {
          // Identify the axis missing a face
          missingFaceAxis = axis;
          break;
        }
      }
      int foundFaceAxisSum = 0;
      int faceSwitch = 1;
      for (int axis = 0; axis < 3; axis++)
      {
        if (axis != missingFaceAxis)
        {
          int foundFace = foundFaces[axis];
          int foundFaceAxis = static_cast<int>(std::floor((static_cast<double>(foundFace)) / 2.0));

          // The sum of the found face axis will always be 3, so compute the sum
          foundFaceAxisSum += foundFaceAxis;
          if (!(foundFace == 2 * axis) && !(foundFace == 2 * axis + 1))
          {
            // when switching axis, we still need to know if there is some mirroring
            // this identify mirroring
            faceSwitch = foundFace - foundFaceAxis * 2;
          }
        }
      }
      // Compute the actual missing face
      foundFaces[missingFaceAxis] = (3 - foundFaceAxisSum) * 2 + faceSwitch;
      nFoundFaces++;
    }
    break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::ComputeSwapFlag(int foundFaces[3], int swap[3])
{
  int permuWise = 1;
  for (int axis = 0; axis < 3; axis++)
  {
    int foundFace = foundFaces[axis];
    if (foundFace != -1)
    {
      int foundFaceAxis = static_cast<int>(std::floor((static_cast<double>(foundFace)) / 2.0));
      if (foundFaceAxis != axis)
      {
        // Compute the swap
        swap[3 - foundFaceAxis - axis] = true;
        if (axis - foundFaceAxis == 1)
        {
          // In case of permutation, we need to know in which order the permutation have been done
          permuWise = 0;
        }
      }
    }
  }
  // Manage the permutation case
  if (swap[0] && swap[1] && swap[2])
  {
    swap[1 + permuWise] = false;
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::ComputeMirrorFlag(int foundFaces[3], int mirror[3])
{
  for (int axis = 0; axis < 3; axis++)
  {
    int foundFace = foundFaces[axis];
    if (foundFace != -1)
    {
      if (foundFace % 2 == 0)
      {
        mirror[axis] = true;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGrid::ReorderCellsPoints(const int* ptsMap, const int transformFlag[3])
{
  // Reorder all cells if necessary
  vtkIdType ids[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  vtkIdType ids2[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  vtkIdType npts, *pts, *ptsTmp, *ptsTmp2;
  vtkCellArray* cells = this->GetCells();
  for (vtkIdType cellId = 0; cellId < this->GetNumberOfCells(); cellId++)
  {
    if (this->IsCellVisible(cellId))
    {
      this->GetCellPoints(cellId, npts, pts);
      for (int ptIdx = 0; ptIdx < 8; ptIdx++)
      {
        ids[ptIdx] = pts[ptIdx];
      }
      ptsTmp = ids;
      ptsTmp2 = ids2;
      for (int axis = 0; axis < 3; axis++)
      {
        if (transformFlag[axis])
        {
          for (int ptIdx = 0; ptIdx < 8; ptIdx++)
          {
            ptsTmp2[ptIdx] = ptsTmp[ptsMap[axis * 8 + ptIdx]];
          }
          std::swap(ptsTmp, ptsTmp2);
        }
      }
      cells->ReplaceCellAtId(cellId, 8, ptsTmp);
      this->GetCellPoints(cellId, npts, pts);
    }
  }
}
