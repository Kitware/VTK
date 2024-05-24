// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPStreaklineFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkPStreaklineFilter is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * vtkPStreaklineFilterBase has the details of the algorithms
 */

#ifndef vtkPStreaklineFilter_h
#define vtkPStreaklineFilter_h

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro
#include "vtkStreaklineFilter.h"

VTK_ABI_NAMESPACE_BEGIN
VTK_DEPRECATED_IN_9_4_0("Use vtkStreaklineFilter instead")
class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPStreaklineFilter : public vtkStreaklineFilter
{
public:
  vtkTypeMacro(vtkPStreaklineFilter, vtkStreaklineFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPStreaklineFilter* New();

protected:
  vtkPStreaklineFilter() = default;
  ~vtkPStreaklineFilter() override = default;
  vtkPStreaklineFilter(const vtkPStreaklineFilter&) = delete;
  void operator=(const vtkPStreaklineFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
