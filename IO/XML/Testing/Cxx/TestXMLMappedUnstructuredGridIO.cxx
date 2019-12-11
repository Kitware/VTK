/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLMappedUnstructuredGridIO

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
  This test was written by Menno Deij - van Rijswijk (MARIN).
----------------------------------------------------------------------------*/

#include "vtkCell.h" // for cell types
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMappedUnstructuredGrid.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtksys/FStream.hxx"

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
  this->Impl->GetFaceStream(this->CellId, this->Faces);
}

class MappedGrid;
class MappedGridImpl : public vtkObject
{
public:
  static MappedGridImpl* New();

  void Initialize(vtkUnstructuredGrid* ug)
  {
    ug->Register(this);
    _grid = ug;
  }

  void PrintSelf(std::ostream& os, vtkIndent id) override;

  // API for vtkMappedUnstructuredGrid implementation
  virtual int GetCellType(vtkIdType cellId);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds);
  virtual void GetFaceStream(vtkIdType cellId, vtkIdList* ptIds);
  virtual void GetPointCells(vtkIdType ptId, vtkIdList* cellIds);
  virtual int GetMaxCellSize();
  virtual void GetIdsOfCellsOfType(int type, vtkIdTypeArray* array);
  virtual int IsHomogeneous();

  // This container is read only -- these methods do nothing but print a warning.
  void Allocate(vtkIdType numCells, int extSize = 1000);
  vtkIdType InsertNextCell(int type, vtkIdList* ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[])
    VTK_SIZEHINT(ptIds, npts);
  vtkIdType InsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[], vtkIdType nfaces,
    const vtkIdType faces[]) VTK_SIZEHINT(ptIds, npts) VTK_SIZEHINT(faces, nfaces);
  void ReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);

  vtkIdType GetNumberOfCells();
  void SetOwner(MappedGrid* owner) { this->Owner = owner; }

  vtkPoints* GetPoints() { return _grid->GetPoints(); }

protected:
  MappedGridImpl() = default;
  ~MappedGridImpl() override { _grid->UnRegister(this); }

private:
  vtkUnstructuredGrid* _grid;
  MappedGrid* Owner;
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
  const vtkIdType vtkNotUsed(ptIds)[], vtkIdType vtkNotUsed(nfaces),
  const vtkIdType vtkNotUsed(faces)[])
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
  : public vtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl> >
{
public:
  typedef vtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl> > _myBase;

  int GetDataObjectType() override { return VTK_UNSTRUCTURED_GRID_BASE; }

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

using namespace std;

bool compareFiles(const string& p1, const string& p2)
{
  vtksys::ifstream f1(p1.c_str(), ifstream::binary | ifstream::ate);
  vtksys::ifstream f2(p2.c_str(), ifstream::binary | ifstream::ate);

  if (f1.fail() || f2.fail())
  {
    return false; // file problem
  }

  if (f1.tellg() != f2.tellg())
  {
    return false; // size mismatch
  }

  // seek back to beginning and use equal to compare contents
  f1.seekg(0, vtksys::ifstream::beg);
  f2.seekg(0, vtksys::ifstream::beg);
  return equal(istreambuf_iterator<char>(f1.rdbuf()), istreambuf_iterator<char>(),
    istreambuf_iterator<char>(f2.rdbuf()));
}

int TestXMLMappedUnstructuredGridIO(int argc, char* argv[])
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

  vtkNew<vtkUnstructuredGrid> ug;
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

  vtkNew<vtkIdList> faces;
  // top face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(7);

  // four triangle side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(6);
  faces->InsertNextId(7);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(7);
  faces->InsertNextId(4);
  faces->InsertNextId(8);

  // insert the polyhedron cell
  ug->InsertNextCell(
    VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

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
  faces->InsertNextId(4);

  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(3);

  // four side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(2);
  faces->InsertNextId(3);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(
    VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  // for testing, we write in appended, ascii and binary mode and request that
  // the files are ** binary ** equal.
  //
  // first, find a file we can write to

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  string dir(tempDir);
  if (dir.empty())
  {
    cerr << "Could not determine temporary directory." << endl;
    return EXIT_FAILURE;
  }

  string f1 = dir + "/test_ug_input.vtu";
  string f2 = dir + "/test_mapped_input.vtu";

  vtkNew<vtkXMLUnstructuredGridWriter> w;
  w->SetInputData(ug);
  w->SetFileName(f1.c_str());

  w->Update();
  if (points->GetData()->GetInformation()->Has(vtkDataArray::L2_NORM_RANGE()))
  {
    // for the normal unstructured grid the L2_NORM_RANGE is added. This
    // makes file comparison impossible. therefore, after the first Update()
    // remove the L2_NORM_RANGE information key and write the file again.
    points->GetData()->GetInformation()->Remove(vtkDataArray::L2_NORM_RANGE());
  }
  w->Update();

  // create a mapped grid which basically takes the original grid
  // and uses it to map to.
  vtkNew<MappedGrid> mg;
  mg->GetImplementation()->Initialize(ug);

  vtkNew<vtkXMLUnstructuredGridWriter> w2;
  w2->SetInputData(mg);
  w2->SetFileName(f2.c_str());
  w2->Update();

  // compare the files in appended, then ascii, then binary mode.
  bool same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in appended mode.\n";
    return EXIT_FAILURE;
  }
  w->SetDataModeToAscii();
  w2->SetDataModeToAscii();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in ascii mode.\n";
    return EXIT_FAILURE;
  }
  w->SetDataModeToBinary();
  w2->SetDataModeToBinary();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in binary mode.\n";
    return EXIT_FAILURE;
  }

  // clean up after ourselves: remove written files and free temp dir name
  remove(f1.c_str());
  remove(f2.c_str());

  delete[] tempDir;

  return 0;
}
