/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExerciseUnstructuredGridRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef _ExerciseUnstructuredGridRayCastMapper_h
#define _ExerciseUnstructuredGridRayCastMapper_h

#include "vtkSystemIncludes.h"

class vtkUnstructuredGridVolumeRayCastFunction;
class vtkUnstructuredGridVolumeRayIntegrator;

typedef vtkUnstructuredGridVolumeRayCastFunction *(*RayCastFunctionCreator)(void);
typedef vtkUnstructuredGridVolumeRayIntegrator *(*RayIntegratorCreator)(void);

// Exercises the unstructured grid ray cast mapper with the given function
// and integrator.
int ExerciseUnstructuredGridRayCastMapper(
                                      int argc, char *argv[],
                                      RayCastFunctionCreator NewFunction = NULL,
                                      RayIntegratorCreator NewIntegrator = NULL,
                                      int UseCellData = 0,
                                      int TestDependentComponents = 1);

#endif
