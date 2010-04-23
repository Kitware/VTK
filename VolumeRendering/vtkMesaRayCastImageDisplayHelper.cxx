/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRayCastImageDisplayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Make sure this is first, so any includes of gl.h can be stoped if needed
#define VTK_IMPLEMENT_MESA_CXX

#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"

#include <math.h>
#include "vtkMesaRayCastImageDisplayHelper.h"

// make sure this file is included before the #define takes place
// so we don't get two vtkMesaRayCastImageDisplayHelper classes defined.
#include "vtkOpenGLRayCastImageDisplayHelper.h"
#include "vtkMesaRayCastImageDisplayHelper.h"

// Make sure vtkMesaRayCastImageDisplayHelper is a copy of 
// vtkOpenGLRayCastImageDisplayHelper with vtkOpenGLRayCastImageDisplayHelper 
// replaced with vtkMesaRayCastImageDisplayHelper
#define vtkOpenGLRayCastImageDisplayHelper vtkMesaRayCastImageDisplayHelper
#include "vtkOpenGLRayCastImageDisplayHelper.cxx"
#undef vtkOpenGLRayCastImageDisplayHelper

vtkStandardNewMacro(vtkMesaRayCastImageDisplayHelper);
