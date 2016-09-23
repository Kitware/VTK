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

#include "vtkOpenGL.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkOpenGLTexture.h"
#include "vtkTexture.h"

#include "vtkgl.h" // vtkgl namespace

#include "vtkShader2.h"
#include "vtkShaderProgram2.h"
#include "vtkUniformVariables.h"
#include "vtkShader2Collection.h"
#include "vtkTextureUnitManager.h"
#include "vtkGLSLShaderDeviceAdapter2.h"
#include "vtkOpenGLPainterDeviceAdapter.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"

#include <cassert>

namespace
{
  void ComputeMaterialColor(GLfloat result[4],
    bool premultiply_colors_with_alpha,
    double color_factor,
    const double color[3],
    double opacity)
  {
    double opacity_factor = premultiply_colors_with_alpha? opacity : 1.0;
    for (int cc=0; cc < 3; cc++)
    {
      result[cc] = static_cast<GLfloat>(
        opacity_factor * color_factor * color[cc]);
    }
    result[3] = opacity;
  }
}

vtkStandardNewMacro(vtkOpenGLProperty);

extern const char *vtkOpenGLPropertyDefaultMain_vs;
extern const char *vtkOpenGLPropertyDefaultMain_fs;
extern const char *vtkOpenGLPropertyDefaultPropFunc_vs;
extern const char *vtkOpenGLPropertyDefaultPropFunc_fs;

vtkCxxSetObjectMacro(vtkOpenGLProperty,PropProgram,vtkShaderProgram2);

vtkOpenGLProperty::vtkOpenGLProperty()
{
  this->PropProgram = 0;
  this->CachedShaderProgram2 = 0;
  this->LastRendererShaderProgram2 = 0;
  this->LastPropProgram = 0;

  this->DefaultMainVS = 0;
  this->DefaultMainFS = 0;
  this->DefaultPropVS = 0;
  this->DefaultPropFS = 0;
  this->UseDefaultMainVS = false;
  this->UseDefaultMainFS = false;
  this->UseDefaultPropVS = false;
  this->UseDefaultPropFS = false;
  this->ShaderDeviceAdapter2 = 0;
  this->CurrentShaderProgram2 = 0;
}

vtkOpenGLProperty::~vtkOpenGLProperty()
{
  this->SetPropProgram(0);

  if (this->CachedShaderProgram2)
  {
    this->CachedShaderProgram2->Delete();
    this->CachedShaderProgram2 = 0;
  }
  if (this->DefaultMainVS)
  {
    this->DefaultMainVS->Delete();
    this->DefaultMainVS = 0;
  }
  if (this->DefaultMainFS)
  {
    this->DefaultMainFS->Delete();
    this->DefaultMainFS = 0;
  }
   if (this->DefaultPropVS)
   {
    this->DefaultPropVS->Delete();
    this->DefaultPropVS = 0;
   }
  if (this->DefaultPropFS)
  {
    this->DefaultPropFS->Delete();
    this->DefaultPropFS = 0;
  }
  if (this->ShaderDeviceAdapter2)
  {
    this->ShaderDeviceAdapter2->Delete();
    this->ShaderDeviceAdapter2 = 0;
  }
}

vtkShaderDeviceAdapter2* vtkOpenGLProperty::GetShaderDeviceAdapter2()
{
  return this->ShaderDeviceAdapter2;
}

// ----------------------------------------------------------------------------
void vtkOpenGLProperty::AddShaderVariable(const char *name,
                                          int numVars,
                                          int *x)
{
  this->Superclass::AddShaderVariable(name, numVars, x);
  if (!this->PropProgram)
  {
    return;
  }
  vtkShader2Collection* shaders = this->PropProgram->GetShaders();
  shaders->InitTraversal();
  vtkShader2* shader = shaders->GetNextShader();
  while (shader)
  {
    shader->GetUniformVariables()->SetUniformi(name, numVars, x);
    shader = shaders->GetNextShader();
  }
}

