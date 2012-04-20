/*=========================================================================

  Program:   Visualization Toolkit
  Module:    HomogeneousRayIntegration.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExerciseUnstructuredGridRayCastMapper.h"

#include "vtkUnstructuredGridHomogeneousRayIntegrator.h"

static vtkUnstructuredGridVolumeRayIntegrator *CreateHomogeneousRayIntegrator()
{
  return vtkUnstructuredGridHomogeneousRayIntegrator::New();
}

int HomogeneousRayIntegration(int argc, char *argv[])
{
  return ExerciseUnstructuredGridRayCastMapper(argc, argv, NULL,
                                               CreateHomogeneousRayIntegrator,
                                               1);
}
