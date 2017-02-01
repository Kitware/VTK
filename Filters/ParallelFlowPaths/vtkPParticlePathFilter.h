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
class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticlePathFilter: public vtkPParticleTracerBase
{
public:
  vtkTypeMacro(vtkPParticlePathFilter,vtkPParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPParticlePathFilter *New();

protected:
  vtkPParticlePathFilter();
  ~vtkPParticlePathFilter();

  virtual void ResetCache() VTK_OVERRIDE;
  virtual int OutputParticles(vtkPolyData* poly) VTK_OVERRIDE;
  virtual void InitializeExtraPointDataArrays(vtkPointData* outputPD) VTK_OVERRIDE;
  virtual void AppendToExtraPointDataArrays(vtkParticleTracerBaseNamespace::ParticleInformation &) VTK_OVERRIDE;
  void Finalize() VTK_OVERRIDE;

  ParticlePathFilterInternal It;
  vtkDoubleArray* SimulationTime;
  vtkIntArray* SimulationTimeStep;

private:
  vtkPParticlePathFilter(const vtkPParticlePathFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPParticlePathFilter&) VTK_DELETE_FUNCTION;
};
#endif
