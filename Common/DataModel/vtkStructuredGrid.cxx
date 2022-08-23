/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStructuredGrid.h"

#include "vtkCellData.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkQuad.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertex.h"

vtkStandardNewMacro(vtkStructuredGrid);
vtkStandardExtendedNewMacro(vtkStructuredGrid);

#define vtkAdjustBoundsMacro(A, B)                                                                 \
  A[0] = (B[0] < A[0] ? B[0] : A[0]);                                                              \
  A[1] = (B[0] > A[1] ? B[0] : A[1]);                                                              \
  A[2] = (B[1] < A[2] ? B[1] : A[2]);                                                              \
  A[3] = (B[1] > A[3] ? B[1] : A[3]);                                                              \
  A[4] = (B[2] < A[4] ? B[2] : A[4]);                                                              \
  A[5] = (B[2] > A[5] ? B[2] : A[5])

//------------------------------------------------------------------------------
vtkStructuredGrid::vtkStructuredGrid()
{
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Quad = vtkQuad::New();
  this->Hexahedron = vtkHexahedron::New();
  this->EmptyCell = vtkEmptyCell::New();

#if !defined(VTK_LEGACY_REMOVE)
  this->Dimensions[0] = 0.0;
  this->Dimensions[1] = 0.0;
  this->Dimensions[2] = 0.0;
#endif

  this->DataDescription = VTK_EMPTY;

  const int extent[6] = { 0, -1, 0, -1, 0, -1 };
  memcpy(this->Extent, extent, sizeof(extent));

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//------------------------------------------------------------------------------
vtkStructuredGrid::~vtkStructuredGrid()
{
  this->Vertex->Delete();
  this->Line->Delete();
  this->Quad->Delete();
  this->Hexahedron->Delete();
  this->EmptyCell->Delete();
}

//------------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured grid.
void vtkStructuredGrid::CopyStructure(vtkDataSet* ds)
{
  vtkStructuredGrid* sg = static_cast<vtkStructuredGrid*>(ds);
  Superclass::CopyStructure(ds);

#if !defined(VTK_LEGACY_REMOVE)
  for (int i = 0; i < 3; i++)
  {
    this->Dimensions[i] = sg->Dimensions[i];
  }
#endif

  this->SetExtent(sg->GetExtent());

  this->DataDescription = sg->DataDescription;

  if (ds->HasAnyBlankPoints())
  {
    // there is blanking
    this->GetPointData()->AddArray(ds->GetPointGhostArray());
  }
  if (ds->HasAnyBlankCells())
  {
    // there is blanking
    this->GetCellData()->AddArray(ds->GetCellGhostArray());
  }
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::Initialize()
{
  this->Superclass::Initialize();

  if (this->Information)
  {
    this->SetDimensions(0, 0, 0);
  }
}

//------------------------------------------------------------------------------
int vtkStructuredGrid::GetCellType(vtkIdType cellId)
{
  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    return VTK_EMPTY_CELL;
  }

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      return VTK_EMPTY_CELL;

    case VTK_SINGLE_POINT:
      return VTK_VERTEX;

    case VTK_X_LINE:
    case VTK_Y_LINE:
    case VTK_Z_LINE:
      return VTK_LINE;

    case VTK_XY_PLANE:
    case VTK_YZ_PLANE:
    case VTK_XZ_PLANE:
      return VTK_QUAD;

    case VTK_XYZ_GRID:
      return VTK_HEXAHEDRON;

    default:
      vtkErrorMacro(<< "Bad data description!");
      return VTK_EMPTY_CELL;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredGrid::GetCellSize(vtkIdType cellId)
{
  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    return 0;
  }

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      return 0;

    case VTK_SINGLE_POINT:
      return 1;

    case VTK_X_LINE:
    case VTK_Y_LINE:
    case VTK_Z_LINE:
      return 2;

    case VTK_XY_PLANE:
    case VTK_YZ_PLANE:
    case VTK_XZ_PLANE:
      return 4;

    case VTK_XYZ_GRID:
      return 8;

    default:
      vtkErrorMacro(<< "Bad data description!");
      return 0;
  }
}

