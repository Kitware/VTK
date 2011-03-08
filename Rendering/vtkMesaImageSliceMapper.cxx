/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageSliceMapper.cxx

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
#include "vtkMesaImageSliceMapper.h"
#include "vtkMesaCamera.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRenderWindow.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaImageSliceMapper classes defined.
#include "vtkOpenGLImageSliceMapper.h"
#include "vtkMesaImageSliceMapper.h"

// Make sure vtkMesaImageSliceMapper is a copy of vtkOpenGLImageSliceMapper
// with vtkOpenGLImageSliceMapper replaced with vtkMesaImageSliceMapper
#define vtkOpenGLImageSliceMapper vtkMesaImageSliceMapper
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#include "vtkOpenGLImageSliceMapper.cxx"
#undef vtkOpenGLImageSliceMapper
#undef vtkOpenGLRenderWindow

vtkStandardNewMacro(vtkMesaImageSliceMapper);
