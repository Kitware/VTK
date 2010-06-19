/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"

#include "vtkGraphicsFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"
#include "vtkPainterPolyDataMapper.h"

// if using some sort of opengl, then include these files
#if defined(VTK_USE_OGLR) || defined(VTK_USE_OSMESA) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLClipPlanesPainter.h"
#include "vtkOpenGLCoincidentTopologyResolutionPainter.h"
#include "vtkOpenGLDisplayListPainter.h"
#include "vtkOpenGLGlyph3DMapper.h"
#include "vtkOpenGLImageActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLLightingPainter.h"
#include "vtkOpenGLPainterDeviceAdapter.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRepresentationPainter.h"
#include "vtkOpenGLScalarsToColorsPainter.h"
#include "vtkOpenGLTexture.h"
#endif

// Win32 specific stuff
#ifdef _WIN32
# ifndef VTK_USE_OGLR
#  include "vtkWin32OpenGLRenderWindow.h"
#  include "vtkWin32RenderWindowInteractor.h"
#  define VTK_DISPLAY_WIN32_OGL
# endif // VTK_USE_OGLR
#endif

// Apple OSX stuff
#ifdef VTK_USE_CARBON
# include "vtkCarbonRenderWindow.h"
# include "vtkCarbonRenderWindowInteractor.h"
# define VTK_DISPLAY_CARBON
#endif

#ifdef VTK_USE_COCOA
# include "vtkCocoaRenderWindow.h"
# include "vtkCocoaRenderWindowInteractor.h"
# define VTK_DISPLAY_COCOA
#endif

// X OpenGL stuff
#ifdef VTK_USE_OGLR
# include "vtkXRenderWindowInteractor.h"
# include "vtkXOpenGLRenderWindow.h"
# define VTK_DISPLAY_X11_OGL
#endif

// OSMESA OpenGL stuff
#ifdef VTK_USE_OSMESA
# include "vtkRenderWindowInteractor.h"
# include "vtkOSOpenGLRenderWindow.h"
//# define VTK_DISPLAY_X11_OGL
#endif

#if defined(VTK_USE_MANGLED_MESA)
#include "vtkMesaActor.h"
#include "vtkMesaCamera.h"
#include "vtkMesaClipPlanesPainter.h"
#include "vtkMesaCoincidentTopologyResolutionPainter.h"
#include "vtkMesaDisplayListPainter.h"
#include "vtkMesaImageActor.h"
#include "vtkMesaLight.h"
#include "vtkMesaLightingPainter.h"
#include "vtkMesaPainterDeviceAdapter.h"
#include "vtkMesaProperty.h"
#include "vtkMesaPolyDataMapper.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRepresentationPainter.h"
#include "vtkMesaScalarsToColorsPainter.h"
#include "vtkMesaTexture.h"
#include "vtkXMesaRenderWindow.h"
#endif

#include "vtkDummyGPUInfoList.h"
#ifdef VTK_USE_DIRECTX // Windows
# include "vtkDirectXGPUInfoList.h"
#else
# ifdef VTK_USE_CORE_GRAPHICS // Mac
#  include "vtkCoreGraphicsGPUInfoList.h"
# endif
#endif
#ifdef VTK_USE_NVCONTROL // Linux and X server extensions queries
# include "vtkXGPUInfoList.h"
#endif

#include "vtkCriticalSection.h"

#include "stdlib.h"

static vtkSimpleCriticalSection vtkUseMesaClassesCriticalSection;
static vtkSimpleCriticalSection vtkOffScreenOnlyModeCriticalSection;
int vtkGraphicsFactory::UseMesaClasses = 0;

#ifdef VTK_USE_OFFSCREEN
int vtkGraphicsFactory::OffScreenOnlyMode = 1;
#else
int vtkGraphicsFactory::OffScreenOnlyMode = 0;
#endif

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
#if defined(VTK_DISPLAY_X11_OGL) || defined(VTK_USE_OSMESA)
    temp = "OpenGL";
#endif
#ifdef VTK_DISPLAY_WIN32_OGL
    temp = "Win32OpenGL";
#endif
#ifdef VTK_DISPLAY_CARBON
    temp = "CarbonOpenGL";
#endif
#ifdef VTK_DISPLAY_COCOA
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
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkXMesaRenderWindow::New();
        }
#endif
      return vtkXOpenGLRenderWindow::New();
      }
    }
  if ( !vtkGraphicsFactory::GetOffScreenOnlyMode() )
    {
    if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
      {
      return vtkXRenderWindowInteractor::New();
      }
    }
#endif

  if(strcmp(vtkclassname, "vtkGPUInfoList") == 0)
      {
#ifdef VTK_USE_DIRECTX // Windows
      return vtkDirectXGPUInfoList::New();     
#else
# ifdef VTK_USE_CORE_GRAPHICS // Mac
      return vtkCoreGraphicsGPUInfoList::New();
# else
#  ifdef VTK_USE_NVCONTROL // X11
      return vtkXGPUInfoList::New();
#  else
      return vtkDummyGPUInfoList::New();
#  endif
# endif
#endif
      }

