/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PreIntegrationIncremental.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExerciseUnstructuredGridRayCastMapper.h"

#include "vtkUnstructuredGridPreIntegration.h"

static vtkUnstructuredGridVolumeRayIntegrator *CreatePreIntegration()
{
  vtkUnstructuredGridPreIntegration *integrator =
    vtkUnstructuredGridPreIntegration::New();
  integrator->IncrementalPreIntegrationOn();
  return integrator;
}

int PreIntegrationIncremental(int argc, char *argv[])
{
  return ExerciseUnstructuredGridRayCastMapper(argc, argv, NULL,
                                               CreatePreIntegration, 0, 0);
}