//------------------------------------------------------------------------------
vtkCell* vtkStructuredGrid::GetCell(vtkIdType cellId)
{
  // Make sure data is defined
  if (!this->Points)
  {
    vtkErrorMacro(<< "No data");
    return nullptr;
  }

  // Get dimensions
  int dims[3];
  this->GetDimensions(dims);

  vtkCell* cell = nullptr;
  vtkIdType idx;
  int i, j, k;
  int d01, offset1, offset2;

  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    return this->EmptyCell;
  }

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      return this->EmptyCell;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = this->Vertex;
      cell->PointIds->SetId(0, 0);
      break;

    case VTK_X_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_Y_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_Z_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_XY_PLANE:
      cell = this->Quad;
      i = cellId % (dims[0] - 1);
      j = cellId / (dims[0] - 1);
      idx = i + j * dims[0];
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_YZ_PLANE:
      cell = this->Quad;
      j = cellId % (dims[1] - 1);
      k = cellId / (dims[1] - 1);
      idx = j + k * dims[1];
      offset1 = 1;
      offset2 = dims[1];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_XZ_PLANE:
      cell = this->Quad;
      i = cellId % (dims[0] - 1);
      k = cellId / (dims[0] - 1);
      idx = i + k * dims[0];
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_XYZ_GRID:
      cell = this->Hexahedron;
      d01 = dims[0] * dims[1];
      i = cellId % (dims[0] - 1);
      j = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      k = cellId / ((dims[0] - 1) * (dims[1] - 1));
      idx = i + j * dims[0] + k * d01;
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      idx += d01;
      cell->PointIds->SetId(4, idx);
      cell->PointIds->SetId(5, idx + offset1);
      cell->PointIds->SetId(6, idx + offset1 + offset2);
      cell->PointIds->SetId(7, idx + offset2);
      break;

    default:
      vtkErrorMacro(<< "Invalid DataDescription.");
      return nullptr;
  }

  // Extract point coordinates and point ids. NOTE: the ordering of the vtkQuad
  // and vtkHexahedron cells are tricky.
  int NumberOfIds = cell->PointIds->GetNumberOfIds();
  for (i = 0; i < NumberOfIds; i++)
  {
    idx = cell->PointIds->GetId(i);
    cell->Points->SetPoint(i, this->Points->GetPoint(idx));
  }
  return cell;
}

