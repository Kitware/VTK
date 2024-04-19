// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetCellIterator
 * @brief   Implementation of vtkCellIterator using
 * vtkDataSet API.
 */

#ifndef vtkDataSetCellIterator_h
#define vtkDataSetCellIterator_h

#include "vtkCellIterator.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetCellIterator : public vtkCellIterator
{
public:
  static vtkDataSetCellIterator* New();
  vtkTypeMacro(vtkDataSetCellIterator, vtkCellIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;

protected:
  vtkDataSetCellIterator();
  ~vtkDataSetCellIterator() override;

  void ResetToFirstCell() override;
  void IncrementToNextCell() override;
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;

  friend class vtkDataSet;
  void SetDataSet(vtkDataSet* ds);

  vtkSmartPointer<vtkDataSet> DataSet;
  vtkIdType CellId;

private:
  vtkDataSetCellIterator(const vtkDataSetCellIterator&) = delete;
  void operator=(const vtkDataSetCellIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDataSetCellIterator_h
