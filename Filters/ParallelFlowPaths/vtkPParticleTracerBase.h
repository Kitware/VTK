// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParticleTracerBase
 * @brief   A parallel particle tracer for vector fields
 *
 * vtkPParticleTracerBase is the base class for parallel filters that advect particles
 * in a vector field. Note that the input vtkPointData structure must
 * be identical on all datasets.
 * @sa
 * vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
 * vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkStreamTracer
 */

#ifndef vtkPParticleTracerBase_h
#define vtkPParticleTracerBase_h

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro
#include "vtkParticleTracerBase.h"

VTK_DEPRECATED_IN_9_4_0("Use vtkParticleTracerBase instead")
VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticleTracerBase : public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkPParticleTracerBase, vtkParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPParticleTracerBase() = default;
  ~vtkPParticleTracerBase() override = default;

private:
  vtkPParticleTracerBase(const vtkPParticleTracerBase&) = delete;
  void operator=(const vtkPParticleTracerBase&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
