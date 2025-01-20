// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHierarchicalDataSetGeometryFilter
 * @brief   extract geometry from hierarchical data
 *
 * Legacy class. Use vtkCompositeDataGeometryFilter instead.
 *
 * @sa
 * vtkCompositeDataGeometryFilter
 */

#ifndef vtkHierarchicalDataSetGeometryFilter_h
#define vtkHierarchicalDataSetGeometryFilter_h

#include "vtkCompositeDataGeometryFilter.h"
#include "vtkDeprecation.h"           // For VTK_DEPRECATED_IN_9_5_0
#include "vtkFiltersGeometryModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;

class VTK_DEPRECATED_IN_9_5_0("Please use `vtkCompositeDataGeometryFilter` instead.")
  VTKFILTERSGEOMETRY_EXPORT vtkHierarchicalDataSetGeometryFilter
  : public vtkCompositeDataGeometryFilter
{
public:
  static vtkHierarchicalDataSetGeometryFilter* New();
  vtkTypeMacro(vtkHierarchicalDataSetGeometryFilter, vtkCompositeDataGeometryFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHierarchicalDataSetGeometryFilter();
  ~vtkHierarchicalDataSetGeometryFilter() override;

private:
  vtkHierarchicalDataSetGeometryFilter(const vtkHierarchicalDataSetGeometryFilter&) = delete;
  void operator=(const vtkHierarchicalDataSetGeometryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
