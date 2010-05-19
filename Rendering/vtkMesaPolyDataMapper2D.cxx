/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper2D.cxx

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
#include "vtkMesaPolyDataMapper2D.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"

// make sure this file is included before the #define takes place
// so we don't get two vtkMesaPolyDataMapper2D classes defined.
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkMesaPolyDataMapper2D.h"

// Make sure vtkMesaPolyDataMapper2D is a copy of vtkOpenGLPolyDataMapper2D
// with vtkOpenGLPolyDataMapper2D replaced with vtkMesaPolyDataMapper2D
#define vtkOpenGLPolyDataMapper2D vtkMesaPolyDataMapper2D
#include "vtkOpenGLPolyDataMapper2D.cxx"
#undef vtkOpenGLPolyDataMapper2D

vtkStandardNewMacro(vtkMesaPolyDataMapper2D);
