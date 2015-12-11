/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderingCoreEnums.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkRenderingCoreEnums_h
#define vtkRenderingCoreEnums_h

// Marker shapes for plotting
enum
{
  VTK_MARKER_NONE = 0,
  VTK_MARKER_CROSS,
  VTK_MARKER_PLUS,
  VTK_MARKER_SQUARE,
  VTK_MARKER_CIRCLE,
  VTK_MARKER_DIAMOND,

  VTK_MARKER_UNKNOWN  // Must be last.
};

#endif // vtkRenderingCoreEnums_h
// VTK-HeaderTest-Exclude: vtkRenderingCoreEnums.h
