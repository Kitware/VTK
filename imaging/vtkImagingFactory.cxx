/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagingFactory.cxx
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
#include "vtkImagingFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"

#ifdef VTK_USE_OGLR
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLImager.h"
#include "vtkOpenGLImageWindow.h"
#include "vtkXOpenGLTextMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#endif

#ifdef VTK_USE_MESA
#include "vtkMesaImageMapper.h"
#include "vtkMesaImager.h"
#include "vtkMesaImageWindow.h"
#include "vtkXMesaTextMapper.h"
#include "vtkMesaPolyDataMapper2D.h"
#endif

#ifdef _WIN32
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLImager.h"
#include "vtkWin32OpenGLImageWindow.h"
#include "vtkWin32OpenGLTextMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#else
 #ifdef VTK_USE_QUARTZ
  #include "vtkOpenGLImageMapper.h"
  #include "vtkOpenGLImager.h"
  #include "vtkOpenGLPolyDataMapper2D.h"
  #include "vtkQuartzTextMapper.h"
  #include "vtkQuartzImageWindow.h"
  #include "vtkQuartzImageMapper.h"
 #endif
#endif

const char *vtkImagingFactoryGetRenderLibrary()
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

vtkObject* vtkImagingFactory::CreateInstance(const char* vtkclassname )
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

  const char *rl = vtkImagingFactoryGetRenderLibrary();

#ifdef VTK_USE_OGLR
  if (!strcmp("OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkXOpenGLTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageWindow") == 0)
      {
      return vtkOpenGLImageWindow::New();
      }
    if(strcmp(vtkclassname, "vtkImager") == 0)
      {
      return vtkOpenGLImager::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
      return vtkOpenGLImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
      return vtkOpenGLPolyDataMapper2D::New();
      }
    }
#endif

#ifdef _WIN32
  if (!strcmp("Win32OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkWin32OpenGLTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageWindow") == 0)
      {
      return vtkWin32OpenGLImageWindow::New();
      }
    if(strcmp(vtkclassname, "vtkImager") == 0)
      {
      return vtkOpenGLImager::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
      return vtkOpenGLImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
      return vtkOpenGLPolyDataMapper2D::New();
      }
    }
#endif

#ifdef VTK_USE_QUARTZ
  if (!strcmp("QuartzOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkQuartzTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageWindow") == 0)
      {
      return vtkQuartzImageWindow::New();
      }
    if(strcmp(vtkclassname, "vtkImager") == 0)
      {
      return vtkOpenGLImager::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
      return vtkOpenGLImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
      return vtkOpenGLPolyDataMapper2D::New();
      }
    }
#endif


#ifdef VTK_USE_MESA
  if (!strcmp("Mesa",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkXMesaTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageWindow") == 0)
      {
      return vtkMesaImageWindow::New();
      }
    if(strcmp(vtkclassname, "vtkImager") == 0)
      {
      return vtkMesaImager::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
      return vtkMesaImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
      return vtkMesaPolyDataMapper2D::New();
      }
    }
#endif 
  vtkGenericWarningMacro("Attempting to create an OpenGL or Mesa based object with a VTK that is not linked/configured with Mesa/OpenGL.");
  abort();
  return 0;
}


