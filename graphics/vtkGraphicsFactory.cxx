/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkObjectFactory.h"
#include "vtkGraphicsFactory.h"

#ifdef VTK_USE_OGLR
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLProjectedPolyDataRayBounder.h"
#include "vtkXRenderWindowInteractor.h"
#endif

#ifdef VTK_USE_MESA
#include "vtkMesaActor.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkMesaProperty.h"
#include "vtkMesaPolyDataMapper.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRenderWindow.h"
#include "vtkMesaTexture.h"
#include "vtkMesaVolumeTextureMapper2D.h"
#include "vtkMesaProjectedPolyDataRayBounder.h"
#include "vtkXRenderWindowInteractor.h"
#endif

#ifdef _WIN32
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLProjectedPolyDataRayBounder.h"
#include "vtkWin32RenderWindowInteractor.h"
#endif

char *vtkGraphicsFactoryGetRenderLibrary()
{
  char *temp;
  
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

  char *rl = vtkGraphicsFactoryGetRenderLibrary();
  
#ifdef VTK_USE_OGLR
  if (!strcmp("OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkRenderWindow") == 0)
      {
      return vtkOpenGLRenderWindow::New();
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
  if(strcmp(vtkclassname, "vtkRenderWindowInteractor") == 0)
    {
    return vtkXRenderWindowInteractor::New();
    }
#endif

#if defined(VTK_USE_OGLR) || defined(_WIN32)
  if (!strcmp("OpenGL",rl) || !strcmp("Win32OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkActor") == 0)
      {
      return vtkOpenGLActor::New();
      }
    if(strcmp(vtkclassname, "vtkCamera") == 0)
      {
      return vtkOpenGLCamera::New();
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
    if(strcmp(vtkclassname, "vtkProjectedPolyDataRayBounder") == 0)
      {
      return vtkOpenGLProjectedPolyDataRayBounder::New();
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
    if(strcmp(vtkclassname, "vtkProjectedPolyDataRayBounder") == 0)
      {
      return vtkMesaProjectedPolyDataRayBounder::New();
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
    }
#endif

  
  return 0;
}


