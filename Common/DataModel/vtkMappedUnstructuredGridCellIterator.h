// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkMappedUnstructuredGridCellIterator
 * @brief   Default cell iterator for
 * vtkMappedUnstructuredGrid.
 *
 *
 * This class is used by default for vtkMappedUnstructedGrid instances. It
 * uses random access for data lookups. Custom vtkCellIterator implementations
 * should be used instead when random-access is inefficient.
 */

#ifndef vtkMappedUnstructuredGridCellIterator_h
#define vtkMappedUnstructuredGridCellIterator_h

#include "vtkCellIterator.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
template <class Implementation, class CellIterator>
class vtkMappedUnstructuredGrid;

template <class Implementation>
class vtkMappedUnstructuredGridCellIterator : public vtkCellIterator
{
public:
  vtkTemplateTypeMacro(vtkMappedUnstructuredGridCellIterator<Implementation>, vtkCellIterator);
  typedef Implementation ImplementationType;
  typedef vtkMappedUnstructuredGridCellIterator<ImplementationType> ThisType;
  static vtkMappedUnstructuredGridCellIterator<ImplementationType>* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetMappedUnstructuredGrid(vtkMappedUnstructuredGrid<ImplementationType, ThisType>* grid);

  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;

protected:
  vtkMappedUnstructuredGridCellIterator();
  ~vtkMappedUnstructuredGridCellIterator() override;

  void ResetToFirstCell() override;
  void IncrementToNextCell() override;
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;

private:
  vtkMappedUnstructuredGridCellIterator(const vtkMappedUnstructuredGridCellIterator&) = delete;
  void operator=(const vtkMappedUnstructuredGridCellIterator&) = delete;

  vtkSmartPointer<ImplementationType> Impl;
  vtkSmartPointer<vtkPoints> GridPoints;
  vtkIdType CellId;
  vtkIdType NumberOfCells;
};

VTK_ABI_NAMESPACE_END
#include "vtkMappedUnstructuredGridCellIterator.txx" // template implementations

#endif // vtkMappedUnstructuredGridCellIterator_h

// VTK-HeaderTest-Exclude: vtkMappedUnstructuredGridCellIterator.h
