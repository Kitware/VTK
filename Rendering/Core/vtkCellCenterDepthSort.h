// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkCellCenterDepthSort
 * @brief   A simple implementation of vtkCellDepthSort.
 *
 *
 * vtkCellCenterDepthSort is a simple and fast implementation of depth
 * sort, but it only provides approximate results.  The sorting algorithm
 * finds the centroids of all the cells.  It then performs the dot product
 * of the centroids against a vector pointing in the direction of the
 * camera transformed into object space.  It then performs an ordinary sort
 * on the result.
 *
 */

#ifndef vtkCellCenterDepthSort_h
#define vtkCellCenterDepthSort_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkVisibilitySort.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;

class vtkCellCenterDepthSortStack;

class VTKRENDERINGCORE_EXPORT vtkCellCenterDepthSort : public vtkVisibilitySort
{
public:
  vtkTypeMacro(vtkCellCenterDepthSort, vtkVisibilitySort);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCellCenterDepthSort* New();

  void InitTraversal() override;
  vtkIdTypeArray* GetNextCells() override;

protected:
  vtkCellCenterDepthSort();
  ~vtkCellCenterDepthSort() override;

  vtkIdTypeArray* SortedCells;
  vtkIdTypeArray* SortedCellPartition;

  vtkFloatArray* CellCenters;
  vtkFloatArray* CellDepths;
  vtkFloatArray* CellPartitionDepths;

  virtual float* ComputeProjectionVector();
  virtual void ComputeCellCenters();
  virtual void ComputeDepths();

private:
  vtkCellCenterDepthSortStack* ToSort;

  vtkCellCenterDepthSort(const vtkCellCenterDepthSort&) = delete;
  void operator=(const vtkCellCenterDepthSort&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
