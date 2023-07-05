// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHierarchicalPolyDataMapper
 * @brief   a class that renders hierarchical polygonal data
 *
 * Legacy class. Use vtkCompositePolyDataMapper instead.
 *
 * @sa
 * vtkPolyDataMapper
 */

#ifndef vtkHierarchicalPolyDataMapper_h
#define vtkHierarchicalPolyDataMapper_h

#include "vtkCompositePolyDataMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT vtkHierarchicalPolyDataMapper : public vtkCompositePolyDataMapper
{

public:
  static vtkHierarchicalPolyDataMapper* New();
  vtkTypeMacro(vtkHierarchicalPolyDataMapper, vtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHierarchicalPolyDataMapper();
  ~vtkHierarchicalPolyDataMapper() override;

private:
  vtkHierarchicalPolyDataMapper(const vtkHierarchicalPolyDataMapper&) = delete;
  void operator=(const vtkHierarchicalPolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
