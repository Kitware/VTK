/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagingFactory.cxx
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
#include "vtkImagingFactory.h"

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
#include "vtkWin32TextMapper.h"
#include "vtkWin32ImageWindow.h"
#include "vtkWin32ImageMapper.h"
#include "vtkWin32PolyDataMapper2D.h"
#else
#include "vtkXTextMapper.h"
#include "vtkXImageWindow.h"
#include "vtkXImageMapper.h"
#include "vtkXPolyDataMapper2D.h"
#endif

char *vtkImagingFactoryGetRenderLibrary()
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

vtkObject* vtkImagingFactory::CreateInstance(const char* vtkclassname )
{
  // first check the object factory
  vtkObject *ret = vtkObjectFactory::CreateInstance(vtkclassname);
  if (ret)
    {
    return ret;
    }

  char *rl = vtkImagingFactoryGetRenderLibrary();

#ifdef VTK_USE_OGLR
#ifndef VTK_USE_NATIVE_IMAGING
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkXTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageWindow") == 0)
      {
      return vtkXImageWindow::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
      return vtkXImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
      return vtkXPolyDataMapper2D::New();
      }
#else
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
#endif

#ifdef _WIN32
#ifndef VTK_USE_NATIVE_IMAGING
  if(strcmp(vtkclassname, "vtkTextMapper") == 0)
    {
    return vtkWin32TextMapper::New();
    }
  if(strcmp(vtkclassname, "vtkImageWindow") == 0)
    {
    return vtkWin32ImageWindow::New();
    }
  if(strcmp(vtkclassname, "vtkImageMapper") == 0)
    {
    return vtkWin32ImageMapper::New();
    }
  if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
    {
    return vtkWin32PolyDataMapper2D::New();
    }
#else
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
#endif

#ifdef VTK_USE_MESA
#ifndef VTK_USE_NATIVE_IMAGING
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkXTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageWindow") == 0)
      {
      return vtkXImageWindow::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
      return vtkXImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
      return vtkXPolyDataMapper2D::New();
      }
#else
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
#endif  
  return 0;
}


