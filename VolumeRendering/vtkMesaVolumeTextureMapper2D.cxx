/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaVolumeTextureMapper2D.cxx

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
#include "vtkMesaVolumeTextureMapper2D.h"
#include "vtkRenderWindow.h"
#include "vtkMesaProperty.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkCuller.h"

// make sure this file is included before the #define takes place
// so we don't get two vtkMesaVolumeTextureMapper2D classes defined.
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkMesaVolumeTextureMapper2D.h"

// Make sure vtkMesaVolumeTextureMapper2D is a copy of vtkOpenGLVolumeTextureMapper2D
// with vtkOpenGLVolumeTextureMapper2D replaced with vtkMesaVolumeTextureMapper2D
#define vtkOpenGLVolumeTextureMapper2D vtkMesaVolumeTextureMapper2D
#include "vtkOpenGLVolumeTextureMapper2D.cxx"
#undef vtkOpenGLVolumeTextureMapper2D

vtkStandardNewMacro(vtkMesaVolumeTextureMapper2D);
