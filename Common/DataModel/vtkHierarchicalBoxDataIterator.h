// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHierarchicalBoxDataIterator
 * @brief   Empty class for backwards compatibility.
 *
 * @deprecated vtkHierarchicalBoxDataIterator is deprecated in VTK 9.2 and will be removed.
 * Use `vtkUniformGridAMRDataIterator` instead of `vtkHierarchicalBoxDataIterator`.
 */

#ifndef vtkHierarchicalBoxDataIterator_h
#define vtkHierarchicalBoxDataIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // For VTK_DEPRECATED_IN_9_2_0
#include "vtkUniformGridAMRDataIterator.h"

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_2_0(
  "Use vtkUniformGridAMRDataIterator instead of vtkHierarchicalBoxDataIterator")
  VTKCOMMONDATAMODEL_EXPORT vtkHierarchicalBoxDataIterator : public vtkUniformGridAMRDataIterator
{
public:
  static vtkHierarchicalBoxDataIterator* New();
  vtkTypeMacro(vtkHierarchicalBoxDataIterator, vtkUniformGridAMRDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHierarchicalBoxDataIterator();
  ~vtkHierarchicalBoxDataIterator() override;

private:
  vtkHierarchicalBoxDataIterator(const vtkHierarchicalBoxDataIterator&) = delete;
  void operator=(const vtkHierarchicalBoxDataIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkHierarchicalBoxDataIterator.h
