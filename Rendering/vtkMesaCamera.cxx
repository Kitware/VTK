/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This keeps the New method from being defined in included cxx file.
#define VTK_IMPLEMENT_MESA_CXX
#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"

#include <math.h>
#include "vtkToolkits.h"
#include "vtkMesaCamera.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaCamera classes defined.
#include "vtkOpenGLCamera.h"
#include "vtkMesaCamera.h"

// Make sure vtkMesaCamera is a copy of vtkOpenGLCamera
// with vtkOpenGLCamera replaced with vtkMesaCamera
#define vtkOpenGLCamera vtkMesaCamera
#include "vtkOpenGLCamera.cxx"
#undef vtkOpenGLCamera

vtkStandardNewMacro(vtkMesaCamera);
