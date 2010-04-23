/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.cxx

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
#include "vtkMesaPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"
#include "vtkToolkits.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaPolyDataMapper classes defined.
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkMesaPolyDataMapper.h"

// Make sure vtkMesaPolyDataMapper is a copy of vtkOpenGLPolyDataMapper
// with vtkOpenGLPolyDataMapper replaced with vtkMesaPolyDataMapper
#define vtkOpenGLPolyDataMapper vtkMesaPolyDataMapper
#define vtkOpenGLPolyDataMapperDrawPoints vtkMesaPolyDataMapperDrawPoints
#define vtkOpenGLPolyDataMapperDrawLines vtkMesaPolyDataMapperDrawLines
#define vtkOpenGLPolyDataMapperDrawPolygons vtkMesaPolyDataMapperDrawPolygons
#define vtkOpenGLPolyDataMapperDrawTStrips vtkMesaPolyDataMapperDrawTStrips
#define vtkOpenGLPolyDataMapperDrawTStripLines vtkMesaPolyDataMapperDrawTStripLines
#include "vtkOpenGLPolyDataMapper.cxx"
#undef vtkOpenGLPolyDataMapperDrawTStripLines
#undef vtkOpenGLPolyDataMapperDrawTStrips
#undef vtkOpenGLPolyDataMapperDrawPolygons
#undef vtkOpenGLPolyDataMapperDrawLines
#undef vtkOpenGLPolyDataMapper

vtkStandardNewMacro(vtkMesaPolyDataMapper);
