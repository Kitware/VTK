/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPParticlePathFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkPParticleTracerBase.h"
#include "vtkParticlePathFilter.h" //for utility

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro
class VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticlePathFilter : public vtkPParticleTracerBase
{
public:
  vtkTypeMacro(vtkPParticlePathFilter, vtkPParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPParticlePathFilter* New();

protected:
  vtkPParticlePathFilter();
  ~vtkPParticlePathFilter() override;

  virtual void ResetCache() override;
  virtual int OutputParticles(vtkPolyData* poly) override;
  virtual void InitializeExtraPointDataArrays(vtkPointData* outputPD) override;
  virtual void AppendToExtraPointDataArrays(
    vtkParticleTracerBaseNamespace::ParticleInformation&) override;
  void Finalize() override;

  //
  // Store any information we need in the output and fetch what we can
  // from the input
  //
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ParticlePathFilterInternal It;
  vtkDoubleArray* SimulationTime;
  vtkIntArray* SimulationTimeStep;

private:
  vtkPParticlePathFilter(const vtkPParticlePathFilter&) = delete;
  void operator=(const vtkPParticlePathFilter&) = delete;
};
#endif