#if defined(VTK_USE_OSMESA)
  if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
    {
    return vtkOSOpenGLRenderWindow::New();
    }
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return 0; // there is no interactor with OSMesa
    }
#endif

#ifdef VTK_DISPLAY_WIN32_OGL
  if ( !vtkGraphicsFactory::GetOffScreenOnlyMode() )
    {
    if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
      {
      return vtkWin32RenderWindowInteractor::New();
      }
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
  if ( !vtkGraphicsFactory::GetOffScreenOnlyMode() )
    {
    if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
      {
      return vtkCarbonRenderWindowInteractor::New();
      }
    }
  if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
    {
    return vtkCarbonRenderWindow::New();
    }
#endif
#ifdef VTK_USE_COCOA
  if ( !vtkGraphicsFactory::GetOffScreenOnlyMode() )
    {
    if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
      {
      return vtkCocoaRenderWindowInteractor::New();
      }
    }
  if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
    {
    return vtkCocoaRenderWindow::New();
    }
#endif

#if defined(VTK_USE_OGLR) || defined(VTK_USE_OSMESA) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
  if (!strcmp("OpenGL",rl) || !strcmp("Win32OpenGL",rl) || !strcmp("CarbonOpenGL",rl) || !strcmp("CocoaOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkActor") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaActor::New();
        }
#endif
      return vtkOpenGLActor::New();
      }
    if(strcmp(vtkclassname, "vtkCamera") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaCamera::New();
        }
#endif
      return vtkOpenGLCamera::New();
      }
    if(strcmp(vtkclassname, "vtkImageActor") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaImageActor::New();
        }
#endif
      return vtkOpenGLImageActor::New();
      }
    if(strcmp(vtkclassname, "vtkLight") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaLight::New();
        }
#endif
      return vtkOpenGLLight::New();
      }
    if(strcmp(vtkclassname, "vtkProperty") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaProperty::New();
        }
#endif
      return vtkOpenGLProperty::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper") == 0)
      {
      return vtkPainterPolyDataMapper::New();
      }
    if (strcmp(vtkclassname, "vtkPainterDeviceAdapter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaPainterDeviceAdapter::New();
        }
#endif
      return vtkOpenGLPainterDeviceAdapter::New();
      }
    if (strcmp(vtkclassname, "vtkScalarsToColorsPainter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaScalarsToColorsPainter::New();
        }
#endif
      return vtkOpenGLScalarsToColorsPainter::New();
      }
    if (strcmp(vtkclassname, "vtkClipPlanesPainter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaClipPlanesPainter::New();
        }
#endif
      return vtkOpenGLClipPlanesPainter::New();
      }
    if (strcmp(vtkclassname, "vtkCoincidentTopologyResolutionPainter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaCoincidentTopologyResolutionPainter::New();
        }
#endif
      return vtkOpenGLCoincidentTopologyResolutionPainter::New();
      }
    if (strcmp(vtkclassname, "vtkDisplayListPainter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaDisplayListPainter::New();
        }
#endif
      return vtkOpenGLDisplayListPainter::New();
      }
    if (strcmp(vtkclassname, "vtkLightingPainter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaLightingPainter::New();
        }
#endif
      return vtkOpenGLLightingPainter::New();
      }
    if (strcmp(vtkclassname, "vtkRepresentationPainter") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaRepresentationPainter::New();
        }
#endif
      return vtkOpenGLRepresentationPainter::New();
      }
    if(strcmp(vtkclassname, "vtkRenderer") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaRenderer::New();
        }
#endif
      return vtkOpenGLRenderer::New();
      }
    if(strcmp(vtkclassname, "vtkTexture") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return vtkMesaTexture::New();
        }
#endif
      return vtkOpenGLTexture::New();
      }
    if(strcmp(vtkclassname, "vtkGlyph3DMapper") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::UseMesaClasses )
        {
        return NULL;
        }
#endif
      return vtkOpenGLGlyph3DMapper::New();
      }
    }
#endif
        
  return 0;
}

//----------------------------------------------------------------------------
void vtkGraphicsFactory::SetUseMesaClasses(int use)
{
  vtkUseMesaClassesCriticalSection.Lock();
  vtkGraphicsFactory::UseMesaClasses = use;
  vtkUseMesaClassesCriticalSection.Unlock();
}

//----------------------------------------------------------------------------
int vtkGraphicsFactory::GetUseMesaClasses()
{
  return vtkGraphicsFactory::UseMesaClasses;
}

//----------------------------------------------------------------------------
void vtkGraphicsFactory::SetOffScreenOnlyMode(int use)
{
  vtkOffScreenOnlyModeCriticalSection.Lock();
  vtkGraphicsFactory::OffScreenOnlyMode = use;
  vtkOffScreenOnlyModeCriticalSection.Unlock();
}

//----------------------------------------------------------------------------
int vtkGraphicsFactory::GetOffScreenOnlyMode()
{
  return vtkGraphicsFactory::OffScreenOnlyMode;
}

//----------------------------------------------------------------------------
void vtkGraphicsFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
