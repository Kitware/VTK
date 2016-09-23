/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrappingHints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWrappingHints
 * @brief   hint macros for wrappers
 *
 * The macros defined in this file can be used to supply hints for the
 * wrappers.
*/

#ifndef vtkWrappingHints_h
#define vtkWrappingHints_h

#ifdef __VTK_WRAP__
// The return value points to a newly-created VTK object.
#define VTK_NEWINSTANCE [[vtk::newinstance]]
#else
#define VTK_NEWINSTANCE
#endif

#endif
// VTK-HeaderTest-Exclude: vtkWrappingHints.h
