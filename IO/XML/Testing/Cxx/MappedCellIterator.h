#ifndef __vtkMappedGridIterator_h
#define __vtkMappedGridIterator_h


#include <vtkCellIterator.h>

#ifndef VTK_DELETE_FUNCTION
#define VTK_DELETE_FUNCTION
#endif

template <class I>
class MappedCellIterator : public vtkCellIterator
{
public:
  vtkTemplateTypeMacro(MappedCellIterator<I>, vtkCellIterator);
  typedef MappedCellIterator<I> ThisType;

  static MappedCellIterator<I>* New();

  void SetMappedUnstructuredGrid(vtkMappedUnstructuredGrid<I, ThisType> *grid);

  virtual void PrintSelf(std::ostream& os, vtkIndent id);

  virtual bool IsDoneWithTraversal();
  virtual vtkIdType GetCellId();

protected:
  MappedCellIterator();
  ~MappedCellIterator();
  virtual void ResetToFirstCell() { this->CellId = 0; }
  virtual void IncrementToNextCell() { this->CellId++; }
  virtual void FetchCellType();
  virtual void FetchPointIds();
  virtual void FetchPoints();
  virtual void FetchFaces();

private:
  MappedCellIterator(const MappedCellIterator&) VTK_DELETE_FUNCTION;
  void operator=(const MappedCellIterator&) VTK_DELETE_FUNCTION;

  vtkIdType CellId;
  vtkIdType NumberOfCells;
  vtkSmartPointer<I> Impl;
  vtkSmartPointer<vtkPoints> GridPoints;
};

#include "MappedCellIterator.txx"

#endif // __vtkMappedGridIterator_h
