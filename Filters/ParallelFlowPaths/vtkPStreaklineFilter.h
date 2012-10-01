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
// .NAME vtkPStreaklineFilter - A Parallel Particle tracer for unsteady vector fields
// .SECTION Description
// vtkPStreaklineFilter is a filter that integrates a vector field to generate
//
//
// .SECTION See Also
// vtkPStreaklineFilterBase has the details of the algorithms


#ifndef __vtkPStreaklineFilter_h
#define __vtkPStreaklineFilter_h

#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreaklineFilter.h" //for utility
#include "vtkPParticleTracerBase.h"
#include "vtkFiltersParallelFlowPathsModule.h" // For export macro

class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPStreaklineFilter: public vtkPParticleTracerBase
{
 public:
  vtkTypeMacro(vtkPStreaklineFilter,vtkPParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPStreaklineFilter *New();

 protected:
  vtkPStreaklineFilter();
  ~vtkPStreaklineFilter(){}
  vtkPStreaklineFilter(const vtkPStreaklineFilter&);  // Not implemented.
  void operator=(const vtkPStreaklineFilter&);  // Not implemented.
  virtual int OutputParticles(vtkPolyData* poly);
  virtual void Finalize();

  StreaklineFilterInternal It;
};


#endif