// ----------------------------------------------------------------------------
void vtkOpenGLProperty::AddShaderVariable(const char *name,
                                          int numVars,
                                          float *x)
{
  this->Superclass::AddShaderVariable(name, numVars, x);
  if (this->PropProgram)
  {
    if (this->PropProgram->GetUniformVariables()==0)
    {
      vtkUniformVariables *vars=vtkUniformVariables::New();
      this->PropProgram->SetUniformVariables(vars);
      vars->Delete();
    }
    this->PropProgram->GetUniformVariables()->SetUniformf(name, numVars, x);
  }
}

// ----------------------------------------------------------------------------
void vtkOpenGLProperty::AddShaderVariable(const char *name,
                                          int numVars,
                                          double *x)
{
  float *xf = new float[numVars];
  for (int i = 0; i < numVars; i++)
  {
    xf[i] = static_cast<float> (x[i]);
  }
  this->AddShaderVariable(name, numVars, xf);
  delete [] xf;
}

// ----------------------------------------------------------------------------
bool vtkOpenGLProperty::RenderShaders(vtkActor* vtkNotUsed(anActor), vtkRenderer* ren)
{
  // unbind any textures for starters
  vtkOpenGLRenderer *oRenderer = static_cast<vtkOpenGLRenderer *>(ren);
  if (!oRenderer)
  {
    vtkErrorMacro("the vtkOpenGLProperty need a vtkOpenGLRenderer to render.");
    return false;
  }
  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vtkShaderProgram2* prog = oRenderer->GetShaderProgram();
  if (prog)
  {
    assert("check: prog is initialized" && prog->GetContext() == context);
  }

  vtkOpenGLClearErrorMacro();

  vtkShaderProgram2 *propProg;
  if (this->Shading)
  {
    propProg = this->PropProgram;
  }
  else
  {
    propProg = 0;
  }

  bool useShaders = false;
  if (prog || propProg)
  {
    bool shader_support = vtkShaderProgram2::IsSupported(context);

    // mesa doesn't support separate compilation units
    // os mesa:
    // 9.1.4 some tests failing
    vtkOpenGLExtensionManager *extensions = context->GetExtensionManager();

    bool driver_support
      = !extensions->DriverIsMesa()
      || extensions->GetIgnoreDriverBugs(
        "Mesa support for separate compilation units");

    if (shader_support && driver_support)
    {
      useShaders = true;
    }
    else
    {
      vtkErrorMacro(<<"Shaders are not supported by this context.");
    }
  }

  if (useShaders)
  {
    if (!this->CachedShaderProgram2)
    {
      this->CachedShaderProgram2 = vtkShaderProgram2::New();
      this->CachedShaderProgram2->SetContext(context);
    }

    bool needCacheUpdate = (prog != this->LastRendererShaderProgram2)
      || (propProg != this->LastPropProgram);

    if (!needCacheUpdate && prog)
    {
      needCacheUpdate = this->CachedShaderProgram2->GetShaders()->GetMTime()
        < prog->GetShaders()->GetMTime();
    }

    if (!needCacheUpdate && propProg)
    {
      needCacheUpdate = this->CachedShaderProgram2->GetShaders()->GetMTime()
        < propProg->GetShaders()->GetMTime();
    }

    if (needCacheUpdate)
    {
      bool progHasVertex = (prog!=0 ? prog->HasVertexShaders() : false);
      bool progHasFragment = (prog!=0 ? prog->HasFragmentShaders() : false);
      bool needDefaultPropFuncVS = progHasVertex;
      bool needDefaultPropFuncFS = progHasFragment;
      bool needDefaultMainVS = false;
      bool needDefaultMainFS = false;

      if (propProg!=0)
      {
        needDefaultPropFuncVS = needDefaultPropFuncVS
          && !propProg->HasVertexShaders();
        needDefaultPropFuncFS = needDefaultPropFuncFS
          && !propProg->HasFragmentShaders();
        needDefaultMainVS = !progHasVertex
          && propProg->HasVertexShaders();
        needDefaultMainFS = !progHasFragment
          && propProg->HasFragmentShaders();
      }
      this->CachedShaderProgram2->ReleaseGraphicsResources();
      this->CachedShaderProgram2->GetShaders()->RemoveAllItems();
      this->CachedShaderProgram2->GetUniformVariables()->RemoveAllUniforms();
      this->UseDefaultPropVS = false;
      this->UseDefaultPropFS = false;
      this->UseDefaultMainVS = false;
      this->UseDefaultMainFS = false;
      if (prog)
      {
        this->CachedShaderProgram2->GetShaders()->AddCollection(
          prog->GetShaders());
      }

      if (propProg)
      {
        this->CachedShaderProgram2->GetShaders()->AddCollection(
          propProg->GetShaders());
      }
      this->LastRendererShaderProgram2 = prog;
      this->LastPropProgram = propProg;

      if (needDefaultPropFuncVS)
      {
        if (!this->DefaultPropVS)
        {
          this->UseDefaultPropVS = false;
          this->DefaultPropVS = vtkShader2::New();
          this->DefaultPropVS->SetType(VTK_SHADER_TYPE_VERTEX);
          this->DefaultPropVS->SetSourceCode(
            vtkOpenGLPropertyDefaultPropFunc_vs);
          this->DefaultPropVS->SetContext(context);
        }
        if (!this->UseDefaultPropVS)
        {
          this->CachedShaderProgram2->GetShaders()->AddItem(
            this->DefaultPropVS);
          this->UseDefaultPropVS = true;
        }
      }
      if (needDefaultPropFuncFS)
      {
        if (!this->DefaultPropFS)
        {
          this->UseDefaultPropFS = false;
          this->DefaultPropFS = vtkShader2::New();
          this->DefaultPropFS->SetType(VTK_SHADER_TYPE_FRAGMENT);
          this->DefaultPropFS->SetSourceCode(
            vtkOpenGLPropertyDefaultPropFunc_fs);
          this->DefaultPropFS->SetContext(context);
        }
        vtkUniformVariables *v = this->DefaultPropFS->GetUniformVariables();
        int value;
        value = 0;
        v->SetUniformi("useTexture", 1, &value);
        value = 0; // allocate texture unit?
        v->SetUniformi("uTexture", 1, &value);

        if (!UseDefaultPropFS)
        {
          this->CachedShaderProgram2->GetShaders()->AddItem(
            this->DefaultPropFS);
          this->UseDefaultPropFS=true;
        }
      }
      if (needDefaultMainVS)
      {
        if (!this->DefaultMainVS)
        {
          this->UseDefaultMainVS = false;
          this->DefaultMainVS = vtkShader2::New();
          this->DefaultMainVS->SetType(VTK_SHADER_TYPE_VERTEX);
          this->DefaultMainVS->SetSourceCode(vtkOpenGLPropertyDefaultMain_vs);
          this->DefaultMainVS->SetContext(context);
        }
        if (!this->UseDefaultMainVS)
        {
          this->CachedShaderProgram2->GetShaders()->AddItem(
            this->DefaultMainVS);
          this->UseDefaultMainVS = true;
        }
      }
      if (needDefaultMainFS)
      {
        if (!this->DefaultMainFS)
        {
          this->UseDefaultMainFS = false;
          this->DefaultMainFS=vtkShader2::New();
          this->DefaultMainFS->SetType(VTK_SHADER_TYPE_FRAGMENT);
          this->DefaultMainFS->SetSourceCode(vtkOpenGLPropertyDefaultMain_fs);
          this->DefaultMainFS->SetContext(context);
        }
        if (!this->UseDefaultMainFS)
        {
          this->CachedShaderProgram2->GetShaders()->AddItem(
            this->DefaultMainFS);
          this->UseDefaultMainFS = true;
        }
      }

      if (!this->ShaderDeviceAdapter2)
      {
        this->ShaderDeviceAdapter2 = vtkGLSLShaderDeviceAdapter2::New();
      }
      this->ShaderDeviceAdapter2->SetShaderProgram(this->CachedShaderProgram2);
    }

    if (prog &&
       (needCacheUpdate
        || this->CachedShaderProgram2->GetUniformVariables()->GetMTime()
        < prog->GetUniformVariables()->GetMTime()))
    {
      vtkUniformVariables *v = prog->GetUniformVariables();
      this->CachedShaderProgram2->GetUniformVariables()->Merge(v);
    }

    if (propProg &&
       (needCacheUpdate
        || this->CachedShaderProgram2->GetUniformVariables()->GetMTime()
        < propProg->GetUniformVariables()->GetMTime()))
    {
      vtkUniformVariables *v = propProg->GetUniformVariables();
      this->CachedShaderProgram2->GetUniformVariables()->Merge(v);
    }

    this->CurrentShaderProgram2 = this->CachedShaderProgram2;
  }
  else
  {
    this->CurrentShaderProgram2 = 0;
  }

  if (this->CurrentShaderProgram2)
  {
    this->CurrentShaderProgram2->Build();
    if (this->CurrentShaderProgram2->GetLastBuildStatus()
       != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");
      this->CurrentShaderProgram2 = 0;
      this->CachedShaderProgram2->ReleaseGraphicsResources();
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

  // Previous implementation of Render() used to test the prog variable to
  // determine if new style texture was to be used. We are letting that logic
  // be.
  return (prog != NULL);
}

// ----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::Render(vtkActor *anActor, vtkRenderer *ren)
{
  bool rendered_shader_program2 = this->RenderShaders(anActor, ren);

  vtkOpenGLRenderWindow* context = vtkOpenGLRenderWindow::SafeDownCast(
      ren->GetRenderWindow());

  if (!context)
  {
    // must be an OpenGL context.
    return;
  }

  // set interpolation
  switch (this->Interpolation)
  {
  case VTK_FLAT:
    glShadeModel(GL_FLAT);
    break;
  case VTK_GOURAUD:
  case VTK_PHONG:
  default:
    glShadeModel(GL_SMOOTH);
    break;
  }

  if (this->Lighting) // fixed-pipeline
  {
    glEnable(GL_LIGHTING);
  }
  else
  {
    glDisable(GL_LIGHTING);
  }

  // Set the PointSize
  glPointSize(this->PointSize);
  vtkOpenGLGL2PSHelper::SetPointSize(this->PointSize);

  // Set the LineWidth
  glLineWidth(this->LineWidth);
  vtkOpenGLGL2PSHelper::SetLineWidth(this->LineWidth);

  // Set the LineStipple
  if (this->LineStipplePattern != 0xFFFF)
  {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(this->LineStippleRepeatFactor,
                  static_cast<GLushort>(this->LineStipplePattern));
    vtkOpenGLGL2PSHelper::EnableStipple(); // must be called after glLineStipple
  }
  else
  {
    // still need to set this although we are disabling.  else the ATI X1600
    // (for example) still manages to stipple under certain conditions.
    glLineStipple(this->LineStippleRepeatFactor,
                  static_cast<GLushort>(this->LineStipplePattern));
    glDisable(GL_LINE_STIPPLE);
    vtkOpenGLGL2PSHelper::DisableStipple();
  }

  glDisable(GL_TEXTURE_2D); // fixed-pipeline

  // disable alpha testing (this may have been enabled
  // by another actor in OpenGLTexture)
  glDisable (GL_ALPHA_TEST);

  glDisable(GL_COLOR_MATERIAL); // fixed-pipeline

  // turn on/off backface culling
  if (! this->BackfaceCulling && ! this->FrontfaceCulling)
  {
    glDisable (GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  else if (this->BackfaceCulling)
  {
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
  }
  else //if both front & back culling on, will fall into backface culling
  { //if you really want both front and back, use the Actor's visibility flag
    glCullFace (GL_FRONT);
    glEnable (GL_CULL_FACE);
  }

  vtkOpenGLProperty::SetMaterialProperties(
    static_cast<unsigned int>(GL_FRONT_AND_BACK),
    this->Ambient, this->AmbientColor,
    this->Diffuse, this->DiffuseColor,
    this->Specular, this->SpecularColor, this->SpecularPower,
    this->Opacity, context);

  this->RenderTextures(anActor, ren, rendered_shader_program2);
  this->Superclass::Render(anActor, ren);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::SetMaterialProperties(
  unsigned int _face,
  double ambient, const double ambient_color[3],
  double diffuse, const double diffuse_color[3],
  double specular, const double specular_color[3], double specular_power,
  double opacity, vtkOpenGLRenderWindow* context)
{
  GLenum face = static_cast<GLenum>(_face);

  // Dealing with having a correct alpha (none square) in the framebuffer
  // is only required if there is an alpha component in the framebuffer
  // (doh...) and if we cannot deal directly with BlendFuncSeparate.
  GLint alphaBits = context->GetAlphaBitPlanes();
  bool premultiply_colors_with_alpha =
    (vtkgl::BlendFuncSeparate==0 && alphaBits>0);

  GLfloat ambientGL[4];
  ComputeMaterialColor(ambientGL, premultiply_colors_with_alpha,
    ambient, ambient_color, opacity);

  GLfloat diffuseGL[4];
  ComputeMaterialColor(diffuseGL, premultiply_colors_with_alpha,
    diffuse, diffuse_color, opacity);

  GLfloat specularGL[4];
  ComputeMaterialColor(specularGL, premultiply_colors_with_alpha,
    specular, specular_color, opacity);

  glMaterialfv(face, GL_AMBIENT, ambientGL);
  glMaterialfv(face, GL_DIFFUSE, diffuseGL);
  glMaterialfv(face, GL_SPECULAR, specularGL);
  glMaterialf(face, GL_SHININESS, static_cast<GLfloat>(specular_power));

  // The material properties set above are used if shading is
  // enabled. This color set here is used if shading is
  // disabled. Shading is disabled in the
  // vtkOpenGLPolyDataMapper::Draw() method if points or lines
  // are encountered without normals.
  double composite_color[3];
  vtkProperty::ComputeCompositeColor(composite_color,
    ambient, ambient_color, diffuse, diffuse_color, specular, specular_color);

  GLfloat colorGL[4];
  ComputeMaterialColor(colorGL, premultiply_colors_with_alpha,
    1.0, composite_color, opacity);
  glColor4fv(colorGL);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLProperty::RenderTextures(vtkActor*, vtkRenderer* ren,
  bool rendered_shader_program2)
{
  vtkOpenGLRenderWindow* context = vtkOpenGLRenderWindow::SafeDownCast(
      ren->GetRenderWindow());
  assert(context!=NULL);

  // render any textures.
  int numTextures = this->GetNumberOfTextures();
  if (numTextures > 0)
  {
    if (!rendered_shader_program2) // fixed-pipeline multitexturing or old XML shaders.
    {
      this->LoadMultiTexturingExtensions(ren);
      if (vtkgl::ActiveTexture)
      {
        GLint numSupportedTextures;
        glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
        for (int t = 0; t < numTextures; t++)
        {
          int texture_unit = this->GetTextureUnitAtIndex(t);
          if (texture_unit >= numSupportedTextures || texture_unit < 0)
          {
            vtkErrorMacro("Hardware does not support the number of textures defined.");
            continue;
          }

          vtkgl::ActiveTexture(vtkgl::TEXTURE0 +
                               static_cast<GLenum>(texture_unit));
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

      vtkTextureUnitManager *m = context->GetTextureUnitManager();
      for (int t = 0; t < numTextures; t++)
      {
        vtkTexture *tex = this->GetTextureAtIndex(t);
        int unit = m->Allocate();
        if (unit == -1)
        {
          vtkErrorMacro(<<" not enough texture units.");
          return false;
        }
        this->SetTexture(unit,tex);
        vtkgl::ActiveTexture(vtkgl::TEXTURE0 + static_cast<GLenum>(unit));
        // bind (and load if not yet loaded)
        tex->Render(ren);
      }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }
  }

  vtkOpenGLCheckErrorMacro("failed after Render");

  return (numTextures > 0);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::PostRender(vtkActor *actor, vtkRenderer *renderer)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenGLRenderer *oRenderer = static_cast<vtkOpenGLRenderer *>(renderer);
  vtkShaderProgram2 *prog = oRenderer->GetShaderProgram();

  // Reset the face culling now we are done, leaking into text actor etc.
  if (this->BackfaceCulling || this->FrontfaceCulling)
  {
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  if (this->CurrentShaderProgram2) // ie if shaders are supported
  {
    this->CurrentShaderProgram2->Restore();
    this->CurrentShaderProgram2 = 0;
  }

  this->Superclass::PostRender(actor, renderer);

  // render any textures.
  int numTextures = this->GetNumberOfTextures();
  if (numTextures > 0 && vtkgl::ActiveTexture)
  {
    if (!prog) // fixed-pipeline multitexturing or old XML shaders.
    {
      GLint numSupportedTextures;
      glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
      for (int i = 0; i < numTextures; i++)
      {
        int texture_unit = this->GetTextureUnitAtIndex(i);
        if (texture_unit >= numSupportedTextures || texture_unit < 0)
        {
          vtkErrorMacro("Hardware does not support the number of textures defined.");
          continue;
        }
        vtkgl::ActiveTexture(vtkgl::TEXTURE0+
                             static_cast<GLenum>(texture_unit));
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
      vtkTextureUnitManager* m =
        static_cast<vtkOpenGLRenderWindow *>(renderer->GetRenderWindow())->GetTextureUnitManager();

      for (int t = 0; t < numTextures; t++)
      {
        int textureUnit=this->GetTextureUnitAtIndex(t);
        m->Free(textureUnit);
      }
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    }
  }

  vtkOpenGLCheckErrorMacro("failed after PostRender");
}

//-----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::BackfaceRender(vtkActor *vtkNotUsed(anActor), vtkRenderer *ren)
{
  vtkOpenGLRenderWindow* context = vtkOpenGLRenderWindow::SafeDownCast(
    ren->GetRenderWindow());
  vtkOpenGLProperty::SetMaterialProperties(
    static_cast<unsigned int>(GL_BACK),
    this->Ambient, this->AmbientColor,
    this->Diffuse, this->DiffuseColor,
    this->Specular, this->SpecularColor, this->SpecularPower,
    this->Opacity, context);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::LoadMultiTexturingExtensions(vtkRenderer* ren)
{
  if (!vtkgl::MultiTexCoord2d || !vtkgl::ActiveTexture)
  {
    vtkOpenGLExtensionManager* extensions = vtkOpenGLExtensionManager::New();
    extensions->SetRenderWindow(ren->GetRenderWindow());

    // multitexture is a core feature of OpenGL 1.3.
    // multitexture is an ARB extension of OpenGL 1.2.1
    int supports_GL_1_3 = extensions->ExtensionSupported( "GL_VERSION_1_3" );
    int supports_GL_1_2_1 = extensions->ExtensionSupported("GL_VERSION_1_2");
    int supports_ARB_mutlitexture = extensions->ExtensionSupported(
        "GL_ARB_multitexture");

    if (supports_GL_1_3)
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
  int numTextures = this->GetNumberOfTextures();
  if (win && win->GetMapped() && numTextures > 0 && vtkgl::ActiveTexture)
  {
    vtkOpenGLClearErrorMacro();
    GLint numSupportedTextures;
    glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS, &numSupportedTextures);
    for (int i = 0; i < numTextures; i++)
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
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 +
                           static_cast<GLenum>(texture_unit));
      this->GetTextureAtIndex(i)->ReleaseGraphicsResources(win);
    }
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    vtkOpenGLCheckErrorMacro("failwed during ReleaseGraphicsResources");
  }
  else if (numTextures > 0 && vtkgl::ActiveTexture)
  {
    for (int i = 0; i < numTextures; i++)
    {
      this->GetTextureAtIndex(i)->ReleaseGraphicsResources(win);
    }
  }

  this->Superclass::ReleaseGraphicsResources(win);

  if (this->CachedShaderProgram2)
  {
    this->CachedShaderProgram2->ReleaseGraphicsResources();
  }
  if (this->PropProgram)
  {
    this->PropProgram->ReleaseGraphicsResources();
  }
  if (this->DefaultMainVS)
  {
    this->DefaultMainVS->ReleaseGraphicsResources();
  }
   if (this->DefaultMainFS)
   {
    this->DefaultMainFS->ReleaseGraphicsResources();
   }
   if (this->DefaultPropVS)
   {
     this->DefaultPropVS->ReleaseGraphicsResources();
   }
   if (this->DefaultPropFS)
   {
     this->DefaultPropFS->ReleaseGraphicsResources();
   }
}

//----------------------------------------------------------------------------
void vtkOpenGLProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Shader2Collection: ";
  if (this->PropProgram)
  {
    os << endl;
    this->PropProgram->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }

  if (this->CurrentShaderProgram2)
  {
    os << endl;
    this->CurrentShaderProgram2->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }

  if (this->ShaderDeviceAdapter2)
  {
    os << endl;
    this->ShaderDeviceAdapter2->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
