// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStreaklineFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkStreaklineFilter is a filter that integrates a vector field to generate streak lines
 *
 *
 * @sa
 * vtkParticleTracerBase has the details of the algorithms
 */

#ifndef vtkStreaklineFilter_h
#define vtkStreaklineFilter_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkParticleTracerBase.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSFLOWPATHS_EXPORT vtkStreaklineFilter : public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkStreaklineFilter, vtkParticleTracerBase);

  static vtkStreaklineFilter* New();

protected:
  vtkStreaklineFilter() = default;
  ~vtkStreaklineFilter() override = default;
  vtkStreaklineFilter(const vtkStreaklineFilter&) = delete;
  void operator=(const vtkStreaklineFilter&) = delete;

  int Initialize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int Finalize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
};

VTK_ABI_NAMESPACE_END

#endif