//------------------------------------------------------------------------------
vtkCell* vtkStructuredGrid::GetCell(int i, int j, int k)
{
  // Make sure data is defined
  if (!this->Points)
  {
    vtkErrorMacro(<< "No data");
    return nullptr;
  }

  // Get dimensions
  int dims[3];
  this->GetDimensions(dims);

  vtkIdType cellId = i + (j + (k * (dims[1] - 1))) * (dims[0] - 1);
  vtkCell* cell = nullptr;
  vtkIdType idx;
  int d01, offset1, offset2;

  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    return this->EmptyCell;
  }

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      return this->EmptyCell;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = this->Vertex;
      cell->PointIds->SetId(0, 0);
      break;

    case VTK_X_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_Y_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_Z_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_XY_PLANE:
      cell = this->Quad;
      idx = i + j * dims[0];
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_YZ_PLANE:
      cell = this->Quad;
      idx = j + k * dims[1];
      offset1 = 1;
      offset2 = dims[1];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_XZ_PLANE:
      cell = this->Quad;
      idx = i + k * dims[0];
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_XYZ_GRID:
      cell = this->Hexahedron;
      d01 = dims[0] * dims[1];
      idx = i + j * dims[0] + k * d01;
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      idx += d01;
      cell->PointIds->SetId(4, idx);
      cell->PointIds->SetId(5, idx + offset1);
      cell->PointIds->SetId(6, idx + offset1 + offset2);
      cell->PointIds->SetId(7, idx + offset2);
      break;

    default:
      vtkErrorMacro(<< "Invalid DataDescription.");
      return nullptr;
  }

  // Extract point coordinates and point ids. NOTE: the ordering of the vtkQuad
  // and vtkHexahedron cells are tricky.
  int NumberOfIds = cell->PointIds->GetNumberOfIds();
  for (i = 0; i < NumberOfIds; i++)
  {
    idx = cell->PointIds->GetId(i);
    cell->Points->SetPoint(i, this->Points->GetPoint(idx));
  }
  return cell;
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  // Make sure data is defined
  if (!this->Points)
  {
    vtkErrorMacro(<< "No data");
    return;
  }

  // Get dimensions
  int dims[3];
  this->GetDimensions(dims);

  vtkIdType idx;
  int i, j, k;
  int d01, offset1, offset2;
  double x[3];

  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    cell->SetCellTypeToEmptyCell();
    return;
  }

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      cell->SetCellTypeToEmptyCell();
      return;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell->SetCellTypeToVertex();
      cell->PointIds->SetId(0, 0);
      break;

    case VTK_X_LINE:
      cell->SetCellTypeToLine();
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_Y_LINE:
      cell->SetCellTypeToLine();
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_Z_LINE:
      cell->SetCellTypeToLine();
      cell->PointIds->SetId(0, cellId);
      cell->PointIds->SetId(1, cellId + 1);
      break;

    case VTK_XY_PLANE:
      cell->SetCellTypeToQuad();
      i = cellId % (dims[0] - 1);
      j = cellId / (dims[0] - 1);
      idx = i + j * dims[0];
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_YZ_PLANE:
      cell->SetCellTypeToQuad();
      j = cellId % (dims[1] - 1);
      k = cellId / (dims[1] - 1);
      idx = j + k * dims[1];
      offset1 = 1;
      offset2 = dims[1];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_XZ_PLANE:
      cell->SetCellTypeToQuad();
      i = cellId % (dims[0] - 1);
      k = cellId / (dims[0] - 1);
      idx = i + k * dims[0];
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      break;

    case VTK_XYZ_GRID:
      cell->SetCellTypeToHexahedron();
      d01 = dims[0] * dims[1];
      i = cellId % (dims[0] - 1);
      j = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      k = cellId / ((dims[0] - 1) * (dims[1] - 1));
      idx = i + j * dims[0] + k * d01;
      offset1 = 1;
      offset2 = dims[0];

      cell->PointIds->SetId(0, idx);
      cell->PointIds->SetId(1, idx + offset1);
      cell->PointIds->SetId(2, idx + offset1 + offset2);
      cell->PointIds->SetId(3, idx + offset2);
      idx += d01;
      cell->PointIds->SetId(4, idx);
      cell->PointIds->SetId(5, idx + offset1);
      cell->PointIds->SetId(6, idx + offset1 + offset2);
      cell->PointIds->SetId(7, idx + offset2);
      break;
  }

  // Extract point coordinates and point ids. NOTE: the ordering of the vtkQuad
  // and vtkHexahedron cells are tricky.
  int NumberOfIds = cell->PointIds->GetNumberOfIds();
  for (i = 0; i < NumberOfIds; i++)
  {
    idx = cell->PointIds->GetId(i);
    this->Points->GetPoint(idx, x);
    cell->Points->SetPoint(i, x);
  }
}

//------------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkStructuredGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  // Make sure data is defined
  if (!this->Points)
  {
    vtkErrorMacro(<< "No data");
    return;
  }

  // Get dimensions
  int dims[3];
  this->GetDimensions(dims);

  vtkIdType idx = 0;
  int i, j, k;
  vtkIdType d01;
  int offset1 = 0;
  int offset2 = 0;
  double x[3];

  vtkMath::UninitializeBounds(bounds);

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      return;
    case VTK_SINGLE_POINT: // cellId can only be = 0
      this->Points->GetPoint(0, x);
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];
      break;

    case VTK_X_LINE:
    case VTK_Y_LINE:
    case VTK_Z_LINE:
      this->Points->GetPoint(cellId, x);
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];

      this->Points->GetPoint(cellId + 1, x);
      vtkAdjustBoundsMacro(bounds, x);
      break;

    case VTK_XY_PLANE:
    case VTK_YZ_PLANE:
    case VTK_XZ_PLANE:
      if (this->DataDescription == VTK_XY_PLANE)
      {
        i = cellId % (dims[0] - 1);
        j = cellId / (dims[0] - 1);
        idx = i + j * dims[0];
        offset1 = 1;
        offset2 = dims[0];
      }
      else if (this->DataDescription == VTK_YZ_PLANE)
      {
        j = cellId % (dims[1] - 1);
        k = cellId / (dims[1] - 1);
        idx = j + k * dims[1];
        offset1 = 1;
        offset2 = dims[1];
      }
      else if (this->DataDescription == VTK_XZ_PLANE)
      {
        i = cellId % (dims[0] - 1);
        k = cellId / (dims[0] - 1);
        idx = i + k * dims[0];
        offset1 = 1;
        offset2 = dims[0];
      }

      this->Points->GetPoint(idx, x);
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];

      this->Points->GetPoint(idx + offset1, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset1 + offset2, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset2, x);
      vtkAdjustBoundsMacro(bounds, x);

      break;

    case VTK_XYZ_GRID:
      d01 = dims[0] * dims[1];
      i = cellId % (dims[0] - 1);
      j = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      k = cellId / ((dims[0] - 1) * (dims[1] - 1));
      idx = i + j * dims[0] + k * d01;
      offset1 = 1;
      offset2 = dims[0];

      this->Points->GetPoint(idx, x);
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];

      this->Points->GetPoint(idx + offset1, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset1 + offset2, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset2, x);
      vtkAdjustBoundsMacro(bounds, x);

      idx += d01;

      this->Points->GetPoint(idx, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset1, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset1 + offset2, x);
      vtkAdjustBoundsMacro(bounds, x);

      this->Points->GetPoint(idx + offset2, x);
      vtkAdjustBoundsMacro(bounds, x);

      break;
  }
}

