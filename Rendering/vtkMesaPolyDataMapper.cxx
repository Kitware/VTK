/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
#include "vtkOpenGLPolyDataMapper.cxx"
#undef vtkOpenGLPolyDataMapper

vtkCxxRevisionMacro(vtkMesaPolyDataMapper, "1.12");
vtkStandardNewMacro(vtkMesaPolyDataMapper);
