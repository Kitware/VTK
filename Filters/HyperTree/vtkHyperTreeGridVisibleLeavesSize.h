// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridVisibleLeavesSize
 * @brief   This class is deprecated. Please use vtkHyperTreeGridGenerateFields
 */

#ifndef vtkHyperTreeGridVisibleLeavesSize_h
#define vtkHyperTreeGridVisibleLeavesSize_h

#include "vtkDeprecation.h"            // For VTK_DEPRECATED_IN_9_5_0
#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkHyperTreeGridGenerateFields.h"

VTK_ABI_NAMESPACE_BEGIN

class VTK_DEPRECATED_IN_9_5_0("Please use `vtkHyperTreeGridGenerateFields` instead.")
  VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridVisibleLeavesSize
  : public vtkHyperTreeGridGenerateFields
{
public:
  static vtkHyperTreeGridVisibleLeavesSize* New();
  vtkTypeMacro(vtkHyperTreeGridVisibleLeavesSize, vtkHyperTreeGridGenerateFields);

protected:
  vtkHyperTreeGridVisibleLeavesSize() = default;
  ~vtkHyperTreeGridVisibleLeavesSize() override = default;

private:
  vtkHyperTreeGridVisibleLeavesSize(const vtkHyperTreeGridVisibleLeavesSize&) = delete;
  void operator=(const vtkHyperTreeGridVisibleLeavesSize&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
