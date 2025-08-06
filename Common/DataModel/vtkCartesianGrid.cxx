// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCartesianGrid.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkConstantArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredCellArray.h"
#include "vtkStructuredPointArray.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetSmartPointerMacro(vtkCartesianGrid, StructuredPoints, vtkPoints);

//------------------------------------------------------------------------------
namespace
{
class CellVisibility
{
public:
  CellVisibility(vtkCartesianGrid* input)
    : Input(input)
  {
  }
  bool operator()(const vtkIdType id) { return !Input->IsCellVisible(id); }

private:
  vtkCartesianGrid* Input;
};
} // anonymous namespace

//------------------------------------------------------------------------------
vtkCartesianGrid::vtkCartesianGrid()
{
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataDescription: " << this->DataDescription << "\n";

  const int* dims = this->Dimensions;
  os << indent << "Dimensions: (" << dims[0] << ", " << dims[1] << ", " << dims[2] << ")\n";

  const int* extent = this->Extent;
  os << indent << "Extent: (" << extent[0];
  for (int idx = 1; idx < 6; ++idx)
  {
    os << ", " << extent[idx];
  }
  os << ")\n";
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::ShallowCopy(vtkDataObject* dataObject)
{
  vtkCartesianGrid* grid = vtkCartesianGrid::SafeDownCast(dataObject);
  if (grid)
  {
    // set extent sets, extent, dimensions, and data description
    this->SetExtent(grid->Extent);
  }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::DeepCopy(vtkDataObject* dataObject)
{
  vtkCartesianGrid* grid = vtkCartesianGrid::SafeDownCast(dataObject);
  if (grid)
  {
    // set extent sets, extent, dimensions, and data description
    this->SetExtent(grid->Extent);
  }

  // Do superclass
  this->Superclass::DeepCopy(dataObject);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::CopyStructure(vtkDataSet* ds)
{
  vtkCartesianGrid* cg = vtkCartesianGrid::SafeDownCast(ds);
  this->SetExtent(cg->Extent);
}

//------------------------------------------------------------------------------
vtkPoints* vtkCartesianGrid::GetPoints()
{
  if (this->StructuredPoints == nullptr)
  {
    this->BuildPoints();
  }
  return this->StructuredPoints.Get();
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetPoint(vtkIdType ptId, double x[3])
{
  vtkPoints* points = this->GetPoints();
  static_cast<vtkStructuredPointArray<double>*>(points->GetData())->GetTypedTuple(ptId, x);
}

//------------------------------------------------------------------------------
int vtkCartesianGrid::GetCellType(vtkIdType cellId)
{
  // see whether the cell is blanked
  return this->IsCellVisible(cellId) ? this->StructuredCellTypes->GetValue(cellId) : VTK_EMPTY_CELL;
}

//------------------------------------------------------------------------------
vtkIdType vtkCartesianGrid::GetCellSize(vtkIdType cellId)
{
  // see whether the cell is blanked
  return this->IsCellVisible(cellId) ? this->StructuredCells->GetCellSize(cellId) : 0;
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts,
  vtkIdList* ptIds) VTK_SIZEHINT(pts, npts)
{
  this->StructuredCells->GetCellAtId(cellId, npts, pts, ptIds);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  this->StructuredCells->GetCellAtId(cellId, ptIds);
}

//------------------------------------------------------------------------------
vtkIdType vtkCartesianGrid::FindCell(double x[3], vtkCell* vtkNotUsed(cell),
  vtkGenericCell* vtkNotUsed(gencell), vtkIdType vtkNotUsed(cellId), double tol2, int& subId,
  double pcoords[3], double* weights)
{
  return this->FindCell(x, nullptr, 0, tol2, subId, pcoords, weights);
}

//------------------------------------------------------------------------------
vtkCell* vtkCartesianGrid::FindAndGetCell(double x[3], vtkCell* vtkNotUsed(cell),
  vtkIdType vtkNotUsed(cellId), double tol2, int& subId, double pcoords[3], double* weights)
{
  const vtkIdType cellId = this->FindCell(x, nullptr, 0, tol2, subId, pcoords, nullptr);

  if (cellId < 0)
  {
    return nullptr;
  }

  vtkCell* cell = this->GetCell(cellId);
  cell->InterpolateFunctions(pcoords, weights);

  return cell;
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  this->GetCellNeighbors(cellId, ptIds, cellIds, nullptr);
}

//------------------------------------------------------------------------------
vtkCell* vtkCartesianGrid::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->GenericCell);
  return this->GenericCell->GetRepresentativeCell();
}

//------------------------------------------------------------------------------
vtkCell* vtkCartesianGrid::GetCell(int iMin, int jMin, int kMin)
{
  int ijkMin[3] = { iMin, jMin, kMin };
  const auto cellId = vtkStructuredData::ComputeCellId(this->Dimensions, ijkMin);
  return this->GetCell(cellId);
}

//------------------------------------------------------------------------------
bool vtkCartesianGrid::HasAnyBlankPoints()
{
  return this->PointData->HasAnyGhostBitSet(vtkDataSetAttributes::HIDDENPOINT);
}

//------------------------------------------------------------------------------
bool vtkCartesianGrid::HasAnyBlankCells()
{
  int cellBlanking = this->CellData->HasAnyGhostBitSet(vtkDataSetAttributes::HIDDENCELL);
  return cellBlanking || this->HasAnyBlankPoints();
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::Initialize()
{
  this->Superclass::Initialize();
  if (this->Information)
  {
    this->SetDimensions(0, 0, 0);
  }
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetCellNeighbors(
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
      seedLoc ? vtkStructuredData::GetCellNeighbors(
                  cellId, ptIds, cellIds, this->GetDimensions(), seedLoc)
              : vtkStructuredData::GetCellNeighbors(cellId, ptIds, cellIds, this->GetDimensions());
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
vtkStructuredCellArray* vtkCartesianGrid::GetCells()
{
  return this->StructuredCells;
}

//------------------------------------------------------------------------------
vtkConstantArray<int>* vtkCartesianGrid::GetCellTypesArray()
{
  return this->StructuredCellTypes;
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetCellDims(int cellDims[3])
{
  for (int i = 0; i < 3; ++i)
  {
    cellDims[i] = ((this->Dimensions[i] - 1) < 1) ? 1 : this->Dimensions[i] - 1;
  }
}

//------------------------------------------------------------------------------
// Turn off a particular data point.
void vtkCartesianGrid::BlankPoint(vtkIdType ptId)
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
void vtkCartesianGrid::BlankPoint(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputePointId(this->GetDimensions(), ijk);
  this->BlankPoint(idx);
}

//------------------------------------------------------------------------------
// Turn on a particular data point.
void vtkCartesianGrid::UnBlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if (!ghosts)
  {
    return;
  }
  ghosts->SetValue(ptId, ghosts->GetValue(ptId) & ~vtkDataSetAttributes::HIDDENPOINT);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::UnBlankPoint(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputePointId(this->GetDimensions(), ijk);
  this->UnBlankPoint(idx);
}

//------------------------------------------------------------------------------
// Turn off a particular data cell.
void vtkCartesianGrid::BlankCell(vtkIdType cellId)
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
void vtkCartesianGrid::BlankCell(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputeCellId(this->GetDimensions(), ijk);
  assert("cell id in range:" && ((idx >= 0) && (idx < this->GetNumberOfCells())));
  this->BlankCell(idx);
}

//------------------------------------------------------------------------------
// Turn on a particular data cell.
void vtkCartesianGrid::UnBlankCell(vtkIdType cellId)
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
void vtkCartesianGrid::UnBlankCell(int i, int j, int k)
{
  int ijk[3] = { i, j, k };
  const int idx = vtkStructuredData::ComputeCellId(this->GetDimensions(), ijk);
  assert("cell id in range:" && ((idx >= 0) && (idx < this->GetNumberOfCells())));
  this->UnBlankCell(idx);
}

//------------------------------------------------------------------------------
unsigned char vtkCartesianGrid::IsPointVisible(vtkIdType pointId)
{
  return vtkStructuredData::IsPointVisible(pointId, this->GetPointGhostArray());
}

//------------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkCartesianGrid::IsCellVisible(vtkIdType cellId)
{
  return vtkStructuredData::IsCellVisible(cellId, this->GetDimensions(), this->GetDataDescription(),
    this->GetCellGhostArray(), this->GetPointGhostArray());
}

//------------------------------------------------------------------------------
int* vtkCartesianGrid::GetDimensions()
{
  this->GetDimensions(this->Dimensions);
  return this->Dimensions;
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::GetDimensions(int* dOut)
{
  const int* extent = this->Extent;
  dOut[0] = extent[1] - extent[0] + 1;
  dOut[1] = extent[3] - extent[2] + 1;
  dOut[2] = extent[5] - extent[4] + 1;
}

#if VTK_ID_TYPE_IMPL != VTK_INT
//------------------------------------------------------------------------------
void vtkCartesianGrid::GetDimensions(vtkIdType dims[3])
{
  // Use vtkIdType to avoid overflow on large images
  const int* extent = this->Extent;
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
}
#endif

//------------------------------------------------------------------------------
void vtkCartesianGrid::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6];
  ext[0] = x1;
  ext[1] = x2;
  ext[2] = y1;
  ext[3] = y2;
  ext[4] = z1;
  ext[5] = z2;
  this->SetExtent(ext);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::SetExtent(int* extent)
{
  int description;

  description = vtkStructuredData::SetExtent(extent, this->Extent);
  if (description < 0) // improperly specified
  {
    vtkErrorMacro("Bad Extent, retaining previous values");
  }

  if (description == vtkStructuredData::VTK_STRUCTURED_UNCHANGED)
  {
    return;
  }

  vtkStructuredData::GetDimensionsFromExtent(extent, this->GetDimensions());

  this->DataDescription = description;

  this->BuildImplicitStructures();

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::SetScalarType(int type, vtkInformation* meta_data)
{
  vtkDataObject::SetPointDataActiveScalarInfo(meta_data, type, -1);
}

//------------------------------------------------------------------------------
int vtkCartesianGrid::GetScalarType()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (!scalars)
  {
    return VTK_DOUBLE;
  }
  return scalars->GetDataType();
}

//------------------------------------------------------------------------------
bool vtkCartesianGrid::HasScalarType(vtkInformation* meta_data)
{
  vtkInformation* scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data, FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!scalarInfo)
  {
    return false;
  }

  return scalarInfo->Has(FIELD_ARRAY_TYPE()) != 0;
}

//------------------------------------------------------------------------------
int vtkCartesianGrid::GetScalarType(vtkInformation* meta_data)
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
void vtkCartesianGrid::SetNumberOfScalarComponents(int num, vtkInformation* meta_data)
{
  vtkDataObject::SetPointDataActiveScalarInfo(meta_data, -1, num);
}

//----------------------------------------------------------------------------
bool vtkCartesianGrid::HasNumberOfScalarComponents(vtkInformation* meta_data)
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
int vtkCartesianGrid::GetNumberOfScalarComponents(vtkInformation* meta_data)
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
int vtkCartesianGrid::GetNumberOfScalarComponents()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (scalars)
  {
    return scalars->GetNumberOfComponents();
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::BuildImplicitStructures()
{
  this->BuildPoints();
  this->BuildCells();
  this->BuildCellTypes();
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::BuildCells()
{
  this->StructuredCells = vtkStructuredData::GetCellArray(this->GetExtent(), true);
}

//------------------------------------------------------------------------------
void vtkCartesianGrid::BuildCellTypes()
{
  this->StructuredCellTypes = vtkStructuredData::GetCellTypesArray(this->GetExtent(), true);
}

VTK_ABI_NAMESPACE_END
