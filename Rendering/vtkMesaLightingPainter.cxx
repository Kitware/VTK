/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaLightingPainter.cxx

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
// so we don't get two vtkMesaLightingPainter classes defined.
#include "vtkOpenGLLightingPainter.h"
#include "vtkMesaLightingPainter.h"

// Make sure vtkMesaLightingPainter is a copy of vtkOpenGLLightingPainter
// with vtkOpenGLLightingPainter replaced with vtkMesaLightingPainter
#define vtkOpenGLLightingPainter vtkMesaLightingPainter
#include "vtkOpenGLLightingPainter.cxx"
#undef vtkOpenGLLightingPainter

vtkStandardNewMacro(vtkMesaLightingPainter);
