#include "MappedCellIterator.h"
#include <vtkPoints.h>

template <class I>
MappedCellIterator<I>*
MappedCellIterator<I>::New()
{
  VTK_STANDARD_NEW_BODY(ThisType);
}

template <class I>
MappedCellIterator<I>::MappedCellIterator()
  : CellId(0)
  , NumberOfCells(0)
  , Impl(nullptr)
  , GridPoints(nullptr)
{
}

template <class I> void MappedCellIterator<I>
::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "Mapped Internal Block" << endl;
}

template <class I>
MappedCellIterator<I>::~MappedCellIterator()
{
}

template <class I> void
MappedCellIterator<I>::SetMappedUnstructuredGrid(vtkMappedUnstructuredGrid<I, ThisType> *grid)
{
  this->Impl = grid->GetImplementation();
  this->CellId = 0;
  this->GridPoints = grid->GetPoints();
  this->NumberOfCells = grid->GetNumberOfCells();
}

template <class I> bool
MappedCellIterator<I>::IsDoneWithTraversal()
{
  if (!this->Impl) return true;
  return CellId >= this->NumberOfCells;
}

template <class I> vtkIdType
MappedCellIterator<I>::GetCellId()
{
  return this->CellId;
}

template <class I> void
MappedCellIterator<I>::FetchCellType()
{
  this->CellType = Impl->GetCellType(this->CellId);
}

template <class I> void
MappedCellIterator<I>::FetchPointIds()
{
  this->Impl->GetCellPoints(this->CellId, this->PointIds);
}

template <class I> void
MappedCellIterator<I>::FetchPoints()
{
  this->GridPoints->GetPoints(this->GetPointIds(), this->Points);
}

template <class I> void
MappedCellIterator<I>::FetchFaces()
{
  this->Impl->GetFaceStream(this->CellId, this->Faces);
}
