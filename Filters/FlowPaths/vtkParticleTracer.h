/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParticleTracer - A Parallel Particle tracer for unsteady vector fields
// .SECTION Description
// vtkParticleTracer is a filter that integrates a vector field to advect particles
//
//
// .SECTION See Also
// vtkParticleTracerBase has the details of the algorithms


#ifndef __vtkParticleTracer_h
#define __vtkParticleTracer_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkParticleTracerBase.h"

class VTKFILTERSFLOWPATHS_EXPORT vtkParticleTracer: public vtkParticleTracerBase
{
 public:
  vtkTypeMacro(vtkParticleTracer,vtkParticleTracerBase)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkParticleTracer *New();

 protected:
  vtkParticleTracer();
  virtual ~vtkParticleTracer(){}
  vtkParticleTracer(const vtkParticleTracer&);  // Not implemented.
  void operator=(const vtkParticleTracer&);  // Not implemented.
  virtual int OutputParticles(vtkPolyData* poly);
};


#endif
