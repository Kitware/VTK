/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagingFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkObjectFactory.h"
#include "vtkImagingFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"

#if defined(VTK_USE_OGLR) || defined(VTK_USE_OSMESA)
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkOpenGLFreeTypeTextMapper.h"
#endif

#if defined(VTK_USE_MANGLED_MESA)
#include "vtkMesaImageMapper.h"
#include "vtkMesaPolyDataMapper2D.h"
#include "vtkMesaFreeTypeTextMapper.h"
#endif

#ifdef _WIN32
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkOpenGLFreeTypeTextMapper.h"
#else
#ifdef VTK_USE_QUARTZ
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkQuartzImageMapper.h"
#include "vtkOpenGLFreeTypeTextMapper.h"
#endif
#endif

#ifdef VTK_USE_CARBON
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkOpenGLFreeTypeTextMapper.h"
#endif

#ifdef VTK_USE_COCOA
#include "vtkOpenGLImageMapper.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkOpenGLFreeTypeTextMapper.h"
#endif

#include "vtkCriticalSection.h"
static vtkSimpleCriticalSection vtkUseMesaClassesCriticalSection;
int vtkImagingFactory::UseMesaClasses = 0;

vtkStandardNewMacro(vtkImagingFactory);

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
  
  // if nothing is set then work down the list of possible renderers
  if ( !temp )
    {
#if defined(VTK_USE_OGLR)  || defined(VTK_USE_OSMESA)
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

#if defined(VTK_USE_OGLR) || defined(VTK_USE_OSMESA)
  if (!strcmp("OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkImagingFactory::UseMesaClasses )
        {
        return vtkMesaFreeTypeTextMapper::New();
        }
#endif
      return vtkOpenGLFreeTypeTextMapper::New();
      }
    if(strcmp(vtkclassname, "vtkImageMapper") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkImagingFactory::UseMesaClasses )
        {
        return vtkMesaImageMapper::New();
        }
#endif
      return vtkOpenGLImageMapper::New();
      }
    if(strcmp(vtkclassname, "vtkPolyDataMapper2D") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkImagingFactory::UseMesaClasses )
        {
        return vtkMesaPolyDataMapper2D::New();
        }
#endif
      return vtkOpenGLPolyDataMapper2D::New();
      }
    }
#endif

#ifdef _WIN32
  if (!strcmp("Win32OpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkOpenGLFreeTypeTextMapper::New();
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

#ifdef VTK_USE_CARBON
  if (!strcmp("CarbonOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkOpenGLFreeTypeTextMapper::New();
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
#ifdef VTK_USE_COCOA
  if (!strcmp("CocoaOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkTextMapper") == 0)
      {
      return vtkOpenGLFreeTypeTextMapper::New();
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

  return 0;
}

void vtkImagingFactory::SetUseMesaClasses(int use)
{
  vtkUseMesaClassesCriticalSection.Lock();
  vtkImagingFactory::UseMesaClasses = use;
  vtkUseMesaClassesCriticalSection.Unlock();
}

int vtkImagingFactory::GetUseMesaClasses()
{
  return vtkImagingFactory::UseMesaClasses;
}

//----------------------------------------------------------------------------
void vtkImagingFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
