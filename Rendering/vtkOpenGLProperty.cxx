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
#include "vtkTextureUnitManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkXMLMaterial.h"
#include "vtkXMLShader.h"
#include "vtkGLSLShaderDeviceAdapter2.h"

#include <assert.h>

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLProperty, "1.57");
vtkStandardNewMacro(vtkOpenGLProperty);
#endif

extern const char *vtkOpenGLPropertyDefaultMain_vs;
extern const char *vtkOpenGLPropertyDefaultMain_fs;
extern const char *vtkOpenGLPropertyDefaultPropFunc_vs;
extern const char *vtkOpenGLPropertyDefaultPropFunc_fs;

vtkCxxSetObjectMacro(vtkOpenGLProperty,Shader2Collection,vtkShader2Collection);

vtkOpenGLProperty::vtkOpenGLProperty()
{
  this->Shader2Collection=0;
  
  this->CachedShaderProgram2=0;
  this->LastCachedShaderProgram2=0;
  
  this->PropProgram=0;
  this->DefaultMainVS=0;
  this->DefaultMainFS=0;
  this->DefaultPropVS=0;
  this->DefaultPropFS=0;
  this->UseDefaultMainVS=false;
  this->UseDefaultMainFS=false;
  this->UseDefaultPropVS=false;
  this->UseDefaultPropFS=false;
  this->ShaderDeviceAdapter2=0;
  this->CurrentShaderProgram2=0;
}

