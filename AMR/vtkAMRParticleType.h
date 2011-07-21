/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRParticleType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRParticleType.h -- Defines particles types
//
// .SECTION Description
// Defines an enumerator that holds all the allowable/supported
// particle types in VTK.

#ifndef VTKAMRPARTICLETYPE_H_
#define VTKAMRPARTICLETYPE_H_

typedef enum {
  VTK_GENERIC_PARTICLE = 0,
  VTK_TRACER_PARTICLE  = 1,
  VTK_CHARGE_PARTICLE  = 2,
  VTK_MASSIVE_PARTICLE = 3,

  VTK_NUMBER_OF_PARTICLE_TYPES
} VTKParticleType;

#endif /* VTKAMRPARTICLETYPE_H_ */
