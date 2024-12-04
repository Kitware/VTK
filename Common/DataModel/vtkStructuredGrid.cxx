// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStructuredGrid.h"

#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredCellArray.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStructuredGrid);
vtkStandardExtendedNewMacro(vtkStructuredGrid);

//------------------------------------------------------------------------------
vtkStructuredGrid::vtkStructuredGrid()
{
  this->DataDescription = VTK_EMPTY;

  for (int idx = 0; idx < 3; ++idx)
  {
    this->Dimensions[idx] = 0;
  }

  const int extent[6] = { 0, -1, 0, -1, 0, -1 };
  memcpy(this->Extent, extent, sizeof(extent));

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//------------------------------------------------------------------------------
vtkStructuredGrid::~vtkStructuredGrid() = default;

//------------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured grid.
void vtkStructuredGrid::CopyStructure(vtkDataSet* ds)
{
  vtkStructuredGrid* sg = static_cast<vtkStructuredGrid*>(ds);
  this->Superclass::CopyStructure(ds);

  // set extent sets, extent, dimensions, and data description
  this->SetExtent(sg->Extent);

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
void vtkStructuredGrid::BuildCells()
{
  this->StructuredCells = vtkStructuredData::GetCellArray(this->Extent, false);
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::BuildCellTypes()
{
  this->StructuredCellTypes = vtkStructuredData::GetCellTypesArray(this->Extent, false);
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::BuildImplicitStructures()
{
  this->BuildCells();
  this->BuildCellTypes();
}

//------------------------------------------------------------------------------
vtkCell* vtkStructuredGrid::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->GenericCell);
  return this->GenericCell->GetRepresentativeCell();
}

//------------------------------------------------------------------------------
vtkCell* vtkStructuredGrid::GetCell(int i, int j, int k)
{
  int ijkMin[3] = { i, j, k };
  const auto cellId = vtkStructuredData::ComputeCellId(this->Dimensions, ijkMin);
  return this->GetCell(cellId);
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

  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    cell->SetCellTypeToEmptyCell();
    return;
  }

  cell->SetCellType(this->StructuredCellTypes->GetValue(cellId));
  this->StructuredCells->GetCellAtId(cellId, cell->PointIds);
  this->Points->GetPoints(cell->PointIds, cell->Points);
}

//------------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkStructuredGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  vtkIdType npts, pts[8];
  this->StructuredCells->GetCellAtId(cellId, npts, pts);
  vtkBoundingBox::ComputeBounds(this->Points, pts, npts, bounds);
}

//------------------------------------------------------------------------------
int vtkStructuredGrid::GetCellType(vtkIdType cellId)
{
  // see whether the cell is blanked
  return this->IsCellVisible(cellId) ? this->StructuredCellTypes->GetValue(cellId) : VTK_EMPTY_CELL;
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredGrid::GetCellSize(vtkIdType cellId)
{
  // see whether the cell is blanked
  return this->IsCellVisible(cellId) ? this->StructuredCells->GetCellSize(cellId) : 0;
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts,
  vtkIdList* ptIds) VTK_SIZEHINT(pts, npts)
{
  this->StructuredCells->GetCellAtId(cellId, npts, pts, ptIds);
}

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  this->StructuredCells->GetCellAtId(cellId, ptIds);
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
void vtkStructuredGrid::GetCellNeighbors(
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
vtkStructuredCellArray* vtkStructuredGrid::GetCells()
{
  return this->StructuredCells;
}

//------------------------------------------------------------------------------
vtkConstantArray<int>* vtkStructuredGrid::GetCellTypesArray()
{
  return this->StructuredCellTypes;
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
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkStructuredGrid::IsCellVisible(vtkIdType cellId)
{
  return vtkStructuredData::IsCellVisible(cellId, this->Dimensions, this->DataDescription,
    this->GetCellGhostArray(), this->GetPointGhostArray());
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

//------------------------------------------------------------------------------
void vtkStructuredGrid::GetCellDims(int cellDims[3])
{
  for (int i = 0; i < 3; ++i)
  {
    cellDims[i] = ((this->Dimensions[i] - 1) < 1) ? 1 : this->Dimensions[i] - 1;
  }
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

  vtkStructuredData::GetDimensionsFromExtent(extent, this->Dimensions);

  this->DataDescription = description;

  this->BuildImplicitStructures();
  this->Modified();
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
void vtkStructuredGrid::GetDimensions(int dims[3])
{
  const int* extent = this->Extent;
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
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
  // set extent sets, extent, dimensions, and data description
  this->SetExtent(src->Extent);
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

  const int* dims = this->Dimensions;
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
  if (i < this->Extent[0] || i > this->Extent[1] || j < this->Extent[2] || j > this->Extent[3] ||
    k < this->Extent[4] || k > this->Extent[5])
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
    id = vtkStructuredData::ComputePointIdForExtent(this->Extent, pos);
  }
  else
  {
    id = vtkStructuredData::ComputePointId(this->Dimensions, pos);
  }

  this->GetPoint(id, p);
}

VTK_ABI_NAMESPACE_END