vtkOpenGLProperty::~vtkOpenGLProperty()
{
  this->SetShader2Collection(0);
  
  if(this->CachedShaderProgram2!=0)
    {
    this->CachedShaderProgram2->Delete();
    }
  if(this->PropProgram!=0)
    {
    this->PropProgram->Delete();
    }
  if(this->DefaultMainVS!=0)
    {
    this->DefaultMainVS->Delete();
    }
  if(this->DefaultMainFS!=0)
    {
    this->DefaultMainFS->Delete();
    }
   if(this->DefaultPropVS!=0)
    {
    this->DefaultPropVS->Delete();
    }
  if(this->DefaultPropFS!=0)
    {
    this->DefaultPropFS->Delete();
    }
  if(this->ShaderDeviceAdapter2!=0)
    {
    this->ShaderDeviceAdapter2->Delete();
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
  
  bool supportShaders=false;
  
  if(prog!=0 || this->Shader2Collection!=0)
    {
    supportShaders=vtkShaderProgram2::IsSupported(
      static_cast<vtkOpenGLRenderWindow *>(oRenderer->GetRenderWindow()));
    if(supportShaders)
      {
      const char*gl_renderer=
        reinterpret_cast<const char *>(glGetString(GL_RENDERER));
      if(strstr(gl_renderer, "Mesa") != 0)
        {
        supportShaders=false;
        vtkErrorMacro(<<"Mesa does not support separate compilation units.");
        }
      }
    else
      {
      vtkErrorMacro(<<"Shaders are not supported by this context.");
      }
    }
  
  if(supportShaders && prog!=0)
    {
    bool progHasVertex=prog->HasVertexShaders();
    bool progHasFragment=prog->HasFragmentShaders();
    bool needDefaultPropFuncVS=progHasVertex;
    bool needDefaultPropFuncFS=progHasFragment;
    bool needDefaultMainVS=false;
    bool needDefaultMainFS=false;
    bool needCacheUpdate=false;
    bool needCacheUniformUpdate=false;
    
    if(this->CachedShaderProgram2==0)
      {
      this->CachedShaderProgram2=vtkShaderProgram2::New();
      this->CachedShaderProgram2->SetContext(static_cast<vtkOpenGLRenderWindow *>(oRenderer->GetRenderWindow()));
      }
    needCacheUpdate=this->LastCachedShaderProgram2!=prog
      || this->CachedShaderProgram2->GetShaders()->GetMTime()<prog->GetShaders()->GetMTime();
    
    needCacheUniformUpdate=this->LastCachedShaderProgram2!=prog || this->CachedShaderProgram2->GetUniformVariables()->GetMTime()<prog->GetUniformVariables()->GetMTime();
    
    if(this->Shader2Collection!=0)
      {
      needDefaultPropFuncVS=needDefaultPropFuncVS &&
        !this->Shader2Collection->HasVertexShaders();
      needDefaultPropFuncFS=needDefaultPropFuncFS &&
        !this->Shader2Collection->HasFragmentShaders();
      
      needCacheUpdate=needCacheUpdate ||
        this->CachedShaderProgram2->GetShaders()->GetMTime()<this->Shader2Collection->GetMTime();
      
      needDefaultMainVS=!progHasVertex && this->Shader2Collection->HasVertexShaders();
      needDefaultMainFS=!progHasFragment && this->Shader2Collection->HasFragmentShaders();
      }
    
    if(needCacheUpdate)
      {
      this->CachedShaderProgram2->GetShaders()->RemoveAllItems();
      this->UseDefaultPropVS=false;
      this->UseDefaultPropFS=false;
      this->UseDefaultMainVS=false;
      this->UseDefaultMainFS=false;
      this->CachedShaderProgram2->GetShaders()->AddCollection(
        prog->GetShaders());
      
      if(this->Shader2Collection!=0)
        {
        this->CachedShaderProgram2->GetShaders()->AddCollection(
          this->Shader2Collection);
        }
      this->LastCachedShaderProgram2=prog;
      }
    
    if(needCacheUniformUpdate)
      {
      vtkUniformVariables *v=prog->GetUniformVariables();
      
      this->CachedShaderProgram2->GetUniformVariables()->DeepCopy(v);
      }
    
    
    if(needDefaultPropFuncVS)
      {
      if(this->DefaultPropVS==0)
        {
        this->UseDefaultPropVS=false;
        this->DefaultPropVS=vtkShader2::New();
        this->DefaultPropVS->SetType(VTK_SHADER_TYPE_VERTEX);
        this->DefaultPropVS->SetSourceCode(vtkOpenGLPropertyDefaultPropFunc_vs);
        // so far, no uniform on the default prop vs.
        assert("check: prog is initialized" && prog->GetContext()!=0);
        this->DefaultPropVS->SetContext(prog->GetContext());
        }
      if(!this->UseDefaultPropVS)
        {
        this->CachedShaderProgram2->GetShaders()->AddItem(this->DefaultPropVS);
        this->UseDefaultPropVS=true;
        }
      }
    if(needDefaultPropFuncFS)
      {
      if(this->DefaultPropFS==0)
        {
        this->UseDefaultPropFS=false;
        this->DefaultPropFS=vtkShader2::New();
        this->DefaultPropFS->SetType(VTK_SHADER_TYPE_FRAGMENT);
        this->DefaultPropFS->SetSourceCode(vtkOpenGLPropertyDefaultPropFunc_fs);
        assert("check: prog is initialized" && prog->GetContext()!=0);
        this->DefaultPropFS->SetContext(prog->GetContext());
        }
      vtkUniformVariables *v=this->DefaultPropFS->GetUniformVariables();
      int value;
      value=0;
      v->SetUniformi("useTexture",1,&value);
      value=0; // allocate texture unit?
      v->SetUniformi("uTexture",1,&value);
      
      if(!UseDefaultPropFS)
        {
        this->CachedShaderProgram2->GetShaders()->AddItem(this->DefaultPropFS);
        this->UseDefaultPropFS=true;
        }
      }
    if(needDefaultMainVS)
      {
      if(this->DefaultMainVS==0)
        {
        this->UseDefaultMainVS=false;
        this->DefaultMainVS=vtkShader2::New();
        this->DefaultMainVS->SetType(VTK_SHADER_TYPE_VERTEX);
        this->DefaultMainVS->SetSourceCode(vtkOpenGLPropertyDefaultMain_vs);
        // so far, no uniform on the default main() vs.
        assert("check: prog is initialized" && prog->GetContext()!=0);
        this->DefaultMainVS->SetContext(prog->GetContext());
        }
      if(!this->UseDefaultMainVS)
        {
        this->CachedShaderProgram2->GetShaders()->AddItem(this->DefaultMainVS);
        this->UseDefaultMainVS=true;
        }
      }
    if(needDefaultMainFS)
      {
      if(this->DefaultMainFS==0)
        {
        this->UseDefaultMainFS=false;
        this->DefaultMainFS=vtkShader2::New();
        this->DefaultMainFS->SetType(VTK_SHADER_TYPE_FRAGMENT);
        this->DefaultMainFS->SetSourceCode(vtkOpenGLPropertyDefaultMain_fs);
        // so far, no uniform on the default main() fs.
        assert("check: prog is initialized" && prog->GetContext()!=0);
        this->DefaultMainFS->SetContext(prog->GetContext());
        }
      if(!this->UseDefaultMainFS)
        {
        this->CachedShaderProgram2->GetShaders()->AddItem(this->DefaultMainFS);
        this->UseDefaultMainFS=true;
        }
      }
    }
  else
    {
    if(supportShaders && this->Shader2Collection!=0)
      {
      bool needDefaultMainVS=this->Shader2Collection->HasVertexShaders();
      bool needDefaultMainFS=this->Shader2Collection->HasFragmentShaders();
      
      if(this->PropProgram==0)
        {
        this->PropProgram=vtkShaderProgram2::New();
        this->PropProgram->SetContext(static_cast<vtkOpenGLRenderWindow *>(oRenderer->GetRenderWindow()));
        }
      this->PropProgram->GetShaders()->AddCollection(this->Shader2Collection);
      
      if(needDefaultMainVS)
        {
        if(this->DefaultMainVS==0)
          {
          this->DefaultMainVS=vtkShader2::New();
          this->DefaultMainVS->SetType(VTK_SHADER_TYPE_VERTEX);
          this->DefaultMainVS->SetSourceCode(vtkOpenGLPropertyDefaultMain_vs);
          // so far, no uniform on the default main() vs.
          }
        this->PropProgram->GetShaders()->AddItem(this->DefaultMainVS);
        this->UseDefaultMainVS=true;
        assert("check: prog is initialized" && this->PropProgram->GetContext()!=0);
        this->DefaultMainVS->SetContext(this->PropProgram->GetContext());
        }
      if(needDefaultMainFS)
        {
        if(this->DefaultMainFS==0)
          {
          this->DefaultMainFS=vtkShader2::New();
          this->DefaultMainFS->SetType(VTK_SHADER_TYPE_FRAGMENT);
          this->DefaultMainFS->SetSourceCode(vtkOpenGLPropertyDefaultMain_fs);
          // so far, no uniform on the default main() fs.
          }
        this->PropProgram->GetShaders()->AddItem(this->DefaultMainFS);
        this->UseDefaultMainFS=true;
        assert("check: prog is initialized" && this->PropProgram->GetContext()!=0);
        this->DefaultMainFS->SetContext(this->PropProgram->GetContext());
        }
      }
    else
      {
      // fixed-pipeline
      }
    }
  
  if((supportShaders && prog!=0) || this->PropProgram!=0)
    {
    if(this->ShaderDeviceAdapter2==0)
      {
      this->ShaderDeviceAdapter2=vtkGLSLShaderDeviceAdapter2::New();
      }
    if(prog!=0)
      {
      this->ShaderDeviceAdapter2->SetShaderProgram(this->CachedShaderProgram2);
      this->CurrentShaderProgram2=this->CachedShaderProgram2;
      }
    else
      {
      this->ShaderDeviceAdapter2->SetShaderProgram(this->PropProgram);
      this->CurrentShaderProgram2=this->PropProgram;
      }
    }
  
  if(this->CurrentShaderProgram2!=0)
    {
    this->CurrentShaderProgram2->Build();
    if(this->CurrentShaderProgram2->GetLastBuildStatus()
       !=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");
      this->CurrentShaderProgram2=0;
      
      if(this->PropProgram!=0)
        {
        this->PropProgram->ReleaseGraphicsResources();
        this->UseDefaultMainVS=false;
        this->UseDefaultMainFS=false;
        this->PropProgram->Delete();
        this->PropProgram=0;
        }
      else
        {
        this->CachedShaderProgram2->ReleaseGraphicsResources();
        if(this->Shader2Collection!=0)
          {
          this->CachedShaderProgram2->GetShaders()->RemoveCollection(this->Shader2Collection);
          }
        if(this->UseDefaultMainVS)
          {
          this->CachedShaderProgram2->GetShaders()->RemoveItem(this->DefaultMainVS);
          this->UseDefaultMainVS=false;
          }
        if(this->UseDefaultMainFS)
          {
          this->CachedShaderProgram2->GetShaders()->RemoveItem(this->DefaultMainFS);
          this->UseDefaultMainFS=false;
          }
        if(this->UseDefaultPropVS)
          {
          this->CachedShaderProgram2->GetShaders()->RemoveItem(this->DefaultPropVS);
          this->UseDefaultPropVS=false;
          }
        if(this->UseDefaultPropFS)
          {
          this->CachedShaderProgram2->GetShaders()->RemoveItem(this->DefaultPropFS);
          this->UseDefaultPropFS=false;
          }
        }
      }
    else
      {
      this->CurrentShaderProgram2->Use();
      if(!this->CurrentShaderProgram2->IsValid())
        {
        vtkErrorMacro(<< "Using the current shader program is invalid with the current OpenGL state. Validation log=" << this->CurrentShaderProgram2->GetLastValidateLog());
        }
      }
    }
  
  glDisable(GL_TEXTURE_2D); // fixed-pipeline

  // disable alpha testing (this may have been enabled
  // by another actor in OpenGLTexture)
  glDisable (GL_ALPHA_TEST);

  glDisable(GL_COLOR_MATERIAL); // fixed-pipeline

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

  if(this->Lighting) // fixed-pipeline
    {
    glEnable(GL_LIGHTING);
    }
  else
    {
    glDisable(GL_LIGHTING);
    }
  
  // render any textures.
  vtkIdType numTextures = this->GetNumberOfTextures();
  if (numTextures > 0)
    {
    if(prog==0) // fixed-pipeline multitexturing
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
        this->GetTextureAtIndex(0)->Render(ren); // one-texture fixed-pipeline
        }
      }
    else
      {
      // texture unit are assigned at each call to render, as render can
      // happen in different/multiple passes.
      
      vtkTextureUnitManager *m=
        static_cast<vtkOpenGLRenderWindow *>(oRenderer->GetRenderWindow())->GetTextureUnitManager();
      
      for (vtkIdType t = 0; t < numTextures; t++)
        {
        vtkTexture *tex=this->GetTextureAtIndex(t);
        int unit=m->Allocate();
        if(unit==-1)
          {
          vtkErrorMacro(<<" not enough texture units.");
          return;
          }
        this->SetTexture(unit,tex);
        vtkgl::ActiveTexture(vtkgl::TEXTURE0 + unit);
        // bind (and load if not yet loaded)
        tex->Render(ren);
        }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
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
  
  if(this->CurrentShaderProgram2!=0) // ie if shaders are supported
    {
    this->CurrentShaderProgram2->Restore();
    this->CurrentShaderProgram2=0;
    }
   
  this->Superclass::PostRender(actor, renderer);

  // render any textures.
  vtkIdType numTextures = this->GetNumberOfTextures();
  if (numTextures > 0 && vtkgl::ActiveTexture)
    {
    if(prog==0) // fixed-pipeline multitexturing
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
    else
      {
      vtkTextureUnitManager *m=
        static_cast<vtkOpenGLRenderWindow *>(renderer->GetRenderWindow())->GetTextureUnitManager();
      
      for (vtkIdType t = 0; t < numTextures; t++)
        {
        int textureUnit=this->GetTextureUnitAtIndex(t);
        m->Free(textureUnit);
        }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
      }
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

// ----------------------------------------------------------------------------
// Description:
// Read this->Material from new style shaders.
void vtkOpenGLProperty::ReadFrameworkMaterial()
{
  vtkShader2Collection *shaders=vtkShader2Collection::New();
  this->SetShader2Collection(shaders);
  shaders->Delete();
  
  
  if (!this->Material)
    {
    vtkErrorMacro("No Material set to read.");
    return;
    }
 
  int cc;
  int max = this->Material->GetNumberOfVertexShaders();
  for (cc=0; cc < max; cc++)
    {
    vtkShader2 *shader=vtkShader2::New();
    vtkXMLShader *XMLshader=this->Material->GetVertexShader(cc);
    
    shader->SetType(VTK_SHADER_TYPE_VERTEX);
    shader->SetSourceCode(XMLshader->GetCode());
    
    // there is no uniform in the example
//    vtkUniformVariables *var=shader->GetUniformVariables();
//    var->
    
    shaders->AddItem(shader);
    shader->Delete();
    }
  vtkDebugMacro(<< max << " Vertex shaders added.");
 
  max = this->Material->GetNumberOfFragmentShaders();
  for (cc=0; cc < max; cc++)
    {
    vtkShader2 *shader=vtkShader2::New();
    vtkXMLShader *XMLshader=this->Material->GetFragmentShader(cc);
    
    shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    shader->SetSourceCode(XMLshader->GetCode());
    
    shaders->AddItem(shader);
    shader->Delete();
    }
  vtkDebugMacro(<< max << " Fragment shaders added.");
  
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
  
  if(this->CachedShaderProgram2!=0)
    {
    this->CachedShaderProgram2->ReleaseGraphicsResources();
    }
  if(this->Shader2Collection!=0)
    {
    this->Shader2Collection->ReleaseGraphicsResources();
    }
  if(this->PropProgram!=0)
    {
    this->PropProgram->ReleaseGraphicsResources();
    }
  if(this->DefaultMainVS!=0)
    {
    this->DefaultMainVS->ReleaseGraphicsResources();
    }
   if(this->DefaultMainFS!=0)
    {
    this->DefaultMainFS->ReleaseGraphicsResources();
    }
   if(this->DefaultPropVS!=0)
     {
     this->DefaultPropVS->ReleaseGraphicsResources();
     }
   if(this->DefaultPropFS!=0)
     {
     this->DefaultPropFS->ReleaseGraphicsResources();
     }
}

//----------------------------------------------------------------------------
void vtkOpenGLProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Shader2Collection: ";
  if(this->Shader2Collection!=0)
    {
    os << endl;
    this->Shader2Collection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  
  if(this->CurrentShaderProgram2!=0)
    {
    os << endl;
    this->CurrentShaderProgram2->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  
  if(this->ShaderDeviceAdapter2!=0)
    {
    os << endl;
    this->ShaderDeviceAdapter2->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
