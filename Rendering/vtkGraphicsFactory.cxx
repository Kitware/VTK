/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.cxx
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

#include "vtkObjectFactory.h"
#include "vtkGraphicsFactory.h"
#include "vtkToolkits.h"
#include "stdlib.h"
#include "vtkDebugLeaks.h"

// if using some sort of opengl, then include these files
#if defined(VTK_USE_OGLR) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLImageActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLVolumeRayCastMapper.h"
#endif

// Win32 specific stuff
#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"
#endif

// Apple OSX stuff
#ifdef VTK_USE_CARBON
#include "vtkCarbonRenderWindow.h"
#include "vtkCarbonRenderWindowInteractor.h"
#endif
#ifdef VTK_USE_COCOA
#include "vtkCocoaRenderWindow.h"
#include "vtkCocoaRenderWindowInteractor.h"
#endif

// X OpenGL stuff
#ifdef VTK_USE_OGLR
#include "vtkXRenderWindowInteractor.h"
#include "vtkXOpenGLRenderWindow.h"
#endif

#if defined(VTK_MANGLE_MESA)
#include "vtkMesaActor.h"
#include "vtkMesaCamera.h"
#include "vtkMesaImageActor.h"
#include "vtkMesaLight.h"
#include "vtkMesaProperty.h"
#include "vtkMesaPolyDataMapper.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaTexture.h"
#include "vtkMesaVolumeTextureMapper2D.h"
#include "vtkMesaVolumeRayCastMapper.h"
#include "vtkXMesaRenderWindow.h"
#endif

#include "vtkCriticalSection.h"
static vtkSimpleCriticalSection vtkUseMesaClassesCriticalSection;
int vtkGraphicsFactory::UseMesaClasses = 0;

vtkCxxRevisionMacro(vtkGraphicsFactory, "1.29");
vtkStandardNewMacro(vtkGraphicsFactory);

const char *vtkGraphicsFactory::GetRenderLibrary()
{
  const char *temp;
  
  // first check the environment variable
  temp = getenv("VTK_RENDERER");
  
  // Backward compatibility
  if ( temp )
    {
    if (!strcmp("oglr",temp))
      {
      temp = "OpenGL";
      }
    else if (!strcmp("woglr",temp))
      {
      temp = "Win32OpenGL";
      }
    else if (strcmp("OpenGL",temp) && 
             strcmp("Win32OpenGL",temp))
      {
      vtkGenericWarningMacro(<<"VTK_RENDERER set to unsupported type:" << temp);
      temp = NULL;
      }
    }

  // if nothing is set then work down the list of possible renderers
  if ( !temp )
    {
#ifdef VTK_USE_OGLR
    temp = "OpenGL";
#endif
#ifdef _WIN32
    temp = "Win32OpenGL";
#endif
#ifdef VTK_USE_CARBON
    temp = "CarbonOpenGL";
#endif
#ifdef VTK_USE_COCOA
    temp = "CocoaOpenGL";
#endif
    }
  
  return temp;
}

vtkObject* vtkGraphicsFactory::CreateInstance(const char* vtkclassname )
{
  // first check the object factory
  vtkObject *ret = vtkObjectFactory::CreateInstance(vtkclassname);
  if (ret)
    {
    return ret;
    }
  // if the factory failed to create the object,
  // then destroy it now, as vtkDebugLeaks::ConstructClass was called
  // with vtkclassname, and not the real name of the class
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::DestructClass(vtkclassname);
#endif
  const char *rl = vtkGraphicsFactory::GetRenderLibrary();
  
#ifdef VTK_USE_OGLR
  if (!strcmp("OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkXMesaRenderWindow::New();
        }
#endif
      return vtkXOpenGLRenderWindow::New();
      }
    }
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkXRenderWindowInteractor::New();
    }
#endif

#ifdef _WIN32
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkWin32RenderWindowInteractor::New();
    }
  if (!strcmp("Win32OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
      {
      return vtkWin32OpenGLRenderWindow::New();
      }
    }
#endif

#ifdef VTK_USE_CARBON
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkCarbonRenderWindowInteractor::New();
    }
  if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
    {
    return vtkCarbonRenderWindow::New();
    }
#endif
#ifdef VTK_USE_COCOA
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkCocoaRenderWindowInteractor::New();
    }
  if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
    {
    return vtkCocoaRenderWindow::New();
    }
#endif

#if defined(VTK_USE_OGLR) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
  if (!strcmp("OpenGL",rl) || !strcmp("Win32OpenGL",rl) || !strcmp("CarbonOpenGL",rl) || !strcmp("CocoaOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkActor") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaActor::New();
        }
#endif
      return vtkOpenGLActor::New();
      }
    if(strcmp(vtkclassname, "vtkCamera") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaCamera::New();
        }
#endif
      return vtkOpenGLCamera::New();
      }
    if(strcmp(vtkclassname, "vtkImageActor") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaImageActor::New();
        }
#endif
      return vtkOpenGLImageActor::New();
      }
    if(strcmp(vtkclassname, "vtkLight") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaLight::New();
        }
#endif
      return vtkOpenGLLight::New();
      }
    if(strcmp(vtkclassname, "vtkProperty") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaProperty::New();
        }
#endif
      return vtkOpenGLProperty::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaPolyDataMapper::New();
        }
#endif
      return vtkOpenGLPolyDataMapper::New();
      }
    if(strcmp(vtkclassname, "vtkRenderer") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaRenderer::New();
        }
#endif
      return vtkOpenGLRenderer::New();
      }
    if(strcmp(vtkclassname, "vtkTexture") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaTexture::New();
        }
#endif
      return vtkOpenGLTexture::New();
      }
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper2D") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaVolumeTextureMapper2D::New();
        }
#endif
      return vtkOpenGLVolumeTextureMapper2D::New();
      }
    if(strcmp(vtkclassname, "vtkVolumeRayCastMapper") == 0)
      {
#if defined(VTK_MANGLE_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaVolumeRayCastMapper::New();
        }
#endif
      return vtkOpenGLVolumeRayCastMapper::New();
      }
    }
#endif
        
  return 0;
}

void vtkGraphicsFactory::SetUseMesaClasses(int use)
{
  vtkUseMesaClassesCriticalSection.Lock();
  vtkGraphicsFactory::UseMesaClasses = use;
  vtkUseMesaClassesCriticalSection.Unlock();
}

int vtkGraphicsFactory::GetUseMesaClasses()
{
  return vtkGraphicsFactory::UseMesaClasses;
}

