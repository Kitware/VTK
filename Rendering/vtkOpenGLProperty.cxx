/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif

#include "vtkObjectFactory.h"
#include "vtkToolkits.h"  // for VTK_USE_GL2PS
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLTexture.h"
#include "vtkTexture.h"

#ifdef VTK_USE_GL2PS
#include "gl2ps.h"
#include "vtkGL2PSExporter.h"
#endif // VTK_USE_GL2PS

#include "vtkgl.h" // vtkgl namespace

#include "vtkShader2.h"
#include "vtkShaderProgram2.h"
#include "vtkUniformVariables.h"
#include "vtkShader2Collection.h"
#include <assert.h>

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLProperty, "1.49");
vtkStandardNewMacro(vtkOpenGLProperty);
#endif

extern const char *vtkOpenGLProperty_fs;


vtkOpenGLProperty::vtkOpenGLProperty()
{
  this->Shader=0;
}

vtkOpenGLProperty::~vtkOpenGLProperty()
{
  if(this->Shader!=0)
    {
    this->Shader->Delete();
    }
}

//-----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::Render(vtkActor *anActor,
                             vtkRenderer *ren)
{
  int i;
  GLenum method;
  float Info[4];
  GLenum Face;
  double  color[4];
  
  // unbind any textures for starters
  vtkOpenGLRenderer *oRenderer=static_cast<vtkOpenGLRenderer *>(ren);
  vtkShaderProgram2 *prog=oRenderer->GetShaderProgram();
  
  if(prog!=0 && prog->HasFragmentShaders() )
    {
    if(this->Shader==0)
      {
      this->Shader=vtkShader2::New();
      this->Shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
      this->Shader->SetSourceCode(vtkOpenGLProperty_fs);
      vtkUniformVariables *v=this->Shader->GetUniformVariables();
      int value;
      value=0;
      v->SetUniformi("useTexture",1,&value);
      value=0;
      v->SetUniformi("uTexture",1,&value);
      }
    prog->GetShaders()->AddItem(this->Shader);
    assert("check: prog is initialized" && prog->GetContext()!=0);
    this->Shader->SetContext(prog->GetContext());
    prog->Use();
    if(!prog->IsValid())
      {
      vtkErrorMacro(<<prog->GetLastValidateLog());
      }
    }
 
  glDisable(GL_TEXTURE_2D);

  // disable alpha testing (this may have been enabled
  // by another actor in OpenGLTexture)
  glDisable (GL_ALPHA_TEST);

  glDisable(GL_COLOR_MATERIAL);

  Face = GL_FRONT_AND_BACK;
  // turn on/off backface culling
  if ( ! this->BackfaceCulling && ! this->FrontfaceCulling)
    {
    glDisable (GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  else if ( this->BackfaceCulling)
    {
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    }
  else //if both front & back culling on, will fall into backface culling
    { //if you really want both front and back, use the Actor's visibility flag
    glCullFace (GL_FRONT);
    glEnable (GL_CULL_FACE);
    }

  Info[3] = this->Opacity;
  
  double factor;
  GLint alphaBits;
  glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
  
  // Dealing with having a correct alpha (none square) in the framebuffer
  // is only required if there is an alpha component in the framebuffer
  // (doh...) and if we cannot deal directly with BlendFuncSeparate.
  if(vtkgl::BlendFuncSeparate==0 && alphaBits>0)
    {
    factor=this->Opacity;
    }
  else
    {
    factor=1;
    }
  
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(factor*this->Ambient*this->AmbientColor[i]);
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(factor*this->Diffuse*this->DiffuseColor[i]);
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(factor*this->Specular*this->SpecularColor[i]);
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = static_cast<float>(this->SpecularPower);
  glMaterialfv( Face, GL_SHININESS, Info );

  // set interpolation 
  switch (this->Interpolation) 
    {
    case VTK_FLAT:
      method = GL_FLAT;
      break;
    case VTK_GOURAUD:
    case VTK_PHONG:
      method = GL_SMOOTH;
      break;
    default:
      method = GL_SMOOTH;
      break;
    }
  
  glShadeModel(method);

  // The material properties set above are used if shading is
  // enabled. This color set here is used if shading is 
  // disabled. Shading is disabled in the 
  // vtkOpenGLPolyDataMapper::Draw() method if points or lines
  // are encountered without normals. 
  this->GetColor( color );
  color[0] *= factor;
  color[1] *= factor;
  color[2] *= factor;
  color[3] = this->Opacity;

  glColor4dv( color );

  // Set the PointSize
  glPointSize (this->PointSize);

  // Set the LineWidth
  glLineWidth (this->LineWidth);

  // Set pointsize and linewidth for GL2PS output.
#ifdef VTK_USE_GL2PS
  gl2psPointSize(this->PointSize*
                 vtkGL2PSExporter::GetGlobalPointSizeFactor());
  gl2psLineWidth(this->LineWidth*
                 vtkGL2PSExporter::GetGlobalLineWidthFactor());
#endif // VTK_USE_GL2PS

  // Set the LineStipple
  if (this->LineStipplePattern != 0xFFFF)
    {
    glEnable (GL_LINE_STIPPLE);
#ifdef VTK_USE_GL2PS
    gl2psEnable(GL2PS_LINE_STIPPLE);
#endif // VTK_USE_GL2PS
    glLineStipple (this->LineStippleRepeatFactor, this->LineStipplePattern);
    }
  else
    {
    // still need to set this although we are disabling.  else the ATI X1600
    // (for example) still manages to stipple under certain conditions.
    glLineStipple (this->LineStippleRepeatFactor, this->LineStipplePattern);
    glDisable (GL_LINE_STIPPLE);
#ifdef VTK_USE_GL2PS
    gl2psDisable(GL2PS_LINE_STIPPLE);
#endif // VTK_USE_GL2PS
    }

  // render any textures.
  vtkIdType numTextures = this->GetNumberOfTextures();
  if (numTextures > 0)
    {
    this->LoadMultiTexturingExtensions(ren);
    if (vtkgl::ActiveTexture)
      {
      GLint numSupportedTextures;
      glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
      for (vtkIdType t = 0; t < numTextures; t++)
        {
        int texture_unit = this->GetTextureUnitAtIndex(t);
        if (texture_unit >= numSupportedTextures || texture_unit < 0)
          {
          vtkErrorMacro("Hardware does not support the number of textures defined.");
          continue;
          }

        vtkgl::ActiveTexture(vtkgl::TEXTURE0 + texture_unit);
        this->GetTextureAtIndex(t)->Render(ren);
        }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
      }
    else
      {
      this->GetTextureAtIndex(0)->Render(ren);
      }
    }

  this->Superclass::Render(anActor, ren);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::PostRender(vtkActor *actor,
                                   vtkRenderer *renderer)
{
  vtkOpenGLRenderer *oRenderer=static_cast<vtkOpenGLRenderer *>(renderer);
  vtkShaderProgram2 *prog=oRenderer->GetShaderProgram();
  if(prog!=0 && prog->HasFragmentShaders())
    {
    prog->Restore();
    prog->GetShaders()->RemoveItem(this->Shader);
    }
  this->Superclass::PostRender(actor, renderer);

  // render any textures.
  vtkIdType numTextures = this->GetNumberOfTextures();
  if (numTextures > 0 && vtkgl::ActiveTexture)
    {
    GLint numSupportedTextures;
    glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
    for (vtkIdType i = 0; i < numTextures; i++)
      {
      int texture_unit = this->GetTextureUnitAtIndex(i);
      if (texture_unit >= numSupportedTextures || texture_unit < 0)
        {
        vtkErrorMacro("Hardware does not support the number of textures defined.");
        continue;
        }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 + texture_unit);
      // Disable any possible texture.  Wouldn't having a PostRender on
      // vtkTexture be better?
      glDisable(GL_TEXTURE_1D);
      glDisable(GL_TEXTURE_2D);
      glDisable(vtkgl::TEXTURE_3D);
      glDisable(vtkgl::TEXTURE_RECTANGLE_ARB);
      glDisable(vtkgl::TEXTURE_CUBE_MAP);
      }
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }
}

//-----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::BackfaceRender(vtkActor *vtkNotUsed(anActor),
                             vtkRenderer *vtkNotUsed(ren))
{
  int i;
  float Info[4];
  GLenum Face;

  Face = GL_BACK;

  Info[3] = this->Opacity;

  double factor;
  GLint alphaBits;
  glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
  
  // Dealing with having a correct alpha (none square) in the framebuffer
  // is only required if there is an alpha component in the framebuffer
  // (doh...) and if we cannot deal directly with BlendFuncSeparate.
  if(vtkgl::BlendFuncSeparate==0 && alphaBits>0)
    {
    factor=this->Opacity;
    }
  else
    {
    factor=1;
    }
  
  for (i=0; i < 3; i++) 
    {
    Info[i] = 
      static_cast<float>(factor*this->Ambient*this->AmbientColor[i]);
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = 
      static_cast<float>(factor*this->Diffuse*this->DiffuseColor[i]);
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = 
      static_cast<float>(factor*this->Specular*this->SpecularColor[i]);
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = static_cast<float>(this->SpecularPower);
  glMaterialfv( Face, GL_SHININESS, Info );

}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::LoadMultiTexturingExtensions(vtkRenderer* ren)
{
  if ( ! vtkgl::MultiTexCoord2d || ! vtkgl::ActiveTexture )
    {
    vtkOpenGLExtensionManager* extensions = vtkOpenGLExtensionManager::New();
    extensions->SetRenderWindow( ren->GetRenderWindow() );

    // multitexture is a core feature of OpenGL 1.3.
    // multitexture is an ARB extension of OpenGL 1.2.1
    int supports_GL_1_3 = extensions->ExtensionSupported( "GL_VERSION_1_3" );
    int supports_GL_1_2_1 = extensions->ExtensionSupported("GL_VERSION_1_2");
    int supports_ARB_mutlitexture = 
      extensions->ExtensionSupported("GL_ARB_multitexture");
    
    if(supports_GL_1_3)
      {
      extensions->LoadExtension("GL_VERSION_1_3");
      }
    else if(supports_GL_1_2_1 && supports_ARB_mutlitexture)
      {
      extensions->LoadExtension("GL_VERSION_1_2");
      extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    extensions->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::ReleaseGraphicsResources(vtkWindow *win)
{
  // release any textures.
  vtkIdType numTextures = this->GetNumberOfTextures();
  if (win && numTextures > 0 && vtkgl::ActiveTexture)
    {
    GLint numSupportedTextures;
    glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
    for (vtkIdType i = 0; i < numTextures; i++)
      {
      if (vtkOpenGLTexture::SafeDownCast(this->GetTextureAtIndex(i))->GetIndex() == 0)
        {
        continue;
        }
      int texture_unit = this->GetTextureUnitAtIndex(i);
      if (texture_unit >= numSupportedTextures || texture_unit < 0)
        {
        vtkErrorMacro("Hardware does not support the texture unit " << texture_unit << ".");
        continue;
        }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 + texture_unit);
      this->GetTextureAtIndex(i)->ReleaseGraphicsResources(win);
      }
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }
  else if (numTextures > 0 && vtkgl::ActiveTexture)
    {
    for (vtkIdType i = 0; i < numTextures; i++)
      {
      this->GetTextureAtIndex(i)->ReleaseGraphicsResources(win);
      }
    }

  this->Superclass::ReleaseGraphicsResources(win);
  if(this->Shader!=0)
    {
    this->Shader->ReleaseGraphicsResources();
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
