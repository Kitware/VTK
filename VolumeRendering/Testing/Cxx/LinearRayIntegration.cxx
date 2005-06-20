/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LinearRayIntegration.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExerciseUnstructuredGridRayCastMapper.h"

#include "vtkUnstructuredGridLinearRayIntegrator.h"

static vtkUnstructuredGridVolumeRayIntegrator *CreateLinearRayIntegrator()
{
  return vtkUnstructuredGridLinearRayIntegrator::New();
}

int LinearRayIntegration(int argc, char *argv[])
{
  return ExerciseUnstructuredGridRayCastMapper(argc, argv, NULL,
                                               CreateLinearRayIntegrator);
}
