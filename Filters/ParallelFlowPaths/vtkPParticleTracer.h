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
// .NAME vtkPParticleTracer - A Parallel Particle tracer for unsteady vector fields
// .SECTION Description
// vtkPParticleTracer is a filter that integrates a vector field to generate
//
//
// .SECTION See Also
// vtkPParticleTracerBase has the details of the algorithms


#ifndef __vtkPParticleTracer_h
#define __vtkPParticleTracer_h

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
  vtkPParticleTracer(const vtkPParticleTracer&);  // Not implemented.
  void operator=(const vtkPParticleTracer&); // Not implemented
};


#endif
