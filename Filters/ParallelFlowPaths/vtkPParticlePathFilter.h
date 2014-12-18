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
// .NAME vtkPParticlePathFilter - A Parallel Particle tracer for unsteady vector fields
// .SECTION Description
// vtkPParticlePathFilter is a filter that integrates a vector field to generate
//
//
// .SECTION See Also
// vtkPParticlePathFilterBase has the details of the algorithms


#ifndef vtkPParticlePathFilter_h
#define vtkPParticlePathFilter_h


#include "vtkPParticleTracerBase.h"
#include "vtkParticlePathFilter.h" //for utility

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro
class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticlePathFilter: public vtkPParticleTracerBase
{
public:
  vtkTypeMacro(vtkPParticlePathFilter,vtkPParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPParticlePathFilter *New();

protected:
  vtkPParticlePathFilter();
  ~vtkPParticlePathFilter();

  virtual void ResetCache();
  virtual int OutputParticles(vtkPolyData* poly);
  virtual void InitializeExtraPointDataArrays(vtkPointData* outputPD);
  virtual void AppendToExtraPointDataArrays(vtkParticleTracerBaseNamespace::ParticleInformation &);
  void Finalize();

  ParticlePathFilterInternal It;
private:
  vtkPParticlePathFilter(const vtkPParticlePathFilter&);  // Not implemented.
  void operator=(const vtkPParticlePathFilter&); // Not implemented

  vtkDoubleArray* SimulationTime;
  vtkIntArray* SimulationTimeStep;
};


#endif
