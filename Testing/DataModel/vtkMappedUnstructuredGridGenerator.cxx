// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*----------------------------------------------------------------------------
  This test was written by Menno Deij - van Rijswijk (MARIN).
----------------------------------------------------------------------------*/

#include "vtkMappedUnstructuredGridGenerator.h"

#include "vtkCell.h" // for cell types
#include "vtkCellArray.h"
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMappedUnstructuredGrid.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace
{ // this namespace contains the supporting mapped grid definition used in the test

template <class I>
class MappedCellIterator : public vtkCellIterator
{
public:
  vtkTemplateTypeMacro(MappedCellIterator<I>, vtkCellIterator);
  typedef MappedCellIterator<I> ThisType;

  static MappedCellIterator<I>* New();

  void SetMappedUnstructuredGrid(vtkMappedUnstructuredGrid<I, ThisType>* grid);

  void PrintSelf(std::ostream& os, vtkIndent id) override;

  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;

protected:
  MappedCellIterator();
  ~MappedCellIterator() override;
  void ResetToFirstCell() override { this->CellId = 0; }
  void IncrementToNextCell() override { this->CellId++; }
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;
  void FetchFaces() override;

private:
  MappedCellIterator(const MappedCellIterator&) = delete;
  void operator=(const MappedCellIterator&) = delete;

  vtkIdType CellId;
  vtkIdType NumberOfCells;
  vtkSmartPointer<I> Impl;
  vtkSmartPointer<vtkPoints> GridPoints;
};

template <class I>
MappedCellIterator<I>* MappedCellIterator<I>::New()
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

template <class I>
void MappedCellIterator<I>::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Mapped Internal Block" << endl;
}

template <class I>
MappedCellIterator<I>::~MappedCellIterator() = default;

template <class I>
void MappedCellIterator<I>::SetMappedUnstructuredGrid(vtkMappedUnstructuredGrid<I, ThisType>* grid)
{
  this->Impl = grid->GetImplementation();
  this->CellId = 0;
  this->GridPoints = grid->GetPoints();
  this->NumberOfCells = grid->GetNumberOfCells();
}

template <class I>
bool MappedCellIterator<I>::IsDoneWithTraversal()
{
  if (!this->Impl)
    return true;
  return CellId >= this->NumberOfCells;
}

template <class I>
vtkIdType MappedCellIterator<I>::GetCellId()
{
  return this->CellId;
}

template <class I>
void MappedCellIterator<I>::FetchCellType()
{
  this->CellType = Impl->GetCellType(this->CellId);
}

template <class I>
void MappedCellIterator<I>::FetchPointIds()
{
  this->Impl->GetCellPoints(this->CellId, this->PointIds);
}

template <class I>
void MappedCellIterator<I>::FetchPoints()
{
  this->GridPoints->GetPoints(this->GetPointIds(), this->Points);
}

template <class I>
void MappedCellIterator<I>::FetchFaces()
{
  this->Impl->GetPolyhedronFaces(this->CellId, this->Faces);
}

class MappedGridImpl : public vtkObject
{
public:
  static MappedGridImpl* New();

  void Initialize(vtkUnstructuredGrid* ug)
  {
    ug->Register(this);
    Owner->SetPoints(ug->GetPoints());
    _grid = ug;
  }

  void PrintSelf(std::ostream& os, vtkIndent id) override;

  // API for vtkMappedUnstructuredGrid implementation
  virtual int GetCellType(vtkIdType cellId);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds);
  virtual void GetFaceStream(vtkIdType cellId, vtkIdList* ptIds);
  virtual void GetPolyhedronFaces(vtkIdType cellId, vtkCellArray* ptIds);
  virtual void GetPointCells(vtkIdType ptId, vtkIdList* cellIds);
  virtual int GetMaxCellSize();
  virtual void GetIdsOfCellsOfType(int type, vtkIdTypeArray* array);
  virtual int IsHomogeneous();

  // This container is read only -- these methods do nothing but print a warning.
  void Allocate(vtkIdType numCells, int extSize = 1000);
  vtkIdType InsertNextCell(int type, vtkIdList* ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[])
    VTK_SIZEHINT(ptIds, npts);
  vtkIdType InsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[], vtkCellArray* faces)
    VTK_SIZEHINT(ptIds, npts);
  void ReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);

  vtkIdType GetNumberOfCells();
  void SetOwner(vtkPointSet* owner) { this->Owner = owner; }

  vtkPoints* GetPoints() { return _grid->GetPoints(); }

protected:
  MappedGridImpl() = default;
  ~MappedGridImpl() override { _grid->UnRegister(this); }

private:
  vtkUnstructuredGrid* _grid;
  vtkPointSet* Owner;
};

vtkStandardNewMacro(MappedGridImpl);

void MappedGridImpl::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Mapped Grid Implementation" << endl;
}

int MappedGridImpl::GetCellType(vtkIdType cellId)
{
  return _grid->GetCellType(cellId);
}

int MappedGridImpl::GetMaxCellSize()
{
  return _grid->GetMaxCellSize();
}

void MappedGridImpl::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  _grid->GetCellPoints(cellId, ptIds);
}

