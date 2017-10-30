
#ifndef __mapped_grid_impl_h
#define __mapped_grid_impl_h

#include <vtkUnstructuredGrid.h>
#include <stdio.h>

class MappedGrid;

class MappedGridImpl : public vtkObject
{
public:
  static MappedGridImpl* New();
  vtkTypeMacro(MappedGridImpl, vtkObject);

  void Initialize(vtkUnstructuredGrid* ug) { ug->Register(this); _grid = ug; }

  void PrintSelf(std::ostream& os, vtkIndent id);

  //API for vtkMappedUnstructuredGrid implementation
  virtual int GetCellType(vtkIdType cellId);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  virtual void GetFaceStream(vtkIdType cellId, vtkIdList *ptIds);
  virtual void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);
  virtual int GetMaxCellSize();
  virtual void GetIdsOfCellsOfType(int type, vtkIdTypeArray *array);
  virtual int IsHomogeneous();

  // This container is read only -- these methods do nothing but print a warning.
  void Allocate(vtkIdType numCells, int extSize = 1000);
  vtkIdType InsertNextCell(int type, vtkIdList *ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds,
                           vtkIdType nfaces, vtkIdType *faces);
  void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts);

  vtkIdType GetNumberOfCells();
  void SetOwner(MappedGrid* owner) { this->Owner = owner; }

  vtkPoints* GetPoints() { return _grid->GetPoints(); }

protected:
  MappedGridImpl(){}
  virtual ~MappedGridImpl() { _grid->UnRegister(this); }

private:
  vtkUnstructuredGrid* _grid;

  MappedGrid* Owner;

};


#endif // __mapped_grid_impl_h
