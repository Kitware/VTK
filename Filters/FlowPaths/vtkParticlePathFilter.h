/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticlePathFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParticlePathFilter - A Parallel Particle tracer for unsteady vector fields
// .SECTION Description
// vtkParticlePathFilter is a filter that integrates a vector field to generate particle paths
//
//
// .SECTION See Also
// vtkParticlePathFilterBase has the details of the algorithms


#ifndef vtkParticlePathFilter_h
#define vtkParticlePathFilter_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkParticleTracerBase.h"
#include <vector> // For protected ivars

class VTKFILTERSFLOWPATHS_EXPORT ParticlePathFilterInternal
{
public:
  ParticlePathFilterInternal():Filter(NULL){}
  void Initialize(vtkParticleTracerBase* filter);
  virtual ~ParticlePathFilterInternal(){}
  virtual int OutputParticles(vtkPolyData* poly);
  void Finalize();
  void Reset();

private:
  vtkParticleTracerBase* Filter;
  std::vector<vtkSmartPointer<vtkIdList> > Paths;
};

class VTKFILTERSFLOWPATHS_EXPORT vtkParticlePathFilter: public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkParticlePathFilter,vtkParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkParticlePathFilter *New();

protected:
  vtkParticlePathFilter();
  ~vtkParticlePathFilter();
  vtkParticlePathFilter(const vtkParticlePathFilter&);  // Not implemented.
  void operator=(const vtkParticlePathFilter&);  // Not implemented.

  virtual void ResetCache();
  virtual int OutputParticles(vtkPolyData* poly);
  virtual void InitializeExtraPointDataArrays(vtkPointData* outputPD);
  virtual void AppendToExtraPointDataArrays(vtkParticleTracerBaseNamespace::ParticleInformation &);

  void Finalize();

  ParticlePathFilterInternal It;

private:
  vtkDoubleArray* SimulationTime;
  vtkIntArray* SimulationTimeStep;
};


#endif
