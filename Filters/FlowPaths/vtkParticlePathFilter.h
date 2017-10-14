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
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkParticleTracerBase.h"
#include <vector> // For protected ivars

class VTKFILTERSFLOWPATHS_EXPORT ParticlePathFilterInternal
{
public:
  ParticlePathFilterInternal():Filter(nullptr){}
  void Initialize(vtkParticleTracerBase* filter);
  virtual ~ParticlePathFilterInternal(){}
  virtual int OutputParticles(vtkPolyData* poly);
  void SetClearCache(bool clearCache)
  {
    this->ClearCache = clearCache;
  }
  bool GetClearCache()
  {
    return this->ClearCache;
  }
  void Finalize();
  void Reset();

private:
  vtkParticleTracerBase* Filter;
  // Paths doesn't seem to work properly. it is meant to make connecting lines
  // for the particles
  std::vector<vtkSmartPointer<vtkIdList> > Paths;
  bool ClearCache; // false by default
};

class VTKFILTERSFLOWPATHS_EXPORT vtkParticlePathFilter: public vtkParticleTracerBase
{
public:
  vtkTypeMacro(vtkParticlePathFilter,vtkParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkParticlePathFilter *New();

protected:
  vtkParticlePathFilter();
  ~vtkParticlePathFilter() override;
  vtkParticlePathFilter(const vtkParticlePathFilter&) = delete;
  void operator=(const vtkParticlePathFilter&) = delete;

  void ResetCache() override;
  int OutputParticles(vtkPolyData* poly) override;
  void InitializeExtraPointDataArrays(vtkPointData* outputPD) override;
  void AppendToExtraPointDataArrays(vtkParticleTracerBaseNamespace::ParticleInformation &) override;

  void Finalize() override;

  ParticlePathFilterInternal It;

private:
  vtkDoubleArray* SimulationTime;
  vtkIntArray* SimulationTimeStep;
};

#endif
