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
  this->XCoordinates = vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples(1);
  this->XCoordinates->SetComponent(0, 0, 0.0);

  this->YCoordinates = vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples(1);
  this->YCoordinates->SetComponent(0, 0, 0.0);

  this->ZCoordinates = vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples(1);
  this->ZCoordinates->SetComponent(0, 0, 0.0);
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

  this->Superclass::CopyStructure(ds);

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
  this->Cleanup();
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetPoint(int i, int j, int k, double p[3])
{
  int ijk[3] = { i, j, k };
  const vtkIdType pntIdx = this->ComputePointId(ijk);
  this->GetPoint(pntIdx, p);
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::BuildPoints()
{
  static double identityMatrix[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
  if (this->XCoordinates && this->YCoordinates && this->ZCoordinates)
  {
    this->SetStructuredPoints(vtkStructuredData::GetPoints(this->XCoordinates, this->YCoordinates,
      this->ZCoordinates, this->GetExtent(), identityMatrix));
  }
}

//------------------------------------------------------------------------------
void vtkRectilinearGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    cell->SetCellTypeToEmptyCell();
    return;
  }
  // set cell type
  cell->SetCellType(this->GetCellTypesArray()->GetValue(cellId));

  // get min max ijk
  int ijkMin[3], ijkMax[3];
  vtkStructuredData::ComputeCellStructuredMinMaxCoords(
    cellId, this->GetDimensions(), ijkMin, ijkMax, this->GetDataDescription());

  // set cell point ids
  vtkIdType cellSize;
  this->GetCells()->GetCellAtId(ijkMin, cellSize, cell->PointIds->GetPointer(0));

  // set cell points
  vtkPoints* points = this->GetPoints();
  const auto pointsBackend =
    static_cast<vtkStructuredPointArray<double>*>(points->GetData())->GetBackend();
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
  if (this->GetCells()->GetCellSize(cellId) == 0)
  {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5] = 0.0;
    return;
  }
  int ijkMin[3], ijkMax[3];
  vtkStructuredData::ComputeCellStructuredMinMaxCoords(
    cellId, this->GetDimensions(), ijkMin, ijkMax, this->GetDataDescription());

  vtkPoints* points = this->GetPoints();
  const auto pointsBackend =
    static_cast<vtkStructuredPointArray<double>*>(points->GetData())->GetBackend();
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

//------------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The cell is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the grid, and a 1 if inside the grid.
int vtkRectilinearGrid::ComputeStructuredCoordinates(
  const double x[3], int ijk[3], double pcoords[3])
{
  int i, j;
  double xPrev, xNext, tmp;
  vtkDataArray* scalars[3];

  int dims[3];
  this->GetDimensions(dims);

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
    if (x[j] == xNext && dims[j] != 1)
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
  const int* extent = this->GetExtent();

  // Do nothing for empty datasets:
  for (int dim = 0; dim < 3; ++dim)
  {
    if (extent[2 * dim] > extent[2 * dim + 1])
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

  // If the update extent is larger than the extent,
  // we cannot do anything about it here.
  for (i = 0; i < 3; ++i)
  {
    uExt[i * 2] = updateExtent[i * 2];
    ext[i * 2] = extent[i * 2];
    uExt[i * 2] = std::max(uExt[i * 2], ext[i * 2]);
    uExt[i * 2 + 1] = updateExtent[i * 2 + 1];
    ext[i * 2 + 1] = extent[i * 2 + 1];
    uExt[i * 2 + 1] = std::min(uExt[i * 2 + 1], ext[i * 2 + 1]);
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

  os << indent << "X Coordinates: " << this->XCoordinates << "\n";
  os << indent << "Y Coordinates: " << this->YCoordinates << "\n";
  os << indent << "Z Coordinates: " << this->ZCoordinates << "\n";
}

VTK_ABI_NAMESPACE_END
