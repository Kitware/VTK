/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageActor.cxx

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
#include "vtkMesaImageActor.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaImageActor classes defined.
#include "vtkOpenGLImageActor.h"
#include "vtkMesaImageActor.h"

// Make sure vtkMesaImageActor is a copy of vtkOpenGLImageActor
// with vtkOpenGLImageActor replaced with vtkMesaImageActor
#define vtkOpenGLImageActor vtkMesaImageActor
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#include "vtkOpenGLImageActor.cxx"
#undef vtkOpenGLImageActor
#undef vtkOpenGLRenderWindow

vtkStandardNewMacro(vtkMesaImageActor);