//------------------------------------------------------------------------------
// Turn off a particular data point.
void vtkStructuredGrid::BlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if (!ghosts)
  {
    ghosts = this->AllocatePointGhostArray();
  }
  ghosts->SetValue(ptId, ghosts->GetValue(ptId) | vtkDataSetAttributes::HIDDENPOINT);
  assert(!this->IsPointVisible(ptId));
}

//------------------------------------------------------------------------------
// Turn on a particular data point.
void vtkStructuredGrid::UnBlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if (ghosts)
  {
    ghosts->SetValue(ptId, ghosts->GetValue(ptId) & ~vtkDataSetAttributes::HIDDENPOINT);
  }
  assert(this->IsPointVisible(ptId));
}

//------------------------------------------------------------------------------
// Turn off a particular data cell.
void vtkStructuredGrid::BlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  if (!ghosts)
  {
    ghosts = this->AllocateCellGhostArray();
  }
  ghosts->SetValue(cellId, ghosts->GetValue(cellId) | vtkDataSetAttributes::HIDDENCELL);
  assert(!this->IsCellVisible(cellId));
}

//------------------------------------------------------------------------------
// Turn on a particular data cell.
void vtkStructuredGrid::UnBlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  if (ghosts)
  {
    ghosts->SetValue(cellId, ghosts->GetValue(cellId) & ~vtkDataSetAttributes::HIDDENCELL);
  }
}

//------------------------------------------------------------------------------
unsigned char vtkStructuredGrid::IsPointVisible(vtkIdType pointId)
{
  return vtkStructuredData::IsPointVisible(pointId, this->GetPointGhostArray());
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCellDims(int cellDims[3])
{
  int dims[3];
  this->GetDimensions(dims);

  for (int i = 0; i < 3; ++i)
  {
    cellDims[i] = ((dims[i] - 1) < 1) ? 1 : dims[i] - 1;
  }
}

//------------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkStructuredGrid::IsCellVisible(vtkIdType cellId)
{
  int dims[3];
  this->GetDimensions(dims);

  return vtkStructuredData::IsCellVisible(
    cellId, dims, this->DataDescription, this->GetCellGhostArray(), this->GetPointGhostArray());
}

//------------------------------------------------------------------------------
// Set dimensions of structured grid dataset.
void vtkStructuredGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i - 1, 0, j - 1, 0, k - 1);
}

//------------------------------------------------------------------------------
// Set dimensions of structured grid dataset.
void vtkStructuredGrid::SetDimensions(const int dims[3])
{
  this->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
}

