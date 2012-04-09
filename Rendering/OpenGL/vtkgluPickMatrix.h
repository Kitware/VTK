/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkgluPickMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkgluPickMatrix - implement selected glu functionality
// .SECTION Description
// This file implements selected glu functionality to avoid
// system dependencies on glu.

#ifndef vtkgluPickMatrix_h
#define vtkgluPickMatrix_h

#include "vtkOpenGL.h" // Needed for GLfloat and GLdouble.

// This function was copied from Mesa and sets up the pick matrix
inline void vtkgluPickMatrix( GLdouble x, GLdouble y,
                              GLdouble width, GLdouble height,
                              int *origin, int *size )
{
  GLfloat m[16];
  GLfloat sx, sy;
  GLfloat tx, ty;

  sx = size[0] / width;
  sy = size[1] / height;
  tx = (size[0] + 2.0 * (origin[0] - x)) / width;
  ty = (size[1] + 2.0 * (origin[1] - y)) / height;

#define M(row,col)  m[col*4+row]
   M(0,0) = sx;   M(0,1) = 0.0;  M(0,2) = 0.0;  M(0,3) = tx;
   M(1,0) = 0.0;  M(1,1) = sy;   M(1,2) = 0.0;  M(1,3) = ty;
   M(2,0) = 0.0;  M(2,1) = 0.0;  M(2,2) = 1.0;  M(2,3) = 0.0;
   M(3,0) = 0.0;  M(3,1) = 0.0;  M(3,2) = 0.0;  M(3,3) = 1.0;
#undef M

   glMultMatrixf( m );
}

#endif
// VTK-HeaderTest-Exclude: vtkgluPickMatrix.h
