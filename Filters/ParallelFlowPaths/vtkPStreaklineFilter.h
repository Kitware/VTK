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
#include "vtkPParticleTracerBase.h"
#include "vtkSmartPointer.h"     // For protected ivars.
#include "vtkStreaklineFilter.h" //for utility

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPStreaklineFilter : public vtkPParticleTracerBase
{
public:
  vtkTypeMacro(vtkPStreaklineFilter, vtkPParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPStreaklineFilter* New();

protected:
  vtkPStreaklineFilter();
  ~vtkPStreaklineFilter() override = default;
  vtkPStreaklineFilter(const vtkPStreaklineFilter&) = delete;
  void operator=(const vtkPStreaklineFilter&) = delete;
  int OutputParticles(vtkPolyData* poly) override;
  void Finalize() override;

  StreaklineFilterInternal It;
};

VTK_ABI_NAMESPACE_END
#endif
