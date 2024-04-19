// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHierarchicalDataLevelFilter
 * @brief   generate scalars from levels
 *
 * Legacy class. Use vtkLevelIdScalars instead.
 *
 * @sa
 * vtkLevelIdScalars
 */

#ifndef vtkHierarchicalDataLevelFilter_h
#define vtkHierarchicalDataLevelFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkLevelIdScalars.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkHierarchicalDataLevelFilter : public vtkLevelIdScalars
{
public:
  vtkTypeMacro(vtkHierarchicalDataLevelFilter, vtkLevelIdScalars);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkHierarchicalDataLevelFilter* New();

protected:
  vtkHierarchicalDataLevelFilter();
  ~vtkHierarchicalDataLevelFilter() override;

private:
  vtkHierarchicalDataLevelFilter(const vtkHierarchicalDataLevelFilter&) = delete;
  void operator=(const vtkHierarchicalDataLevelFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
