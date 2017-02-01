/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPStreaklineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPStreaklineFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkPStreaklineFilter is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * vtkPStreaklineFilterBase has the details of the algorithms
*/

#ifndef vtkPStreaklineFilter_h
#define vtkPStreaklineFilter_h

#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreaklineFilter.h" //for utility
#include "vtkPParticleTracerBase.h"
#include "vtkFiltersParallelFlowPathsModule.h" // For export macro

class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPStreaklineFilter: public vtkPParticleTracerBase
{
 public:
  vtkTypeMacro(vtkPStreaklineFilter,vtkPParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPStreaklineFilter *New();

 protected:
  vtkPStreaklineFilter();
  ~vtkPStreaklineFilter(){}
  vtkPStreaklineFilter(const vtkPStreaklineFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPStreaklineFilter&) VTK_DELETE_FUNCTION;
  virtual int OutputParticles(vtkPolyData* poly) VTK_OVERRIDE;
  virtual void Finalize() VTK_OVERRIDE;

  StreaklineFilterInternal It;
};


#endif
