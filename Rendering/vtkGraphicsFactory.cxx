/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkObjectFactory.h"
#include "vtkGraphicsFactory.h"
#include "vtkToolkits.h"
#include "stdlib.h"
#include "vtkDebugLeaks.h"

#ifdef VTK_USE_OGLR
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLImageActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLVolumeRayCastMapper.h"
#endif

#ifdef VTK_USE_MESA
#include "vtkMesaActor.h"
#include "vtkMesaCamera.h"
#include "vtkMesaImageActor.h"
#include "vtkMesaLight.h"
#include "vtkMesaProperty.h"
#include "vtkMesaPolyDataMapper.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRenderWindow.h"
#include "vtkMesaTexture.h"
#include "vtkMesaVolumeTextureMapper2D.h"
#endif

#ifdef _WIN32
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLImageActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLVolumeRayCastMapper.h"
#include "vtkWin32RenderWindowInteractor.h"
#else
#ifdef VTK_USE_QUARTZ
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLImageActor.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkQuartzRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLVolumeRayCastMapper.h"
#include "vtkQuartzRenderWindowInteractor.h"
#else
#include "vtkXRenderWindowInteractor.h"
#endif
#endif

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
    else if (strcmp("Mesa",temp) && strcmp("OpenGL",temp) && 
	     strcmp("Win32OpenGL",temp))
      {
      vtkGenericWarningMacro(<<"VTK_RENDERER set to unsupported type:" << temp);
      temp = NULL;
      }
    }

  // if the environment variable is set to openGL and the user
  //  does not have opengl but they do have mesa, then use it
#ifndef VTK_USE_OGLR
#ifdef VTK_USE_MESA
  if ( temp != NULL )
    {
    if (!strcmp("OpenGL",temp))
      {
       temp = "Mesa";
      }
    }
#endif
#endif
  
  // if nothing is set then work down the list of possible renderers
  if ( !temp )
    {
#ifdef VTK_USE_MESA
    temp = "Mesa";
#endif
#ifdef VTK_USE_OGLR
    temp = "OpenGL";
#endif
#ifdef _WIN32
    temp = "Win32OpenGL";
#endif
#ifdef VTK_USE_QUARTZ
    temp = "QuartzOpenGL";
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
      return vtkXOpenGLRenderWindow::New();
      }
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
#else
#ifdef VTK_USE_QUARTZ
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkQuartzRenderWindowInteractor::New();
    }
  if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
    {
    return vtkQuartzRenderWindow::New();
    }

  #else
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkXRenderWindowInteractor::New();
    }
  #endif
#endif

#if defined(VTK_USE_OGLR) || defined(_WIN32) || defined(VTK_USE_QUARTZ)
  if (!strcmp("OpenGL",rl) || !strcmp("Win32OpenGL",rl) || !strcmp("QuartzOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkActor") == 0)
      {
      return vtkOpenGLActor::New();
      }
    if(strcmp(vtkclassname, "vtkCamera") == 0)
      {
      return vtkOpenGLCamera::New();
      }
    if(strcmp(vtkclassname, "vtkImageActor") == 0)
      {
      return vtkOpenGLImageActor::New();
      }
    if(strcmp(vtkclassname, "vtkLight") == 0)
      {
      return vtkOpenGLLight::New();
      }
    if(strcmp(vtkclassname, "vtkProperty") == 0)
      {
      return vtkOpenGLProperty::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper") == 0)
      {
      return vtkOpenGLPolyDataMapper::New();
      }
    if(strcmp(vtkclassname, "vtkRenderer") == 0)
      {
      return vtkOpenGLRenderer::New();
      }
    if(strcmp(vtkclassname, "vtkTexture") == 0)
      {
      return vtkOpenGLTexture::New();
      }
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper2D") == 0)
      {
      return vtkOpenGLVolumeTextureMapper2D::New();
      }
    if(strcmp(vtkclassname, "vtkVolumeRayCastMapper") == 0)
      {
      return vtkOpenGLVolumeRayCastMapper::New();
      }
    }
#endif
	
#ifdef VTK_USE_MESA
  if (!strcmp("Mesa",rl))
    {
    if(strcmp(vtkclassname, "vtkActor") == 0)
      {
      return vtkMesaActor::New();
      }
    if(strcmp(vtkclassname, "vtkCamera") == 0)
      {
      return vtkMesaCamera::New();
      }
    if(strcmp(vtkclassname, "vtkImageActor") == 0)
      {
      return vtkMesaImageActor::New();
      }
    if(strcmp(vtkclassname, "vtkLight") == 0)
      {
      return vtkMesaLight::New();
      }
    if(strcmp(vtkclassname, "vtkProperty") == 0)
      {
      return vtkMesaProperty::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper") == 0)
      {
      return vtkMesaPolyDataMapper::New();
      }
    if(strcmp(vtkclassname, "vtkRenderer") == 0)
      {
      return vtkMesaRenderer::New();
      }
    if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
      {
      return vtkMesaRenderWindow::New();
      }
    if(strcmp(vtkclassname, "vtkTexture") == 0)
      {
      return vtkMesaTexture::New();
      }
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper2D") == 0)
      {
      return vtkMesaVolumeTextureMapper2D::New();
      }
    if(strcmp(vtkclassname, "vtkVolumeRayCastMapper") == 0)
      {
      return vtkMesaVolumeRayCastMapper::New();
      }
    }
#endif
 vtkGenericWarningMacro("Attempting to create an OpenGL or Mesa based object with a VTK that is not linked/configured with Mesa/OpenGL.");
  abort();
  return 0;
}


