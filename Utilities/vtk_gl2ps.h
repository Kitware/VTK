/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk_gl2ps.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtk_gl2ps_h
#define __vtk_gl2ps_h

/* Use the gl2ps library configured for VTK.  */
#include "vtkToolkits.h"
#ifdef VTK_USE_SYSTEM_GL2PS
# include <gl2ps.h>
#else
# include <vtkgl2ps/gl2ps.h>
#endif

#endif
