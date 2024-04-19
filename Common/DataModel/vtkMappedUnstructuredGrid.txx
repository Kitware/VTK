// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMappedUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkGenericCell.h"
#include <algorithm>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::PrintSelf(
  ostream& os, vtkIndent indent)
{
  os << indent << "Implementation:";
  if (this->Impl == nullptr)
  {
    os << " nullptr" << endl;
  }
  else
  {
    os << endl;
    this->Impl->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::CopyStructure(vtkDataSet* ds)
{
  if (ThisType* grid = ThisType::SafeDownCast(ds))
  {
    this->SetImplementation(grid->GetImplementation());
  }

  this->Superclass::CopyStructure(ds);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::ShallowCopy(vtkDataObject* src)
{
  if (ThisType* grid = ThisType::SafeDownCast(src))
  {
    this->SetImplementation(grid->GetImplementation());
  }

  this->Superclass::ShallowCopy(src);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkIdType vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetNumberOfCells()
{
  return this->Impl->GetNumberOfCells();
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkCell* vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->TempCell);
  return this->TempCell;
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetCell(
  vtkIdType cellId, vtkGenericCell* cell)
{
  cell->SetCellType(this->Impl->GetCellType(cellId));
  this->Impl->GetCellPoints(cellId, cell->PointIds);
  this->Points->GetPoints(cell->PointIds, cell->Points);

  if (cell->RequiresExplicitFaceRepresentation())
  {
    vtkNew<vtkCellArray> faces;
    this->Impl->GetPolyhedronFaces(cellId, faces);
    if (faces->GetNumberOfCells() != 0)
    {
      cell->SetCellFaces(faces);
    }
  }

  if (cell->RequiresInitialization())
  {
    cell->Initialize();
  }
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
int vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetCellType(vtkIdType cellId)
{
  return this->Impl->GetCellType(cellId);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetCellPoints(
  vtkIdType cellId, vtkIdList* ptIds)
{
  this->Impl->GetCellPoints(cellId, ptIds);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkCellIterator* vtkMappedUnstructuredGrid<Implementation, CellIterator>::NewCellIterator()
{
  CellIteratorType* cellIterator = CellIteratorType::New();
  cellIterator->SetMappedUnstructuredGrid(this);
  return cellIterator;
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetPointCells(
  vtkIdType ptId, vtkIdList* cellIds)
{
  this->Impl->GetPointCells(ptId, cellIds);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
int vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetMaxCellSize()
{
  return this->Impl->GetMaxCellSize();
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetIdsOfCellsOfType(
  int type, vtkIdTypeArray* array)
{
  this->Impl->GetIdsOfCellsOfType(type, array);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
int vtkMappedUnstructuredGrid<Implementation, CellIterator>::IsHomogeneous()
{
  return this->Impl->IsHomogeneous();
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::Allocate(vtkIdType numCells, int)
{
  return this->Impl->Allocate(numCells);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkIdType vtkMappedUnstructuredGrid<Implementation, CellIterator>::InternalInsertNextCell(
  int type, vtkIdList* ptIds)
{
  return this->Impl->InsertNextCell(type, ptIds);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkIdType vtkMappedUnstructuredGrid<Implementation, CellIterator>::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType ptIds[])
{
  return this->Impl->InsertNextCell(type, npts, ptIds);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkIdType vtkMappedUnstructuredGrid<Implementation, CellIterator>::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType ptIds[], vtkCellArray* faces)
{
  return this->Impl->InsertNextCell(type, npts, ptIds, faces);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::InternalReplaceCell(
  vtkIdType cellId, int npts, const vtkIdType pts[])
{
  this->Impl->ReplaceCell(cellId, npts, pts);
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkMTimeType vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetMTime()
{
  return std::max(this->MTime.GetMTime(), this->Impl->GetMTime());
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkMappedUnstructuredGrid<Implementation, CellIterator>::vtkMappedUnstructuredGrid()
  : Impl(nullptr)
{
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
vtkMappedUnstructuredGrid<Implementation, CellIterator>::~vtkMappedUnstructuredGrid() = default;

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
void vtkMappedUnstructuredGrid<Implementation, CellIterator>::SetImplementation(
  Implementation* impl)
{
  this->Impl = impl;
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Implementation, class CellIterator>
Implementation* vtkMappedUnstructuredGrid<Implementation, CellIterator>::GetImplementation()
{
  return this->Impl;
}
VTK_ABI_NAMESPACE_END