//------------------------------------------------------------------------------
// Get the points defining a cell. (See vtkDataSet for more info.)
void vtkStructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  // Get dimensions
  int dims[3];
  this->GetDimensions(dims);

  int iMin, iMax, jMin, jMax, kMin, kMax;
  vtkIdType d01 = dims[0] * dims[1];

  ptIds->Reset();
  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->DataDescription)
  {
    case VTK_EMPTY:
      return;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      ptIds->SetNumberOfIds(1);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      ptIds->SetNumberOfIds(2);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMax + jMin * dims[0] + kMin * d01);
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      ptIds->SetNumberOfIds(2);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMin + jMax * dims[0] + kMin * d01);
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      ptIds->SetNumberOfIds(2);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMin + jMin * dims[0] + kMax * d01);
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0] - 1);
      jMax = jMin + 1;
      ptIds->SetNumberOfIds(4);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMax + jMin * dims[0] + kMin * d01);
      ptIds->SetId(2, iMax + jMax * dims[0] + kMin * d01);
      ptIds->SetId(3, iMin + jMax * dims[0] + kMin * d01);
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1] - 1);
      kMax = kMin + 1;
      ptIds->SetNumberOfIds(4);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMin + jMax * dims[0] + kMin * d01);
      ptIds->SetId(2, iMin + jMax * dims[0] + kMax * d01);
      ptIds->SetId(3, iMin + jMin * dims[0] + kMax * d01);
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0] - 1);
      kMax = kMin + 1;
      ptIds->SetNumberOfIds(4);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMax + jMin * dims[0] + kMin * d01);
      ptIds->SetId(2, iMax + jMin * dims[0] + kMax * d01);
      ptIds->SetId(3, iMin + jMin * dims[0] + kMax * d01);
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      ptIds->SetNumberOfIds(8);
      ptIds->SetId(0, iMin + jMin * dims[0] + kMin * d01);
      ptIds->SetId(1, iMax + jMin * dims[0] + kMin * d01);
      ptIds->SetId(2, iMax + jMax * dims[0] + kMin * d01);
      ptIds->SetId(3, iMin + jMax * dims[0] + kMin * d01);
      ptIds->SetId(4, iMin + jMin * dims[0] + kMax * d01);
      ptIds->SetId(5, iMax + jMin * dims[0] + kMax * d01);
      ptIds->SetId(6, iMax + jMax * dims[0] + kMax * d01);
      ptIds->SetId(7, iMin + jMax * dims[0] + kMax * d01);
      break;
  }
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::SetExtent(VTK_FUTURE_CONST int extent[6])
{
  int description = vtkStructuredData::SetExtent(extent, this->Extent);

  if (description < 0) // improperly specified
  {
    vtkErrorMacro(<< "Bad Extent, retaining previous values");
  }

  if (description == VTK_UNCHANGED)
  {
    return;
  }

  this->DataDescription = description;

  this->Modified();

#if !defined(VTK_LEGACY_REMOVE)
  vtkStructuredData::GetDimensionsFromExtent(extent, this->Dimensions);
#endif
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::SetExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin;
  extent[1] = xMax;
  extent[2] = yMin;
  extent[3] = yMax;
  extent[4] = zMin;
  extent[5] = zMax;

  this->SetExtent(extent);
}

//------------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
int* vtkStructuredGrid::GetDimensions()
{
  this->GetDimensions(this->Dimensions);
  return this->Dimensions;
}
#endif

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetDimensions(int dims[3])
{
  const int* extent = this->Extent;
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
}

namespace
{
class CellVisibility
{
public:
  CellVisibility(vtkStructuredGrid* input)
    : Input(input)
  {
  }
  bool operator()(const vtkIdType id) { return !Input->IsCellVisible(id); }

private:
  vtkStructuredGrid* Input;
};
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  int numPtIds = ptIds->GetNumberOfIds();

  int dims[3];
  this->GetDimensions(dims);

  // Use special methods for speed
  switch (numPtIds)
  {
    case 0:
      cellIds->Reset();
      return;

    case 1:
    case 2:
    case 4: // vertex, edge, face neighbors
      vtkStructuredData::GetCellNeighbors(cellId, ptIds, cellIds, dims);
      break;

    default:
      this->Superclass::GetCellNeighbors(cellId, ptIds, cellIds);
  }

