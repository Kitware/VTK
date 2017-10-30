
#ifndef __mapped_grid_h
#define __mapped_grid_h

#include <vtkMappedUnstructuredGrid.h>

#include "MappedGridImpl.h"
#include "MappedCellIterator.h"

class MappedGrid : public vtkMappedUnstructuredGrid<MappedGridImpl, MappedCellIterator<MappedGridImpl> >
{
public:
  typedef vtkMappedUnstructuredGrid<vtkUnstructuredGrid, vtkCellIterator> _myBase;
  vtkTypeMacro(MappedGrid, _myBase);

  int GetDataObjectType() VTK_OVERRIDE { return VTK_UNSTRUCTURED_GRID_BASE; }

  static MappedGrid* New();

  vtkPoints* GetPoints() { return this->GetImplementation()->GetPoints(); }

  vtkIdType GetNumberOfPoints() { return this->GetImplementation()->GetPoints()->GetNumberOfPoints(); }

protected:
  MappedGrid()
  {
    MappedGridImpl* ig = MappedGridImpl::New();
    ig->SetOwner(this);
    this->SetImplementation(ig);
    ig->Delete();
  }
  ~MappedGrid() {}

private:
  MappedGrid(const MappedGrid&) = delete;
  void operator=(const MappedGrid&) = delete;
};

#endif // __mapped_grid_h
