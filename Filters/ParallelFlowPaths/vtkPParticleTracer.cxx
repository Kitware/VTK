/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPParticleTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPParticleTracer.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPParticleTracer);

vtkPParticleTracer::vtkPParticleTracer()
{
  this->IgnorePipelineTime = 0;
}

int vtkPParticleTracer::OutputParticles(vtkPolyData* poly)
{
  this->Output = poly;
  return 1;
}

void vtkPParticleTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