void MappedGridImpl::GetFaceStream(vtkIdType cellId, vtkIdList* ptIds)
{
  _grid->GetFaceStream(cellId, ptIds);
}

void MappedGridImpl::GetPolyhedronFaces(vtkIdType cellId, vtkCellArray* faces)
{
  _grid->GetPolyhedronFaces(cellId, faces);
}

void MappedGridImpl::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  _grid->GetPointCells(ptId, cellIds);
}

int MappedGridImpl::IsHomogeneous()
{
  return _grid->IsHomogeneous();
}

vtkIdType MappedGridImpl::GetNumberOfCells()
{
  return _grid->GetNumberOfCells();
}

void MappedGridImpl::GetIdsOfCellsOfType(int type, vtkIdTypeArray* array)
{
  _grid->GetIdsOfCellsOfType(type, array);
}

void MappedGridImpl::Allocate(vtkIdType vtkNotUsed(numCells), int vtkNotUsed(extSize))
{
  vtkWarningMacro(<< "Read only block\n");
}

vtkIdType MappedGridImpl::InsertNextCell(int vtkNotUsed(type), vtkIdList* vtkNotUsed(ptIds))
{
  vtkWarningMacro(<< "Read only block\n");
  return -1;
}

vtkIdType MappedGridImpl::InsertNextCell(
  int vtkNotUsed(type), vtkIdType vtkNotUsed(npts), const vtkIdType vtkNotUsed(ptIds)[])
{
  vtkWarningMacro(<< "Read only block\n");
  return -1;
}

vtkIdType MappedGridImpl::InsertNextCell(int vtkNotUsed(type), vtkIdType vtkNotUsed(npts),
  const vtkIdType vtkNotUsed(ptIds)[], vtkCellArray* vtkNotUsed(nfaces))
{
  vtkWarningMacro(<< "Read only block\n");
  return -1;
}

void MappedGridImpl::ReplaceCell(
  vtkIdType vtkNotUsed(cellId), int vtkNotUsed(npts), const vtkIdType vtkNotUsed(pts)[])
{
  vtkWarningMacro(<< "Read only block\n");
}

class MappedGrid
  : public vtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl>>
{
public:
  typedef vtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl>> _myBase;

  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNSTRUCTURED_GRID_BASE; }

  static MappedGrid* New();

  vtkPoints* GetPoints() override { return this->GetImplementation()->GetPoints(); }

  vtkIdType GetNumberOfPoints() override
  {
    return this->GetImplementation()->GetPoints()->GetNumberOfPoints();
  }

protected:
  MappedGrid()
  {
    MappedGridImpl* ig = MappedGridImpl::New();
    ig->SetOwner(this);
    this->SetImplementation(ig);
    ig->Delete();
  }
  ~MappedGrid() override = default;

private:
  MappedGrid(const MappedGrid&) = delete;
  void operator=(const MappedGrid&) = delete;
};

vtkStandardNewMacro(MappedGrid);

} // end anonymous namespace

VTK_ABI_NAMESPACE_BEGIN

void vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(vtkUnstructuredGrid** grid)
{
  vtkNew<vtkPoints> points;

  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(0, 1, 0);

  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(1, 0, 1);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(0, 1, 1);

  points->InsertNextPoint(.5, .5, 2);
  points->InsertNextPoint(.5, .5, -1);

  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
  ug->SetPoints(points);

  ug->Allocate(3); // allocate for 3 cells

  vtkNew<vtkIdList> ids;

  // add a hexahedron of the first 8 points (i.e. a cube)
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ug->InsertNextCell(VTK_HEXAHEDRON, ids.GetPointer());
  ids->Reset();

  // add a polyhedron comprise of the top hexahedron face
  // and four triangles to the 9th point
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ids->InsertNextId(8);

  vtkNew<vtkCellArray> faces;
  // top face of four points
  faces->InsertNextCell(4);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(7);

  // four triangle side faces, each of three points
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(8);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(8);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(8);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(8);

  // insert the polyhedron cell
  ug->InsertNextCell(VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), faces);

  // put another pyramid on the bottom towards the 10th point
  faces->Reset();
  ids->Reset();

  // the list of points that the pyramid references
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(9);

  // bottom face of four points
  faces->InsertNextCell(4);

  faces->InsertCellPoint(0);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(3);

  // four side faces, each of three points
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(9);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(9);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(9);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), faces);

  *grid = ug;
}

void vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(
  vtkUnstructuredGridBase** grid)
{
  vtkUnstructuredGrid* ug;
  GenerateUnstructuredGrid(&ug);

  // create a mapped grid which basically takes the original grid
  // and uses it to map to.
  MappedGrid* mg = MappedGrid::New();
  mg->GetImplementation()->Initialize(ug);
  ug->Delete(); // the mapped grid holds the only reference to ug

  *grid = mg;
}

vtkMappedUnstructuredGridGenerator* vtkMappedUnstructuredGridGenerator::New()
{
  return new vtkMappedUnstructuredGridGenerator;
}

void vtkMappedUnstructuredGridGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkMappedUnstructuredGridGenerator object";
}
VTK_ABI_NAMESPACE_END
