
#include "MappedGrid.h"
#include "MappedGridImpl.h"

#include <vtkUnstructuredGrid.h>

using namespace std;

vtkStandardNewMacro(MappedGrid)
vtkStandardNewMacro(MappedGridImpl)

void
MappedGridImpl::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "Mapped Grid Implementation" << endl;
}

int
MappedGridImpl::GetCellType(vtkIdType cellId)
{
  return _grid->GetCellType(cellId);
}

int
MappedGridImpl::GetMaxCellSize()
{
  return _grid->GetMaxCellSize();
}

void
MappedGridImpl::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  _grid->GetCellPoints(cellId, ptIds);
}

void
MappedGridImpl::GetFaceStream(vtkIdType cellId, vtkIdList *ptIds)
{
  _grid->GetFaceStream(cellId, ptIds);
}

void
MappedGridImpl::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  _grid->GetPointCells(ptId, cellIds);
}

int
MappedGridImpl::IsHomogeneous()
{
  return _grid->IsHomogeneous();
}

vtkIdType
MappedGridImpl::GetNumberOfCells()
{
  return _grid->GetNumberOfCells();
}

void
MappedGridImpl::GetIdsOfCellsOfType(int type, vtkIdTypeArray *array)
{
  _grid->GetIdsOfCellsOfType(type, array);
}

void
MappedGridImpl::Allocate(vtkIdType numCells, int extSize)
{
  vtkWarningMacro(<<"Read only block\n");
  return;
}


vtkIdType
MappedGridImpl::InsertNextCell(int type, vtkIdList *ptIds)
{
  vtkWarningMacro(<<"Read only block\n");
  return -1;
}

vtkIdType
MappedGridImpl::InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds)
{
  vtkWarningMacro(<<"Read only block\n");
  return -1;
}

vtkIdType
MappedGridImpl::InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds,
    vtkIdType nfaces, vtkIdType *faces)
{
  vtkWarningMacro(<<"Read only block\n");
  return -1;
}

void
MappedGridImpl::ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts)
{
  vtkWarningMacro(<<"Read only block\n");
  return;
}
