/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreaklineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkParticleTracerBase.h"

class VTKFILTERSFLOWPATHS_EXPORT StreaklineFilterInternal
{
public:
  StreaklineFilterInternal():Filter(nullptr){}
  void Initialize(vtkParticleTracerBase* filter);
  virtual ~StreaklineFilterInternal(){}
  virtual int OutputParticles(vtkPolyData* poly);
  void Finalize();
  void Reset();
private:
  vtkParticleTracerBase* Filter;

};


class VTKFILTERSFLOWPATHS_EXPORT vtkStreaklineFilter: public vtkParticleTracerBase
{
 public:
  vtkTypeMacro(vtkStreaklineFilter,vtkParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkStreaklineFilter *New();

 protected:
  vtkStreaklineFilter();
  ~vtkStreaklineFilter() override{}
  vtkStreaklineFilter(const vtkStreaklineFilter&) = delete;
  void operator=(const vtkStreaklineFilter&) = delete;
  int OutputParticles(vtkPolyData* poly) override;
  void Finalize() override;

  StreaklineFilterInternal It;
};


#endif
