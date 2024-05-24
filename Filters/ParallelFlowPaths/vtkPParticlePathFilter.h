// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPParticlePathFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkPParticlePathFilter is a filter that integrates a vector field to generate
 * path lines.
 *
 * @sa
 * vtkPParticlePathFilterBase has the details of the algorithms
 */

#ifndef vtkPParticlePathFilter_h
#define vtkPParticlePathFilter_h

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro
#include "vtkParticlePathFilter.h"

VTK_ABI_NAMESPACE_BEGIN
VTK_DEPRECATED_IN_9_4_0("Use vtkParticlePathFilter instead")
class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticlePathFilter : public vtkParticlePathFilter
{
public:
  vtkTypeMacro(vtkPParticlePathFilter, vtkParticlePathFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPParticlePathFilter* New();

protected:
  vtkPParticlePathFilter() = default;
  ~vtkPParticlePathFilter() override = default;

private:
  vtkPParticlePathFilter(const vtkPParticlePathFilter&) = delete;
  void operator=(const vtkPParticlePathFilter&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
