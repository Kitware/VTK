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
#include "vtkSmartPointer.h" // For protected ivars.

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSFLOWPATHS_EXPORT StreaklineFilterInternal
{
public:
  StreaklineFilterInternal()
    : Filter(nullptr)
  {
  }
  void Initialize(vtkParticleTracerBase* filter);
  virtual ~StreaklineFilterInternal() = default;
  virtual int OutputParticles(vtkPolyData* particles);
  void Finalize();
  void Reset();

private:
  vtkParticleTracerBase* Filter;
};

class VTKFILTERSFLOWPATHS_EXPORT vtkStreaklineFilter : public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkStreaklineFilter, vtkParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkStreaklineFilter* New();

protected:
  vtkStreaklineFilter();
  ~vtkStreaklineFilter() override = default;
  vtkStreaklineFilter(const vtkStreaklineFilter&) = delete;
  void operator=(const vtkStreaklineFilter&) = delete;
  int OutputParticles(vtkPolyData* particles) override;
  void Finalize() override;

  StreaklineFilterInternal It;
};

VTK_ABI_NAMESPACE_END
#endif
