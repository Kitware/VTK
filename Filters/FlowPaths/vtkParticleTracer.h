// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParticleTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkParticleTracer is a filter that integrates a vector field to advect particles
 *
 *
 * @sa
 * vtkParticleTracerBase has the details of the algorithms
 */

#ifndef vtkParticleTracer_h
#define vtkParticleTracer_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkParticleTracerBase.h"
#include "vtkSmartPointer.h" // For protected ivars.

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSFLOWPATHS_EXPORT vtkParticleTracer : public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkParticleTracer, vtkParticleTracerBase);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkParticleTracer* New();

protected:
  vtkParticleTracer();
  ~vtkParticleTracer() override = default;
  vtkParticleTracer(const vtkParticleTracer&) = delete;
  void operator=(const vtkParticleTracer&) = delete;
  int Finalize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
};

VTK_ABI_NAMESPACE_END
#endif