  // If blanking, remove blanked cells.
  if (this->GetPointGhostArray() || this->GetCellGhostArray())
  {
    vtkIdType* pCellIds = cellIds->GetPointer(0);
    vtkIdType* end =
      std::remove_if(pCellIds, pCellIds + cellIds->GetNumberOfIds(), CellVisibility(this));
    cellIds->Resize(std::distance(pCellIds, end));
  }
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCellNeighbors(
  vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds, int* seedLoc)
{
  int numPtIds = ptIds->GetNumberOfIds();

  int dims[3];
  this->GetDimensions(dims);

  // Use special methods for speed
  switch (numPtIds)
  {
    case 0:
      cellIds->Reset();
      return;

    case 1:
    case 2:
    case 4: // vertex, edge, face neighbors
      vtkStructuredData::GetCellNeighbors(cellId, ptIds, cellIds, dims, seedLoc);
      break;

    default:
      this->Superclass::GetCellNeighbors(cellId, ptIds, cellIds);
  }

  // If blanking, remove blanked cells.
  if (this->GetPointGhostArray() || this->GetCellGhostArray())
  {
    vtkIdType* pCellIds = cellIds->GetPointer(0);
    vtkIdType* end =
      std::remove_if(pCellIds, pCellIds + cellIds->GetNumberOfIds(), CellVisibility(this));
    cellIds->Resize(std::distance(pCellIds, end));
  }
}

//------------------------------------------------------------------------------
unsigned long vtkStructuredGrid::GetActualMemorySize()
{
  return this->Superclass::GetActualMemorySize();
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::ShallowCopy(vtkDataObject* dataObject)
{
  vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(dataObject);
  if (grid != nullptr)
  {
    this->InternalStructuredGridCopy(grid);
  }
  this->Superclass::ShallowCopy(dataObject);
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::DeepCopy(vtkDataObject* dataObject)
{
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());
  vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(dataObject);
  if (grid != nullptr)
  {
    this->InternalStructuredGridCopy(grid);
  }
  this->Superclass::DeepCopy(dataObject);
}

//------------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkStructuredGrid::InternalStructuredGridCopy(vtkStructuredGrid* src)
{
  this->DataDescription = src->DataDescription;

#if !defined(VTK_LEGACY_REMOVE)
  for (int idx = 0; idx < 3; ++idx)
  {
    this->Dimensions[idx] = src->Dimensions[idx];
  }
#endif

  memcpy(this->Extent, src->GetExtent(), 6 * sizeof(int));
}

//------------------------------------------------------------------------------
// Override this method because of blanking
void vtkStructuredGrid::ComputeScalarRange()
{
  if (this->GetMTime() > this->ScalarRangeComputeTime)
  {
    vtkDataArray* ptScalars = this->PointData->GetScalars();
    vtkDataArray* cellScalars = this->CellData->GetScalars();
    double ptRange[2];
    double cellRange[2];
    double s;

    ptRange[0] = VTK_DOUBLE_MAX;
    ptRange[1] = VTK_DOUBLE_MIN;
    if (ptScalars)
    {
      vtkIdType num = this->GetNumberOfPoints();
      for (vtkIdType id = 0; id < num; ++id)
      {
        if (this->IsPointVisible(id))
        {
          s = ptScalars->GetComponent(id, 0);
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
    }

    cellRange[0] = ptRange[0];
    cellRange[1] = ptRange[1];
    if (cellScalars)
    {
      vtkIdType num = this->GetNumberOfCells();
      for (vtkIdType id = 0; id < num; ++id)
      {
        if (this->IsCellVisible(id))
        {
          s = cellScalars->GetComponent(id, 0);
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
    }

    this->ScalarRange[0] = (cellRange[0] >= VTK_DOUBLE_MAX ? 0.0 : cellRange[0]);
    this->ScalarRange[1] = (cellRange[1] <= VTK_DOUBLE_MIN ? 1.0 : cellRange[1]);

    this->ScalarRangeComputeTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::Crop(const int* updateExtent)
{
  // Do nothing for empty datasets:
  for (int dim = 0; dim < 3; ++dim)
  {
    if (this->Extent[2 * dim] > this->Extent[2 * dim + 1])
    {
      vtkDebugMacro(<< "Refusing to crop empty dataset.");
      return;
    }
  }

  int i, j, k;
  int uExt[6];
  const int* extent = this->Extent;

  // If the update extent is larger than the extent,
  // we cannot do anything about it here.
  for (i = 0; i < 3; ++i)
  {
    uExt[i * 2] = updateExtent[i * 2];
    if (uExt[i * 2] < extent[i * 2])
    {
      uExt[i * 2] = extent[i * 2];
    }
    uExt[i * 2 + 1] = updateExtent[i * 2 + 1];
    if (uExt[i * 2 + 1] > extent[i * 2 + 1])
    {
      uExt[i * 2 + 1] = extent[i * 2 + 1];
    }
  }

  // If extents already match, then we need to do nothing.
  if (extent[0] == uExt[0] && extent[1] == uExt[1] && extent[2] == uExt[2] &&
    extent[3] == uExt[3] && extent[4] == uExt[4] && extent[5] == uExt[5])
  {
    return;
  }
  else
  {
    // Get the points.  Protect against empty data objects.
    vtkPoints* inPts = this->GetPoints();
    if (inPts == nullptr)
    {
      return;
    }

    vtkDebugMacro(<< "Cropping Grid");

    vtkStructuredGrid* newGrid = vtkStructuredGrid::New();
    vtkPointData* inPD = this->GetPointData();
    vtkCellData* inCD = this->GetCellData();
    vtkPointData* outPD = newGrid->GetPointData();
    vtkCellData* outCD = newGrid->GetCellData();

    // Allocate necessary objects
    //
    newGrid->SetExtent(uExt);
    int outSize = (uExt[1] - uExt[0] + 1) * (uExt[3] - uExt[2] + 1) * (uExt[5] - uExt[4] + 1);
    vtkPoints* newPts = inPts->NewInstance();
    newPts->SetDataType(inPts->GetDataType());
    newPts->SetNumberOfPoints(outSize);
    outPD->CopyAllocate(inPD, outSize, outSize);
    outCD->CopyAllocate(inCD, outSize, outSize);

    // Traverse this data and copy point attributes to output
    vtkIdType newId = 0;
    int inInc1 = (extent[1] - extent[0] + 1);
    int inInc2 = inInc1 * (extent[3] - extent[2] + 1);
    for (k = uExt[4]; k <= uExt[5]; ++k)
    {
      int kOffset = (k - extent[4]) * inInc2;
      for (j = uExt[2]; j <= uExt[3]; ++j)
      {
        int jOffset = (j - extent[2]) * inInc1;
        for (i = uExt[0]; i <= uExt[1]; ++i)
        {
          vtkIdType idx = (i - extent[0]) + jOffset + kOffset;
          newPts->SetPoint(newId, inPts->GetPoint(idx));
          outPD->CopyData(inPD, idx, newId++);
        }
      }
    }

    // Traverse input data and copy cell attributes to output
    newId = 0;
    inInc1 = (extent[1] - extent[0]);
    inInc2 = inInc1 * (extent[3] - extent[2]);
    for (k = uExt[4]; k < uExt[5]; ++k)
    {
      int kOffset = (k - extent[4]) * inInc2;
      for (j = uExt[2]; j < uExt[3]; ++j)
      {
        int jOffset = (j - extent[2]) * inInc1;
        for (i = uExt[0]; i < uExt[1]; ++i)
        {
          vtkIdType idx = (i - extent[0]) + jOffset + kOffset;
          outCD->CopyData(inCD, idx, newId++);
        }
      }
    }

    this->SetExtent(uExt);
    this->SetPoints(newPts);
    newPts->Delete();
    inPD->ShallowCopy(outPD);
    inCD->ShallowCopy(outCD);
    newGrid->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int dims[3];
  this->GetDimensions(dims);
  os << indent << "Dimensions: (" << dims[0] << ", " << dims[1] << ", " << dims[2] << ")\n";

  const int* extent = this->Extent;
  os << indent << "Extent: " << extent[0] << ", " << extent[1] << ", " << extent[2] << ", "
     << extent[3] << ", " << extent[4] << ", " << extent[5] << endl;

  os << ")\n";
}

//------------------------------------------------------------------------------
vtkStructuredGrid* vtkStructuredGrid::GetData(vtkInformation* info)
{
  return info ? vtkStructuredGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkStructuredGrid* vtkStructuredGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkStructuredGrid::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetPoint(int i, int j, int k, double p[3], bool adjustForExtent)
{
  int extent[6];
  this->GetExtent(extent);

  if (i < extent[0] || i > extent[1] || j < extent[2] || j > extent[3] || k < extent[4] ||
    k > extent[5])
  {
    vtkErrorMacro("ERROR: IJK coordinates are outside of grid extent!");
    return; // out of bounds!
  }

  int pos[3];
  pos[0] = i;
  pos[1] = j;
  pos[2] = k;

  vtkIdType id;

  if (adjustForExtent)
  {
    id = vtkStructuredData::ComputePointIdForExtent(extent, pos);
  }
  else
  {
    int dims[3];
    this->GetDimensions(dims);
    id = vtkStructuredData::ComputePointId(dims, pos);
  }

  this->GetPoint(id, p);
}

//------------------------------------------------------------------------------
bool vtkStructuredGrid::HasAnyBlankPoints()
{
  return this->PointData->HasAnyGhostBitSet(vtkDataSetAttributes::HIDDENPOINT);
}

//------------------------------------------------------------------------------
bool vtkStructuredGrid::HasAnyBlankCells()
{
  int cellBlanking = this->CellData->HasAnyGhostBitSet(vtkDataSetAttributes::HIDDENCELL);
  return cellBlanking || this->HasAnyBlankPoints();
}
