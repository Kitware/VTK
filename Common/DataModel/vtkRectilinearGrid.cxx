// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#define VTK_DEPRECATION_LEVEL 0

#include "vtkRectilinearGrid.h"

#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredCellArray.h"
#include "vtkStructuredPointArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVoxel.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRectilinearGrid);
vtkStandardExtendedNewMacro(vtkRectilinearGrid);

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetXCoordinates(vtkDataArray* xCoords)
{
  auto time = this->GetMTime();
  vtkSetObjectBodyMacro(XCoordinates, vtkDataArray, xCoords);
  if (this->GetMTime() > time)
  {
    this->BuildPoints();
  }
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetYCoordinates(vtkDataArray* yCoords)
{
  auto time = this->GetMTime();
  vtkSetObjectBodyMacro(YCoordinates, vtkDataArray, yCoords);
  if (this->GetMTime() > time)
  {
    this->BuildPoints();
  }
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetZCoordinates(vtkDataArray* zCoords)
{
  auto time = this->GetMTime();
  vtkSetObjectBodyMacro(ZCoordinates, vtkDataArray, zCoords);
  if (this->GetMTime() > time)
  {
    this->BuildPoints();
  }
}

//------------------------------------------------------------------------------
vtkRectilinearGrid::vtkRectilinearGrid()
{
  this->DataDescription = VTK_EMPTY;

  for (int idx = 0; idx < 3; ++idx)
  {
    this->Dimensions[idx] = 0;
    this->Point[idx] = 0;
  }

  this->XCoordinates = vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples(1);
  this->XCoordinates->SetComponent(0, 0, 0.0);

  this->YCoordinates = vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples(1);
  this->YCoordinates->SetComponent(0, 0, 0.0);

  this->ZCoordinates = vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples(1);
  this->ZCoordinates->SetComponent(0, 0, 0.0);

  int extent[6] = { 0, -1, 0, -1, 0, -1 };
  memcpy(this->Extent, extent, 6 * sizeof(int));

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//------------------------------------------------------------------------------
vtkRectilinearGrid::~vtkRectilinearGrid()
{
  this->Cleanup();
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::Cleanup()
{
  if (this->XCoordinates)
  {
    this->XCoordinates->UnRegister(this);
    this->XCoordinates = nullptr;
  }

  if (this->YCoordinates)
  {
    this->YCoordinates->UnRegister(this);
    this->YCoordinates = nullptr;
  }

  if (this->ZCoordinates)
  {
    this->ZCoordinates->UnRegister(this);
    this->ZCoordinates = nullptr;
  }
}

//------------------------------------------------------------------------------
// Copy the geometric and topological structure of an input rectilinear grid
// object.
void vtkRectilinearGrid::CopyStructure(vtkDataSet* ds)
{
  vtkRectilinearGrid* rGrid = static_cast<vtkRectilinearGrid*>(ds);
  this->Initialize();

  // set extent sets, extent, dimensions, and data description
  this->SetExtent(rGrid->Extent);

  this->SetXCoordinates(rGrid->XCoordinates);
  this->SetYCoordinates(rGrid->YCoordinates);
  this->SetZCoordinates(rGrid->ZCoordinates);

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
void vtkRectilinearGrid::Initialize()
{
  this->Superclass::Initialize();

  if (this->Information)
  {
    this->SetDimensions(0, 0, 0);
  }

  this->Cleanup();
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetPoint(vtkIdType ptId, double x[3])
{
  static_cast<vtkStructuredPointArray<double>*>(this->StructuredPoints->GetData())
    ->GetTypedTuple(ptId, x);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetPoint(int i, int j, int k, double p[3])
{
  int ijk[3] = { i, j, k };
  const vtkIdType pntIdx = this->ComputePointId(ijk);
  this->GetPoint(pntIdx, p);
}

//------------------------------------------------------------------------------
vtkPoints* vtkRectilinearGrid::GetPoints()
{
  if (!this->StructuredPoints)
  {
    this->BuildPoints();
  }
  return this->StructuredPoints.Get();
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BuildPoints()
{
  static double identityMatrix[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
  if (this->XCoordinates && this->YCoordinates && this->ZCoordinates)
  {
    this->StructuredPoints = vtkStructuredData::GetPoints(
      this->XCoordinates, this->YCoordinates, this->ZCoordinates, this->Extent, identityMatrix);
  }
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BuildCells()
{
  this->StructuredCells = vtkStructuredData::GetCellArray(this->Extent, true);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BuildCellTypes()
{
  this->StructuredCellTypes = vtkStructuredData::GetCellTypesArray(this->Extent, true);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BuildImplicitStructures()
{
  this->BuildPoints();
  this->BuildCells();
  this->BuildCellTypes();
}

//------------------------------------------------------------------------------
vtkCell* vtkRectilinearGrid::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->GenericCell);
  return this->GenericCell->GetRepresentativeCell();
}

//------------------------------------------------------------------------------
vtkCell* vtkRectilinearGrid::GetCell(int iMin, int jMin, int kMin)
{
  int ijkMin[3] = { iMin, jMin, kMin };
  const auto cellId = vtkStructuredData::ComputeCellId(this->Dimensions, ijkMin);
  return this->GetCell(cellId);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  // Make sure data is defined
  if (!this->StructuredPoints)
  {
    vtkErrorMacro(<< "No data");
    return;
  }
  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    cell->SetCellTypeToEmptyCell();
    return;
  }
  // set cell type
  cell->SetCellType(this->StructuredCellTypes->GetValue(cellId));

  // get min max ijk
  int ijkMin[3], ijkMax[3];
  vtkStructuredData::ComputeCellStructuredMinMaxCoords(
    cellId, this->Dimensions, ijkMin, ijkMax, this->DataDescription);

  // set cell point ids
  vtkIdType cellSize;
  this->StructuredCells->GetCellAtId(ijkMin, cellSize, cell->PointIds->GetPointer(0));

  // set cell points
  const auto pointsBackend =
    static_cast<vtkStructuredPointArray<double>*>(this->StructuredPoints->GetData())->GetBackend();
  int loc[3], npts = 0;
  double point[3];
  for (loc[2] = ijkMin[2]; loc[2] <= ijkMax[2]; loc[2]++)
  {
    point[2] = pointsBackend->mapStructuredZComponent(loc[2]);
    for (loc[1] = ijkMin[1]; loc[1] <= ijkMax[1]; loc[1]++)
    {
      point[1] = pointsBackend->mapStructuredYComponent(loc[1]);
      for (loc[0] = ijkMin[0]; loc[0] <= ijkMax[0]; loc[0]++)
      {
        point[0] = pointsBackend->mapStructuredXComponent(loc[0]);
        cell->Points->SetPoint(npts++, point);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkRectilinearGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  if (this->StructuredCells->GetCellSize(cellId) == 0)
  {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5] = 0.0;
    return;
  }
  int ijkMin[3], ijkMax[3];
  vtkStructuredData::ComputeCellStructuredMinMaxCoords(
    cellId, this->Dimensions, ijkMin, ijkMax, this->DataDescription);

  const auto pointsBackend =
    static_cast<vtkStructuredPointArray<double>*>(this->StructuredPoints->GetData())->GetBackend();
  int loc[3];
  double point[3];
  bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
  bounds[1] = bounds[3] = bounds[5] = VTK_DOUBLE_MIN;
  for (loc[2] = ijkMin[2]; loc[2] <= ijkMax[2]; loc[2]++)
  {
    point[2] = pointsBackend->mapStructuredZComponent(loc[2]);
    bounds[4] = std::min(bounds[4], point[2]);
    bounds[5] = std::max(bounds[5], point[2]);
  }
  for (loc[1] = ijkMin[1]; loc[1] <= ijkMax[1]; loc[1]++)
  {
    point[1] = pointsBackend->mapStructuredYComponent(loc[1]);
    bounds[2] = std::min(bounds[2], point[1]);
    bounds[3] = std::max(bounds[3], point[1]);
  }
  for (loc[0] = ijkMin[0]; loc[0] <= ijkMax[0]; loc[0]++)
  {
    point[0] = pointsBackend->mapStructuredXComponent(loc[0]);
    bounds[0] = std::min(bounds[0], point[0]);
    bounds[1] = std::max(bounds[1], point[0]);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkRectilinearGrid::FindPoint(double x[3])
{
  int i, j, loc[3];
  double xPrev, xNext;
  vtkDataArray* scalars[3];

  scalars[0] = this->XCoordinates;
  scalars[1] = this->YCoordinates;
  scalars[2] = this->ZCoordinates;
  //
  // Find coordinates in x-y-z direction
  //
  for (j = 0; j < 3; j++)
  {
    loc[j] = 0;
    xPrev = scalars[j]->GetComponent(0, 0);
    xNext = scalars[j]->GetComponent(scalars[j]->GetNumberOfTuples() - 1, 0);
    if (x[j] < xPrev || x[j] > xNext)
    {
      return -1;
    }

    for (i = 1; i < scalars[j]->GetNumberOfTuples(); i++)
    {
      xNext = scalars[j]->GetComponent(i, 0);
      if (x[j] >= xPrev && x[j] <= xNext)
      {
        if ((x[j] - xPrev) < (xNext - x[j]))
        {
          loc[j] = i - 1;
        }
        else
        {
          loc[j] = i;
        }
      }
      xPrev = xNext;
    }
  }
  //
  //  From this location get the point id
  //
  return this->ComputePointId(loc);
}

vtkIdType vtkRectilinearGrid::FindCell(double x[3], vtkCell* vtkNotUsed(cell),
  vtkGenericCell* vtkNotUsed(gencell), vtkIdType vtkNotUsed(cellId), double vtkNotUsed(tol2),
  int& subId, double pcoords[3], double* weights)
{
  return this->FindCell(x, static_cast<vtkCell*>(nullptr), 0, 0.0, subId, pcoords, weights);
}

//------------------------------------------------------------------------------
vtkIdType vtkRectilinearGrid::FindCell(double x[3], vtkCell* vtkNotUsed(cell),
  vtkIdType vtkNotUsed(cellId), double vtkNotUsed(tol2), int& subId, double pcoords[3],
  double* weights)
{
  int loc[3];

  if (this->ComputeStructuredCoordinates(x, loc, pcoords) == 0)
  {
    return -1;
  }

  if (weights)
  {
    vtkVoxel::InterpolationFunctions(pcoords, weights);
  }

  //
  //  From this location get the cell id
  //
  subId = 0;
  const vtkIdType cellId = this->ComputeCellId(loc);
  if (!this->IsCellVisible(cellId))
  {
    return -1;
  }
  return cellId;
}

//------------------------------------------------------------------------------
vtkCell* vtkRectilinearGrid::FindAndGetCell(double x[3], vtkCell* vtkNotUsed(cell),
  vtkIdType vtkNotUsed(cellId), double vtkNotUsed(tol2), int& subId, double pcoords[3],
  double* weights)
{
  const vtkIdType cellId = this->FindCell(x, nullptr, 0, 0, subId, pcoords, nullptr);

  if (cellId < 0)
  {
    return nullptr;
  }

  vtkCell* cell = this->GetCell(cellId);
  cell->InterpolateFunctions(pcoords, weights);

  return cell;
}

//------------------------------------------------------------------------------
int vtkRectilinearGrid::GetCellType(vtkIdType cellId)
{
  // see whether the cell is blanked
  return this->IsCellVisible(cellId) ? this->StructuredCellTypes->GetValue(cellId) : VTK_EMPTY_CELL;
}

//------------------------------------------------------------------------------
vtkIdType vtkRectilinearGrid::GetCellSize(vtkIdType cellId)
{
  // see whether the cell is blanked
  return this->IsCellVisible(cellId) ? this->StructuredCells->GetCellSize(cellId) : 0;
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts,
  vtkIdList* ptIds) VTK_SIZEHINT(pts, npts)
{
  this->StructuredCells->GetCellAtId(cellId, npts, pts, ptIds);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  this->StructuredCells->GetCellAtId(cellId, ptIds);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::ComputeBounds()
{
  double tmp;

  if (this->XCoordinates == nullptr || this->YCoordinates == nullptr ||
    this->ZCoordinates == nullptr)
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
  }

  if (this->XCoordinates->GetNumberOfTuples() == 0 ||
    this->YCoordinates->GetNumberOfTuples() == 0 || this->ZCoordinates->GetNumberOfTuples() == 0)
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
  }

  this->Bounds[0] = this->XCoordinates->GetComponent(0, 0);
  this->Bounds[2] = this->YCoordinates->GetComponent(0, 0);
  this->Bounds[4] = this->ZCoordinates->GetComponent(0, 0);

  this->Bounds[1] =
    this->XCoordinates->GetComponent(this->XCoordinates->GetNumberOfTuples() - 1, 0);
  this->Bounds[3] =
    this->YCoordinates->GetComponent(this->YCoordinates->GetNumberOfTuples() - 1, 0);
  this->Bounds[5] =
    this->ZCoordinates->GetComponent(this->ZCoordinates->GetNumberOfTuples() - 1, 0);
  // ensure that the bounds are increasing
  for (int i = 0; i < 5; i += 2)
  {
    if (this->Bounds[i + 1] < this->Bounds[i])
    {
      tmp = this->Bounds[i + 1];
      this->Bounds[i + 1] = this->Bounds[i];
      this->Bounds[i] = tmp;
    }
  }
}

namespace
{
class CellVisibility
{
public:
  CellVisibility(vtkRectilinearGrid* input)
    : Input(input)
  {
  }
  bool operator()(const vtkIdType id) { return !Input->IsCellVisible(id); }

private:
  vtkRectilinearGrid* Input;
};
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  int numPtIds = ptIds->GetNumberOfIds();

  // Use special methods for speed
  switch (numPtIds)
  {
    case 0:
      cellIds->Reset();
      return;

    case 1:
    case 2:
    case 4: // vertex, edge, face neighbors
      vtkStructuredData::GetCellNeighbors(cellId, ptIds, cellIds, this->Dimensions);
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
void vtkRectilinearGrid::GetCellNeighbors(
  vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds, int* seedLoc)
{
  int numPtIds = ptIds->GetNumberOfIds();

  // Use special methods for speed
  switch (numPtIds)
  {
    case 0:
      cellIds->Reset();
      return;

    case 1:
    case 2:
    case 4: // vertex, edge, face neighbors
      vtkStructuredData::GetCellNeighbors(cellId, ptIds, cellIds, this->Dimensions, seedLoc);
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
vtkStructuredCellArray* vtkRectilinearGrid::GetCells()
{
  return this->StructuredCells;
}

//------------------------------------------------------------------------------
vtkConstantArray<int>* vtkRectilinearGrid::GetCellTypesArray()
{
  return this->StructuredCellTypes;
}

//------------------------------------------------------------------------------
// Turn off a particular data point.
void vtkRectilinearGrid::BlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if (!ghosts)
  {
    this->AllocatePointGhostArray();
    ghosts = this->GetPointGhostArray();
  }
  ghosts->SetValue(ptId, ghosts->GetValue(ptId) | vtkDataSetAttributes::HIDDENPOINT);
  assert(!this->IsPointVisible(ptId));
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BlankPoint(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputePointId(this->Dimensions, ijk);
  this->BlankPoint(idx);
}

//------------------------------------------------------------------------------
// Turn on a particular data point.
void vtkRectilinearGrid::UnBlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if (!ghosts)
  {
    return;
  }
  ghosts->SetValue(ptId, ghosts->GetValue(ptId) & ~vtkDataSetAttributes::HIDDENPOINT);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::UnBlankPoint(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputePointId(this->Dimensions, ijk);
  this->UnBlankPoint(idx);
}

//------------------------------------------------------------------------------
// Turn off a particular data cell.
void vtkRectilinearGrid::BlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghost = this->GetCellGhostArray();
  if (!ghost)
  {
    this->AllocateCellGhostArray();
    ghost = this->GetCellGhostArray();
  }
  ghost->SetValue(cellId, ghost->GetValue(cellId) | vtkDataSetAttributes::HIDDENCELL);
  assert(!this->IsCellVisible(cellId));
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BlankCell(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputeCellId(this->Dimensions, ijk);
  assert("cell id in range:" && ((idx >= 0) && (idx < this->GetNumberOfCells())));
  this->BlankCell(idx);
}

//------------------------------------------------------------------------------
// Turn on a particular data cell.
void vtkRectilinearGrid::UnBlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  if (!ghosts)
  {
    return;
  }
  ghosts->SetValue(cellId, ghosts->GetValue(cellId) & ~vtkDataSetAttributes::HIDDENCELL);
  assert(this->IsCellVisible(cellId));
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::UnBlankCell(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputeCellId(this->Dimensions, ijk);
  assert("cell id in range:" && ((idx >= 0) && (idx < this->GetNumberOfCells())));
  this->UnBlankCell(idx);
}

//------------------------------------------------------------------------------
unsigned char vtkRectilinearGrid::IsPointVisible(vtkIdType pointId)
{
  return vtkStructuredData::IsPointVisible(pointId, this->GetPointGhostArray());
}

//------------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkRectilinearGrid::IsCellVisible(vtkIdType cellId)
{
  return vtkStructuredData::IsCellVisible(cellId, this->Dimensions, this->DataDescription,
    this->GetCellGhostArray(), this->GetPointGhostArray());
}

//------------------------------------------------------------------------------
bool vtkRectilinearGrid::HasAnyBlankPoints()
{
  return this->PointData->HasAnyGhostBitSet(vtkDataSetAttributes::HIDDENPOINT);
}

//------------------------------------------------------------------------------
bool vtkRectilinearGrid::HasAnyBlankCells()
{
  int cellBlanking = this->CellData->HasAnyGhostBitSet(vtkDataSetAttributes::HIDDENCELL);
  return cellBlanking || this->HasAnyBlankPoints();
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetCellDims(int cellDims[3])
{
  for (int i = 0; i < 3; ++i)
  {
    cellDims[i] = ((this->Dimensions[i] - 1) < 1) ? 1 : this->Dimensions[i] - 1;
  }
}

//------------------------------------------------------------------------------
// Set dimensions of rectilinear grid dataset.
void vtkRectilinearGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i - 1, 0, j - 1, 0, k - 1);
}

//------------------------------------------------------------------------------
// Set dimensions of rectilinear grid dataset.
void vtkRectilinearGrid::SetDimensions(const int dim[3])
{
  this->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::SetExtent(int extent[6])
{
  int description;

  description = vtkStructuredData::SetExtent(extent, this->Extent);
  if (description < 0) // improperly specified
  {
    vtkErrorMacro(<< "Bad Extent, retaining previous values");
  }

  if (description == VTK_UNCHANGED)
  {
    return;
  }

  this->DataDescription = description;

  vtkStructuredData::GetDimensionsFromExtent(extent, this->Dimensions);

  this->BuildImplicitStructures();

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::SetExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax)
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
// Convenience function computes the structured coordinates for a point x[3].
// The cell is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the grid, and a 1 if inside the grid.
int vtkRectilinearGrid::ComputeStructuredCoordinates(double x[3], int ijk[3], double pcoords[3])
{
  int i, j;
  double xPrev, xNext, tmp;
  vtkDataArray* scalars[3];

  scalars[0] = this->XCoordinates;
  scalars[1] = this->YCoordinates;
  scalars[2] = this->ZCoordinates;
  //
  // Find locations in x-y-z direction
  //
  ijk[0] = ijk[1] = ijk[2] = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  for (j = 0; j < 3; j++)
  {
    xPrev = scalars[j]->GetComponent(0, 0);
    xNext = scalars[j]->GetComponent(scalars[j]->GetNumberOfTuples() - 1, 0);
    if (xNext < xPrev)
    {
      tmp = xNext;
      xNext = xPrev;
      xPrev = tmp;
    }
    if (x[j] < xPrev || x[j] > xNext)
    {
      return 0;
    }
    if (x[j] == xNext && this->Dimensions[j] != 1)
    {
      return 0;
    }

    for (i = 1; i < scalars[j]->GetNumberOfTuples(); i++)
    {
      xNext = scalars[j]->GetComponent(i, 0);
      if (x[j] >= xPrev && x[j] < xNext)
      {
        ijk[j] = i - 1;
        pcoords[j] = (x[j] - xPrev) / (xNext - xPrev);
        break;
      }

      else if (x[j] == xNext)
      {
        ijk[j] = i - 1;
        pcoords[j] = 1.0;
        break;
      }
      xPrev = xNext;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
unsigned long vtkRectilinearGrid::GetActualMemorySize()
{
  unsigned long size = this->Superclass::GetActualMemorySize();

  if (this->XCoordinates)
  {
    size += this->XCoordinates->GetActualMemorySize();
  }

  if (this->YCoordinates)
  {
    size += this->YCoordinates->GetActualMemorySize();
  }

  if (this->ZCoordinates)
  {
    size += this->ZCoordinates->GetActualMemorySize();
  }

  return size;
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::ShallowCopy(vtkDataObject* dataObject)
{
  vtkRectilinearGrid* grid = vtkRectilinearGrid::SafeDownCast(dataObject);

  if (grid != nullptr)
  {
    // set extent sets, extent, dimensions, and data description
    this->SetExtent(grid->Extent);

    this->SetXCoordinates(grid->GetXCoordinates());
    this->SetYCoordinates(grid->GetYCoordinates());
    this->SetZCoordinates(grid->GetZCoordinates());
  }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::DeepCopy(vtkDataObject* dataObject)
{
  auto mkhold = vtkMemkindRAII(this->GetIsInMemkind());
  vtkRectilinearGrid* grid = vtkRectilinearGrid::SafeDownCast(dataObject);

  if (grid != nullptr)
  {
    // set extent sets, extent, dimensions, and data description
    this->SetExtent(grid->Extent);

    vtkDoubleArray* s = vtkDoubleArray::New();
    s->DeepCopy(grid->GetXCoordinates());
    this->SetXCoordinates(s);
    s->Delete();
    s = vtkDoubleArray::New();
    s->DeepCopy(grid->GetYCoordinates());
    this->SetYCoordinates(s);
    s->Delete();
    s = vtkDoubleArray::New();
    s->DeepCopy(grid->GetZCoordinates());
    this->SetZCoordinates(s);
    s->Delete();
  }

  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::Crop(const int* updateExtent)
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
  // What we want.
  int uExt[6];
  // What we have.
  int ext[6];
  const int* extent = this->Extent;

  // If the update extent is larger than the extent,
  // we cannot do anything about it here.
  for (i = 0; i < 3; ++i)
  {
    uExt[i * 2] = updateExtent[i * 2];
    ext[i * 2] = extent[i * 2];
    if (uExt[i * 2] < ext[i * 2])
    {
      uExt[i * 2] = ext[i * 2];
    }
    uExt[i * 2 + 1] = updateExtent[i * 2 + 1];
    ext[i * 2 + 1] = extent[i * 2 + 1];
    if (uExt[i * 2 + 1] > ext[i * 2 + 1])
    {
      uExt[i * 2 + 1] = ext[i * 2 + 1];
    }
  }

  // If extents already match, then we need to do nothing.
  if (ext[0] == uExt[0] && ext[1] == uExt[1] && ext[2] == uExt[2] && ext[3] == uExt[3] &&
    ext[4] == uExt[4] && ext[5] == uExt[5])
  {
    return;
  }
  // Invalid extents would lead to unpleasant results:
  else if (ext[1] < ext[0] || ext[3] < ext[2] || ext[5] < ext[4] || uExt[1] < uExt[0] ||
    uExt[3] < uExt[2] || uExt[5] < uExt[4])
  {
    return;
  }
  else
  {
    vtkRectilinearGrid* newGrid;
    vtkPointData *inPD, *outPD;
    vtkCellData *inCD, *outCD;
    int outSize, jOffset, kOffset;
    vtkIdType idx, newId;
    int inInc1, inInc2;
    vtkDataArray *coords, *newCoords;

    vtkDebugMacro(<< "Cropping Grid");

    newGrid = vtkRectilinearGrid::New();

    inPD = this->GetPointData();
    inCD = this->GetCellData();
    outPD = newGrid->GetPointData();
    outCD = newGrid->GetCellData();

    // Allocate necessary objects
    //
    newGrid->SetExtent(uExt);
    outSize = (uExt[1] - uExt[0] + 1) * (uExt[3] - uExt[2] + 1) * (uExt[5] - uExt[4] + 1);
    outPD->CopyAllocate(inPD, outSize, outSize);
    outCD->CopyAllocate(inCD, outSize, outSize);

    // Create the coordinate arrays.
    // X
    coords = this->GetXCoordinates();
    newCoords = coords->NewInstance();
    newCoords->SetNumberOfComponents(coords->GetNumberOfComponents());
    newCoords->SetNumberOfTuples(uExt[1] - uExt[0] + 1);
    for (idx = uExt[0]; idx <= uExt[1]; ++idx)
    {
      newCoords->InsertComponent(idx - static_cast<vtkIdType>(uExt[0]), 0,
        coords->GetComponent(idx - static_cast<vtkIdType>(ext[0]), 0));
    }
    newGrid->SetXCoordinates(newCoords);
    newCoords->Delete();
    // Y
    coords = this->GetYCoordinates();
    newCoords = coords->NewInstance();
    newCoords->SetNumberOfComponents(coords->GetNumberOfComponents());
    newCoords->SetNumberOfTuples(uExt[3] - uExt[2] + 1);
    for (idx = uExt[2]; idx <= uExt[3]; ++idx)
    {
      newCoords->InsertComponent(idx - static_cast<vtkIdType>(uExt[2]), 0,
        coords->GetComponent(idx - static_cast<vtkIdType>(ext[2]), 0));
    }
    newGrid->SetYCoordinates(newCoords);
    newCoords->Delete();
    // Z
    coords = this->GetZCoordinates();
    newCoords = coords->NewInstance();
    newCoords->SetNumberOfComponents(coords->GetNumberOfComponents());
    newCoords->SetNumberOfTuples(uExt[5] - uExt[4] + 1);
    for (idx = uExt[4]; idx <= uExt[5]; ++idx)
    {
      newCoords->InsertComponent(idx - static_cast<vtkIdType>(uExt[4]), 0,
        coords->GetComponent(idx - static_cast<vtkIdType>(ext[4]), 0));
    }
    newGrid->SetZCoordinates(newCoords);
    newCoords->Delete();

    // Traverse this data and copy point attributes to output
    newId = 0;
    inInc1 = (extent[1] - extent[0] + 1);
    inInc2 = inInc1 * (extent[3] - extent[2] + 1);
    for (k = uExt[4]; k <= uExt[5]; ++k)
    {
      kOffset = (k - extent[4]) * inInc2;
      for (j = uExt[2]; j <= uExt[3]; ++j)
      {
        jOffset = (j - extent[2]) * inInc1;
        for (i = uExt[0]; i <= uExt[1]; ++i)
        {
          idx = (i - extent[0]) + jOffset + kOffset;
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
      kOffset = (k - extent[4]) * inInc2;
      for (j = uExt[2]; j < uExt[3]; ++j)
      {
        jOffset = (j - extent[2]) * inInc1;
        for (i = uExt[0]; i < uExt[1]; ++i)
        {
          idx = (i - extent[0]) + jOffset + kOffset;
          outCD->CopyData(inCD, idx, newId++);
        }
      }
    }

    this->SetExtent(uExt);
    this->SetXCoordinates(newGrid->GetXCoordinates());
    this->SetYCoordinates(newGrid->GetYCoordinates());
    this->SetZCoordinates(newGrid->GetZCoordinates());
    inPD->ShallowCopy(outPD);
    inCD->ShallowCopy(outCD);
    newGrid->Delete();
  }
}

//------------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGrid::GetData(vtkInformation* info)
{
  return info ? vtkRectilinearGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkRectilinearGrid::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", " << this->Dimensions[1] << ", "
     << this->Dimensions[2] << ")\n";

  os << indent << "X Coordinates: " << this->XCoordinates << "\n";
  os << indent << "Y Coordinates: " << this->YCoordinates << "\n";
  os << indent << "Z Coordinates: " << this->ZCoordinates << "\n";

  const int* extent = this->Extent;
  os << indent << "Extent: " << extent[0] << ", " << extent[1] << ", " << extent[2] << ", "
     << extent[3] << ", " << extent[4] << ", " << extent[5] << endl;
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetScalarType(int type, vtkInformation* meta_data)
{
  vtkDataObject::SetPointDataActiveScalarInfo(meta_data, type, -1);
}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::GetScalarType()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (!scalars)
  {
    return VTK_DOUBLE;
  }
  return scalars->GetDataType();
}

//----------------------------------------------------------------------------
bool vtkRectilinearGrid::HasScalarType(vtkInformation* meta_data)
{
  vtkInformation* scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data, FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!scalarInfo)
  {
    return false;
  }

  return scalarInfo->Has(FIELD_ARRAY_TYPE()) != 0;
}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::GetScalarType(vtkInformation* meta_data)
{
  vtkInformation* scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data, FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (scalarInfo)
  {
    return scalarInfo->Get(FIELD_ARRAY_TYPE());
  }
  return VTK_DOUBLE;
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetNumberOfScalarComponents(int num, vtkInformation* meta_data)
{
  vtkDataObject::SetPointDataActiveScalarInfo(meta_data, -1, num);
}

//----------------------------------------------------------------------------
bool vtkRectilinearGrid::HasNumberOfScalarComponents(vtkInformation* meta_data)
{
  vtkInformation* scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data, FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!scalarInfo)
  {
    return false;
  }
  return scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()) != 0;
}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::GetNumberOfScalarComponents(vtkInformation* meta_data)
{
  vtkInformation* scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data, FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (scalarInfo && scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()))
  {
    return scalarInfo->Get(FIELD_NUMBER_OF_COMPONENTS());
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::GetNumberOfScalarComponents()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (scalars)
  {
    return scalars->GetNumberOfComponents();
  }
  return 1;
}
VTK_ABI_NAMESPACE_END
