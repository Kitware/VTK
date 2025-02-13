// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParticlePathFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkParticlePathFilter is a filter that integrates a vector field to generate particle paths
 *
 *
 * @sa
 * vtkParticlePathFilterBase has the details of the algorithms
 */

#ifndef vtkParticlePathFilter_h
#define vtkParticlePathFilter_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkParticleTracerBase.h"
#include "vtkSmartPointer.h" // For Points

#include <functional> // for std::greater
#include <map>        // For Paths
#include <queue>      // For UnusedIndices
#include <vector>     // For Paths

VTK_ABI_NAMESPACE_BEGIN

class vtkPointSet;

class VTKFILTERSFLOWPATHS_EXPORT vtkParticlePathFilter : public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkParticlePathFilter, vtkParticleTracerBase);
  // void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkParticlePathFilter* New();

protected:
  vtkParticlePathFilter() = default;
  ~vtkParticlePathFilter() override = default;
  vtkParticlePathFilter(const vtkParticlePathFilter&) = delete;
  void operator=(const vtkParticlePathFilter&) = delete;

  int Initialize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int Execute(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int Finalize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSmartPointer<vtkPointSet> Points;
  std::map<vtkIdType, std::vector<vtkIdType>> Paths;
  std::priority_queue<vtkIdType, std::vector<vtkIdType>, std::greater<>> UnusedIndices;
};

VTK_ABI_NAMESPACE_END
#endif
