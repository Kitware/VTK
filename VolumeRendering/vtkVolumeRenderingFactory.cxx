/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRenderingFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObjectFactory.h"

#include "vtkVolumeRenderingFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"

// if using some sort of opengl, then include these files
#if defined(VTK_USE_OGLR) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLRayCastImageDisplayHelper.h"
#endif

#if defined(VTK_USE_MANGLED_MESA)
#include "vtkMesaRayCastImageDisplayHelper.h"
#include "vtkMesaVolumeTextureMapper2D.h"
#endif

#include "vtkCriticalSection.h"

#include "stdlib.h"

vtkCxxRevisionMacro(vtkVolumeRenderingFactory, "1.2");
vtkStandardNewMacro(vtkVolumeRenderingFactory);


vtkObject* vtkVolumeRenderingFactory::CreateInstance(const char* vtkclassname )
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
  

#if defined(VTK_USE_OGLR) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
  if (!strcmp("OpenGL",rl) || !strcmp("Win32OpenGL",rl) || !strcmp("CarbonOpenGL",rl) || !strcmp("CocoaOpenGL",rl))
    {
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper2D") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::GetUseMesaClasses() )
        {
        return vtkMesaVolumeTextureMapper2D::New();
        }
#endif
      return vtkOpenGLVolumeTextureMapper2D::New();
      }
    if(strcmp(vtkclassname, "vtkRayCastImageDisplayHelper") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::GetUseMesaClasses() )
        {
        return vtkMesaRayCastImageDisplayHelper::New();
        }
#endif
      return vtkOpenGLRayCastImageDisplayHelper::New();
      }
    }
#endif
        
  return 0;
}

//----------------------------------------------------------------------------
void vtkVolumeRenderingFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
