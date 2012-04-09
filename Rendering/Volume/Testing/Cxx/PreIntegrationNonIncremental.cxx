/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PreIntegrationNonIncremental.cxx

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

  // Turn off incremental building of the table.
  integrator->IncrementalPreIntegrationOff();

  // Make the table much smaller since it takes much longer to build.
  integrator->SetIntegrationTableScalarResolution(32);
  integrator->SetIntegrationTableLengthResolution(64);

  return integrator;
}

int PreIntegrationNonIncremental(int argc, char *argv[])
{
  return ExerciseUnstructuredGridRayCastMapper(argc, argv, NULL,
                                               CreatePreIntegration, 0, 0);
}
