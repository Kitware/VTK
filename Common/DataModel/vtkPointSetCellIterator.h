// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointSetCellIterator
 * @brief   Implementation of vtkCellIterator using
 * vtkPointSet API.
 */

#ifndef vtkPointSetCellIterator_h
#define vtkPointSetCellIterator_h

#include "vtkCellIterator.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkPointSet;

class VTKCOMMONDATAMODEL_EXPORT vtkPointSetCellIterator : public vtkCellIterator
{
public:
  static vtkPointSetCellIterator* New();
  vtkTypeMacro(vtkPointSetCellIterator, vtkCellIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;

protected:
  vtkPointSetCellIterator();
  ~vtkPointSetCellIterator() override;

  void ResetToFirstCell() override;
  void IncrementToNextCell() override;
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;

  friend class vtkPointSet;
  void SetPointSet(vtkPointSet* ds);

  vtkSmartPointer<vtkPointSet> PointSet;
  vtkSmartPointer<vtkPoints> PointSetPoints;
  vtkIdType CellId;

private:
  vtkPointSetCellIterator(const vtkPointSetCellIterator&) = delete;
  void operator=(const vtkPointSetCellIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPointSetCellIterator_h
