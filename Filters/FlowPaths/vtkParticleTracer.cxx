/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkParticleTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParticleTracer.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

vtkObjectFactoryNewMacro(vtkParticleTracer)

vtkParticleTracer::vtkParticleTracer()
{
  this->IgnorePipelineTime = 0;
}

int vtkParticleTracer::OutputParticles(vtkPolyData* poly)
{
  this->Output = poly;
  return 1;
}

void vtkParticleTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
