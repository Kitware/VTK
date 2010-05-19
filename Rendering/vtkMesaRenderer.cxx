/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderer.cxx

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
#include "vtkToolkits.h"
#include "vtkMesaRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaRenderer classes defined.
#include "vtkOpenGLRenderer.h"
#include "vtkMesaRenderer.h"

// Make sure vtkMesaRenderer is a copy of vtkOpenGLRenderer
// with vtkOpenGLRenderer replaced with vtkMesaRenderer
#define vtkOpenGLRenderer vtkMesaRenderer
#include "vtkOpenGLRenderer.cxx"
#undef vtkOpenGLRenderer

vtkStandardNewMacro(vtkMesaRenderer);

vtkCamera *vtkMesaRenderer::MakeCamera()
{
  return vtkMesaCamera::New();
}

vtkLight *vtkMesaRenderer::MakeLight()
{
  return vtkMesaLight::New();
}
