/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaDisplayListPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Make sure this is first, so any includes of gl.h can be stoped if needed
// This also keeps the New method from being defined in included cxx file.
#define VTK_IMPLEMENT_MESA_CXX
#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"

#include <math.h>
#include "vtkToolkits.h"

// make sure this file is included before the #define takes place
// so we don't get two vtkMesaDisplayListPainter classes defined.
#include "vtkOpenGLDisplayListPainter.h"
#include "vtkMesaDisplayListPainter.h"

// Make sure vtkMesaDisplayListPainter is a copy of vtkOpenGLDisplayListPainter
// with vtkOpenGLDisplayListPainter replaced with vtkMesaDisplayListPainter
#define vtkOpenGLDisplayListPainter vtkMesaDisplayListPainter
#include "vtkOpenGLDisplayListPainter.cxx"
#undef vtkOpenGLDisplayListPainter

vtkStandardNewMacro(vtkMesaDisplayListPainter);
