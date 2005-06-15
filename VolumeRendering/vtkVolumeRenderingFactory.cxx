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
#include "vtkVolumeRenderingToolkit.h"
#include "vtkDebugLeaks.h"

// if using some sort of opengl, then include these files
#if defined(VTK_USE_OGLR) || defined(_WIN32) || defined(VTK_USE_COCOA) || defined(VTK_USE_CARBON)
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLRayCastImageDisplayHelper.h"
#ifdef VTK_USE_VOLUMETEXTUREMAPPER3D    
#include "vtkOpenGLVolumeTextureMapper3D.h"
#endif
#endif

#if defined(VTK_USE_MANGLED_MESA)
#include "vtkMesaRayCastImageDisplayHelper.h"
#include "vtkMesaVolumeTextureMapper2D.h"
#endif

#include "vtkCriticalSection.h"

#include "stdlib.h"

vtkCxxRevisionMacro(vtkVolumeRenderingFactory, "1.4");
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
    // 2D Volume Texture Mapper
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
    
    // 3D Volume Texture Mapper
    // Conditionally included based on platform (just win32 now)
#ifdef VTK_USE_VOLUMETEXTUREMAPPER3D    
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper3D") == 0)
      {
#if defined(VTK_USE_MANGLED_MESA)
      if ( vtkGraphicsFactory::GetUseMesaClasses() )
        {
        vtkErrorMacro("No support for mesa in vtkVolumeTextureMapper3D");
        return 0;
        }
#endif
      return vtkOpenGLVolumeTextureMapper3D::New();
      }
#endif
    
    // Ray Cast Image Display Helper
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
