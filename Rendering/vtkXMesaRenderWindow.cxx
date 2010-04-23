/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#define VTK_IMPLEMENT_MESA_CXX

#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"
#include "MangleMesaInclude/osmesa.h"
#include "vtkXMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaProperty.h"
#include "vtkMesaTexture.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkMesaActor.h"
#include "vtkMesaPolyDataMapper.h"
#include "MangleMesaInclude/glx_mangle.h"
#include "MangleMesaInclude/glx.h"
#include "MangleMesaInclude/osmesa.h"


#define vtkXOpenGLRenderWindow vtkXMesaRenderWindow
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#define vtkOpenGLRenderer vtkMesaRenderer
#define vtkOpenGLProperty vtkMesaProperty
#define vtkOpenGLTexture vtkMesaTexture
#define vtkOpenGLCamera vtkMesaCamera
#define vtkOpenGLLight vtkMesaLight
#define vtkOpenGLActor vtkMesaActor
#define vtkOpenGLPolyDataMapper vtkMesaPolyDataMapper
#define vtkOSMesaDestroyWindow vtkOSMangleMesaDestroyWindow
#define vtkOSMesaCreateWindow vtkOSMangleMesaCreateWindow
#define vtkXOpenGLRenderWindowPredProc vtkXMesaRenderWindowPredProc
#define vtkXOpenGLRenderWindowFoundMatch vtkXMesaRenderWindowFoundMatch
#define vtkXError vtkMesaXError
#define vtkXOpenGLRenderWindowTryForVisual vtkXMesaRenderWindowTryForVisual
#define vtkXOpenGLRenderWindowInternal vtkXMesaRenderWindowInternal
#define VTK_OPENGL_HAS_OSMESA 1
// now include the source for vtkXOpenGLRenderWindow
#include "vtkXOpenGLRenderWindow.cxx"

vtkStandardNewMacro(vtkXMesaRenderWindow);
