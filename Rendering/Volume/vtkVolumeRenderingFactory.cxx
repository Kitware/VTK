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
#include "vtkGraphicsFactory.h"

// if using some sort of opengl, then include these files
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkOpenGLHAVSVolumeMapper.h"
#include "vtkOpenGLProjectedAAHexahedraMapper.h"
#include "vtkOpenGLProjectedTetrahedraMapper.h"
#include "vtkOpenGLRayCastImageDisplayHelper.h"
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkOpenGLVolumeTextureMapper3D.h"

#include "vtkCriticalSection.h"

#include "stdlib.h"

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
  
//  if (!strcmp("OpenGL",rl) || !strcmp("Win32OpenGL",rl) || !strcmp("CarbonOpenGL",rl) || !strcmp("CocoaOpenGL",rl))
    {
    // GPU Ray Cast Mapper
    if(strcmp(vtkclassname, "vtkGPUVolumeRayCastMapper") == 0)
      {
      return vtkOpenGLGPUVolumeRayCastMapper::New();
      }

    // Projected axis-aligned hexahedra mapper
    if(strcmp(vtkclassname, "vtkProjectedAAHexahedraMapper") == 0)
      {
      return vtkOpenGLProjectedAAHexahedraMapper::New();
      }

    // Projected Tetrahedra Mapper
    if(strcmp(vtkclassname, "vtkProjectedTetrahedraMapper") == 0)
      {
      return vtkOpenGLProjectedTetrahedraMapper::New();
      }

    // HAVS Mapper
    if(strcmp(vtkclassname, "vtkHAVSVolumeMapper") == 0)
      {
      return vtkOpenGLHAVSVolumeMapper::New();
      }

    // 2D Volume Texture Mapper
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper2D") == 0)
      {
      return vtkOpenGLVolumeTextureMapper2D::New();
      }
    
    // 3D Volume Texture Mapper
    if(strcmp(vtkclassname, "vtkVolumeTextureMapper3D") == 0)
      {
      return vtkOpenGLVolumeTextureMapper3D::New();
      }
    
    // Ray Cast Image Display Helper
    if(strcmp(vtkclassname, "vtkRayCastImageDisplayHelper") == 0)
      {
      return vtkOpenGLRayCastImageDisplayHelper::New();
      }
    }
        
  return 0;
}

//----------------------------------------------------------------------------
void vtkVolumeRenderingFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
