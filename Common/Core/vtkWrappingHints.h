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
#define VTK_WRAP_HINTS_DEFINED
// The return value points to a newly-created VTK object.
#define VTK_NEWINSTANCE [[vtk::newinstance]]
// The parameter is a pointer to a zerocopy buffer.
#define VTK_ZEROCOPY [[vtk::zerocopy]]
// Set preconditions for a function
#define VTK_EXPECTS(x) [[vtk::expects(x)]]
// Set size hint for parameter or return value
#define VTK_SIZEHINT(...) [[vtk::sizehint(__VA_ARGS__)]]
#endif

#ifndef VTK_WRAP_HINTS_DEFINED
#define VTK_NEWINSTANCE
#define VTK_ZEROCOPY
#define VTK_EXPECTS(x)
#define VTK_SIZEHINT(...)
#endif

#endif
// VTK-HeaderTest-Exclude: vtkWrappingHints.h
