// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPParticleTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkPParticleTracer is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * vtkPParticleTracerBase has the details of the algorithms
 */

#ifndef vtkPParticleTracer_h
#define vtkPParticleTracer_h

#include "vtkParticleTracer.h"
#include "vtkSmartPointer.h" // For protected ivars.

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
VTK_DEPRECATED_IN_9_4_0("Use vtkParticleTracer instead")
class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticleTracer : public vtkParticleTracer
{
public:
  vtkTypeMacro(vtkPParticleTracer, vtkParticleTracer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPParticleTracer* New();

protected:
  vtkPParticleTracer() = default;
  ~vtkPParticleTracer() override = default;

private:
  vtkPParticleTracer(const vtkPParticleTracer&) = delete;
  void operator=(const vtkPParticleTracer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
