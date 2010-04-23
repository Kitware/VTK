/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaProperty.cxx

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

#include "vtkToolkits.h"
#include "vtkMesaProperty.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"

#include <math.h>

// make sure this file is included before the #define takes place
// so we don't get two vtkMesaMesaProperty classes defined.
#include "vtkOpenGLProperty.h"
#include "vtkMesaProperty.h"

// Make sure vtkMesaMesaProperty is a copy of vtkOpenGLProperty
// with vtkOpenGLProperty replaced with vtkMesaMesaProperty
#define vtkOpenGLProperty vtkMesaProperty
#include "vtkOpenGLProperty.cxx"
#undef vtkOpenGLProperty

vtkStandardNewMacro(vtkMesaProperty);
