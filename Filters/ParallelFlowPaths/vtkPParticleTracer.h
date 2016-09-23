/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPParticleTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPParticleTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkPParticleTracer is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * vtkPParticleTracerBase has the details of the algorithms
*/

#ifndef vtkPParticleTracer_h
#define vtkPParticleTracer_h

#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkPParticleTracerBase.h"

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro

class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPParticleTracer: public vtkPParticleTracerBase
{
 public:
  vtkTypeMacro(vtkPParticleTracer,vtkPParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPParticleTracer *New();

 protected:
  vtkPParticleTracer();
  ~vtkPParticleTracer(){}
  virtual int OutputParticles(vtkPolyData* poly);
private:
  vtkPParticleTracer(const vtkPParticleTracer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPParticleTracer&) VTK_DELETE_FUNCTION;
};


#endif
