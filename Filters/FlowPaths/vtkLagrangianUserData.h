/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @struct   vtkLagrangianUserData
 * @brief   struct to hold a user data
 *
 * Struct to hold a user data used by the Lagrangian Particle Tracker
 */

#ifndef vtkLagrangianUserData_h
#define vtkLagrangianUserData_h

#include "vtkFiltersFlowPathsModule.h" // For export macro

struct VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianUserData
{
  void* UserData;
};
#endif // vtkLagrangianUserData_h
// VTK-HeaderTest-Exclude: vtkLagrangianUserData.h
