/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLHardwareSupport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLHardwareSupport.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkgl.h"

vtkStandardNewMacro(vtkOpenGLHardwareSupport);

vtkCxxSetObjectMacro(vtkOpenGLHardwareSupport, ExtensionManager, vtkOpenGLExtensionManager);


// ----------------------------------------------------------------------------
vtkOpenGLHardwareSupport::vtkOpenGLHardwareSupport()
{
  this->ExtensionManager = NULL;
}

// ----------------------------------------------------------------------------
vtkOpenGLHardwareSupport::~vtkOpenGLHardwareSupport()
{
  this->SetExtensionManager(NULL);
}

// ----------------------------------------------------------------------------
int vtkOpenGLHardwareSupport::GetNumberOfFixedTextureUnits()
{
  if ( ! vtkgl::MultiTexCoord2d || ! vtkgl::ActiveTexture )
    {
    if(!this->ExtensionManagerSet())
      {
      vtkWarningMacro(<<"extension manager not set. Return 1.");
      return 1;
      }

    // multitexture is a core feature of OpenGL 1.3.
    // multitexture is an ARB extension of OpenGL 1.2.1
    int supports_GL_1_3 = this->ExtensionManager->ExtensionSupported( "GL_VERSION_1_3" );
    int supports_GL_1_2_1 = this->ExtensionManager->ExtensionSupported("GL_VERSION_1_2");
    int supports_ARB_mutlitexture =
      this->ExtensionManager->ExtensionSupported("GL_ARB_multitexture");

    if(supports_GL_1_3)
      {
      this->ExtensionManager->LoadExtension("GL_VERSION_1_3");
      }
    else if(supports_GL_1_2_1 && supports_ARB_mutlitexture)
      {
      this->ExtensionManager->LoadExtension("GL_VERSION_1_2");
      this->ExtensionManager->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    else
      {
      return 1;
      }
    }

  GLint numSupportedTextures = 1;
  glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);

  return numSupportedTextures;
}

// ----------------------------------------------------------------------------
// Description:
// Return the total number of texture image units accessible by a shader
// program.
int vtkOpenGLHardwareSupport::GetNumberOfTextureUnits()
{
  int result=1;
  // vtkgl::MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB is defined in
  // GL_ARB_vertex_shader
  // vtkgl::MAX_COMBINED_TEXTURE_IMAGE_UNITS is defined in OpenGL 2.0

  // test for a function defined both by GL_ARB_vertex_shader and OpenGL 2.0

  bool supports_shaders=vtkgl::GetActiveAttrib!=0;

  if(!supports_shaders)
    {
    if(!this->ExtensionManagerSet())
      {
      vtkWarningMacro(<<"extension manager not set. Return 1.");
      }
    else
      {
      if(this->ExtensionManager->ExtensionSupported("GL_VERSION_2_0"))
        {
        this->ExtensionManager->LoadExtension("GL_VERSION_2_0");
        supports_shaders=true;
        }
      else
        {
        supports_shaders=
          this->ExtensionManager->ExtensionSupported("GL_ARB_vertex_shader")==1;
        if(supports_shaders)
          {
          this->ExtensionManager->LoadCorePromotedExtension(
            "GL_ARB_vertex_shader");
          }
        }
      }
    }

  if(supports_shaders)
    {
    GLint value;
    glGetIntegerv(vtkgl::MAX_COMBINED_TEXTURE_IMAGE_UNITS,&value);
    result=static_cast<int>(value);
    }
  return result;
}


// ----------------------------------------------------------------------------
bool vtkOpenGLHardwareSupport::GetSupportsMultiTexturing()
{
  if ( ! vtkgl::MultiTexCoord2d || ! vtkgl::ActiveTexture )
    {
    if(!ExtensionManagerSet())
      {
      return false;
      }

    // multitexture is a core feature of OpenGL 1.3.
    // multitexture is an ARB extension of OpenGL 1.2.1
    int supports_GL_1_3 = this->ExtensionManager->ExtensionSupported( "GL_VERSION_1_3" );
    int supports_GL_1_2_1 = this->ExtensionManager->ExtensionSupported("GL_VERSION_1_2");
    int supports_ARB_mutlitexture =
      this->ExtensionManager->ExtensionSupported("GL_ARB_multitexture");

    if(supports_GL_1_3 || supports_GL_1_2_1 || supports_ARB_mutlitexture)
      {
      return true;
      }

    return false;
    }
  else
    {
    return true;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLHardwareSupport::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << this->ExtensionManager << endl;
  this->Superclass::PrintSelf(os,indent);
}

bool vtkOpenGLHardwareSupport::ExtensionManagerSet()
{
  if(!this->ExtensionManager)
    {
    vtkErrorMacro("" << this->GetClassName() << ": requires an ExtensionManager set.");
    return false;
    }
  if(!this->ExtensionManager->GetRenderWindow())
    {
    vtkErrorMacro("" << this->GetClassName() << ": requires an ExtensionManager with Render Window set.");
    return false;
    }
  return true;
}
