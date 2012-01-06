/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDepthPeelingPass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDepthPeelingPass.h"
#include "vtkObjectFactory.h"
#include <assert.h>
#include "vtkRenderState.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"
#include <list>
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkTextureUnitManager.h"

vtkStandardNewMacro(vtkDepthPeelingPass);
vtkCxxSetObjectMacro(vtkDepthPeelingPass,TranslucentPass,vtkRenderPass);

// List of rgba layers, id are 2D rectangle texture Ids.
class vtkDepthPeelingPassLayerList
{
public:
  std::list<GLuint> List;
};

extern const char *vtkDepthPeeling_fs;

// ----------------------------------------------------------------------------
vtkDepthPeelingPass::vtkDepthPeelingPass()
{
  this->TranslucentPass=0;
  this->IsSupported=false;
  this->IsChecked=false;

  this->OcclusionRatio=0.0;
  this->MaximumNumberOfPeels=4;
  this->LastRenderingUsedDepthPeeling=false;
  this->DepthPeelingHigherLayer=0;
  this->Prog=vtkShaderProgram2::New();
  this->Shader=vtkShader2::New();
  this->Prog->GetShaders()->AddItem(this->Shader);
  this->Shader->SetSourceCode(vtkDepthPeeling_fs);
  this->Shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
  
  vtkUniformVariables *v=this->Shader->GetUniformVariables();
  
  this->ShadowTexUnit=-1; // not allocated
  this->OpaqueShadowTexUnit=-1; // not allocated
  
  
  int value;
  value=1;
  v->SetUniformi("shadowTex",1,&value);
  value=2;
  v->SetUniformi("opaqueShadowTex",1,&value);
}

// ----------------------------------------------------------------------------
vtkDepthPeelingPass::~vtkDepthPeelingPass()
{
  if(this->TranslucentPass!=0)
    {
      this->TranslucentPass->Delete();
    }
  this->Shader->Delete();
  this->Prog->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Destructor. Delete SourceCode if any.
void vtkDepthPeelingPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  this->Shader->ReleaseGraphicsResources();
  this->Prog->ReleaseGraphicsResources();
  if(this->TranslucentPass)
    {
    this->TranslucentPass->ReleaseGraphicsResources(w);
    }
}

// ----------------------------------------------------------------------------
void vtkDepthPeelingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "OcclusionRation: " << this->OcclusionRatio << endl;
  
  os << indent << "MaximumNumberOfPeels: " << this->MaximumNumberOfPeels
     << endl;
  
  os << indent << "LastRenderingUsedDepthPeeling: ";
  if(this->LastRenderingUsedDepthPeeling)
    {
    os << "On" << endl;
    }
  else
    {
    os << "Off" << endl;
    }
  
  os << indent << "TranslucentPass:";
  if(this->TranslucentPass!=0)
    {
    this->TranslucentPass->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkDepthPeelingPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;
  
  if(this->TranslucentPass==0)
    {
      vtkWarningMacro(<<"No TranslucentPass delegate set. Nothing can be rendered.");
      return;
    }

  // Any prop to render?
  bool hasTranslucentPolygonalGeometry=false;
  int i=0;
  while(!hasTranslucentPolygonalGeometry && i<s->GetPropArrayCount())
    {
      hasTranslucentPolygonalGeometry=
        s->GetPropArray()[i]->HasTranslucentPolygonalGeometry()==1;
      ++i;
    }
  if(!hasTranslucentPolygonalGeometry)
    {
      return; // nothing to render.
    }

  this->CheckSupport(static_cast<vtkOpenGLRenderWindow *>(
                       s->GetRenderer()->GetRenderWindow()));

  if(!this->IsSupported)
    {
      // just alpha blending
      this->LastRenderingUsedDepthPeeling=false;
      this->TranslucentPass->Render(s);
      this->NumberOfRenderedProps=this->TranslucentPass->GetNumberOfRenderedProps();
      return;
    }

  // Depth peeling.
  vtkRenderer *r=s->GetRenderer();
  
  if(s->GetFrameBuffer()==0)
    {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(&this->ViewportWidth,&this->ViewportHeight,
                             &this->ViewportX,&this->ViewportY);
    }
  else
    {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth=size[0];
    this->ViewportHeight=size[1];
    this->ViewportX=0;
    this->ViewportY=0;
    }
    
  // get z bits
  GLint depthBits;
  glGetIntegerv(GL_DEPTH_BITS,&depthBits);
  if(depthBits==16)
    {
      this->DepthFormat=vtkgl::DEPTH_COMPONENT16_ARB;
    }
  else
    {
      this->DepthFormat=vtkgl::DEPTH_COMPONENT24_ARB;
    }
    // 1. Grab the RGBAZ of the opaque layer.
    GLuint opaqueLayerZ=0;
    GLuint opaqueLayerRgba=0;
    glGenTextures(1,&opaqueLayerZ);
    this->OpaqueLayerZ=opaqueLayerZ;
    
    glGenTextures(1,&opaqueLayerRgba);
    // opaque z format
//    vtkgl::ActiveTexture(vtkgl::TEXTURE1 );
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,opaqueLayerZ);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,
                    vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,
                    vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,
                    vtkgl::TEXTURE_COMPARE_MODE,
                    vtkgl::COMPARE_R_TO_TEXTURE);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,
                    vtkgl::TEXTURE_COMPARE_FUNC,
                    GL_LESS);
    
    // Allocate memory
    glTexImage2D(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB,0,this->DepthFormat,
                 this->ViewportWidth,this->ViewportHeight,
                 0,GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    GLint width;
    glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB,0,
                             GL_TEXTURE_WIDTH,&width);
    if(width==0)
      {
      vtkDebugMacro(<<"not enough GPU RAM for opaque z");
      // not enough GPU RAM. Do alpha blending technique instead
      glDeleteTextures(1,&opaqueLayerRgba);
      glDeleteTextures(1,&opaqueLayerZ);
      this->LastRenderingUsedDepthPeeling=false;
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
      this->TranslucentPass->Render(s);
      this->NumberOfRenderedProps=this->TranslucentPass->GetNumberOfRenderedProps();
      return;
      }
    glTexImage2D(vtkgl::TEXTURE_RECTANGLE_ARB,0,this->DepthFormat,
                 this->ViewportWidth,this->ViewportHeight, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
    // Grab the z-buffer
    glCopyTexSubImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, this->ViewportX,
                        this->ViewportY,this->ViewportWidth,
                        this->ViewportHeight);
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,opaqueLayerRgba);
    // opaque rgba format
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    // Allocate memory
    glTexImage2D(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
                 this->ViewportWidth,this->ViewportHeight,
                 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB,0,
                             GL_TEXTURE_WIDTH,&width);
    if(width==0)
      {
      vtkDebugMacro(<<"not enough GPU RAM for opaque rgba");
      // not enough GPU RAM. Do alpha blending technique instead
      glDeleteTextures(1,&opaqueLayerRgba);
      glDeleteTextures(1,&opaqueLayerZ);
      this->LastRenderingUsedDepthPeeling=false;
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
      this->TranslucentPass->Render(s);
      this->NumberOfRenderedProps=this->TranslucentPass->GetNumberOfRenderedProps();
      return;
      }
    
    // Have to be set before a call to UpdateTranslucentPolygonalGeometry()
    // because UpdateTranslucentPolygonalGeometry() will eventually call
    // vtkOpenGLActor::Render() that uses this flag.
    this->LastRenderingUsedDepthPeeling=true;
    this->SetLastRenderingUsedDepthPeeling(s->GetRenderer(),true);
    
    glTexImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
                 this->ViewportWidth,this->ViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, 0);
    // Grab the rgba-buffer
    glCopyTexSubImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, this->ViewportX,
                        this->ViewportY,this->ViewportWidth,
                        this->ViewportHeight);
    
    GLuint queryId;
    vtkgl::GenQueries(1,&queryId);
    int stop=0;
    int infiniteLoop=this->MaximumNumberOfPeels==0;
    
    unsigned int threshold=static_cast<unsigned int>(this->ViewportWidth*this->ViewportHeight*OcclusionRatio);
    this->LayerList=new vtkDepthPeelingPassLayerList;
    
    // save the default blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    
    int multiSampleStatus=glIsEnabled(vtkgl::MULTISAMPLE);
    
    if(multiSampleStatus)
      {
      glDisable(vtkgl::MULTISAMPLE);
      }
    glDisable(GL_BLEND);
    
    vtkUniformVariables *v=this->Shader->GetUniformVariables();
    float value;
    value=this->ViewportX;
    v->SetUniformf("offsetX",1,&value);
    value=this->ViewportY;
    v->SetUniformf("offsetY",1,&value);
    
    
    this->Prog->SetContext(static_cast<vtkOpenGLRenderWindow *>(
                             s->GetRenderer()->GetRenderWindow()));
    this->Shader->SetContext(this->Prog->GetContext());
      
    GLuint nbPixels=0;
    GLuint previousNbPixels=0;
    int l=0;
    while(!stop)
      {
      vtkgl::BeginQuery(vtkgl::SAMPLES_PASSED,queryId);
      stop=!this->RenderPeel(s,l);
      vtkgl::EndQuery(vtkgl::SAMPLES_PASSED);
      // blocking call 
      previousNbPixels=nbPixels;
      if(!stop || l>0) // stop && l==0 <=> no translucent geometry
        {
        vtkgl::GetQueryObjectuiv(queryId,vtkgl::QUERY_RESULT,&nbPixels);
        ++l;
        if(!stop)
          {
          stop=(nbPixels<=threshold) || (nbPixels==previousNbPixels);
          if(!stop && !infiniteLoop)
            {
            stop=l>=this->MaximumNumberOfPeels;
            }
          }
        }
      }
    
    if(l>1) // some higher layer, we allocated some tex unit in RenderPeel()
      {
      vtkTextureUnitManager *m=
        this->Prog->GetContext()->GetTextureUnitManager();
      m->Free(this->ShadowTexUnit);
      m->Free(this->OpaqueShadowTexUnit);
      this->ShadowTexUnit=-1;
      this->OpaqueShadowTexUnit=-1;
      }
    
    if(multiSampleStatus)
      {
      glEnable(vtkgl::MULTISAMPLE);
      }
    // The two following lines are taken from vtkOpenGLProperty to
    // reset texturing state after rendering the props.
    glDisable(GL_TEXTURE_2D);
    glDisable (GL_ALPHA_TEST);
    glDepthFunc(GL_LEQUAL);
    vtkgl::DeleteQueries(1,&queryId);
    if(this->TransparentLayerZ!=0)
      {
      GLuint transparentLayerZ=static_cast<GLuint>(this->TransparentLayerZ);
      glDeleteTextures(1,&transparentLayerZ);
      this->TransparentLayerZ=0;
      }
    
    // Finally, draw sorted opacity
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, this->ViewportWidth, 0, this->ViewportHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glClearColor( static_cast<GLclampf>(0),static_cast<GLclampf>(0),
                  static_cast<GLclampf>(0),static_cast<GLclampf>(0));
    
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(vtkgl::TEXTURE_RECTANGLE_ARB);
    
    // actor in wireframe may have change that
    glPolygonMode(GL_FRONT, GL_FILL);
    
    glDisable(GL_BLEND);
    // First the opaque layer
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,opaqueLayerRgba);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(this->ViewportWidth, 0);
    glVertex2f(this->ViewportWidth, 0);
    glTexCoord2f(this->ViewportWidth, this->ViewportHeight);
    glVertex2f(this->ViewportWidth, this->ViewportHeight);
    glTexCoord2f(0, this->ViewportHeight);
    glVertex2f(0, this->ViewportHeight);
    glEnd();
    
    vtkgl::BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                             GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    // the transparent layers
    std::list<GLuint>::reverse_iterator it=this->LayerList->List.rbegin();
    std::list<GLuint>::reverse_iterator itEnd=this->LayerList->List.rend();
    while(it!=itEnd)
      {
      glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,(*it));
      
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);
      glVertex2f(0, 0);
      glTexCoord2f(this->ViewportWidth, 0);
      glVertex2f(this->ViewportWidth, 0);
      glTexCoord2f(this->ViewportWidth, this->ViewportHeight);
      glVertex2f(this->ViewportWidth, this->ViewportHeight);
      glTexCoord2f(0, this->ViewportHeight);
      glVertex2f(0, this->ViewportHeight);
      glEnd();
      ++it;
      }
    // Restore the default blend function for the next stage (overlay)
    glPopAttrib();
  
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(vtkgl::TEXTURE_RECTANGLE_ARB);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Destroy the layers
    size_t c=this->LayerList->List.size();
    GLuint *ids=new GLuint[c];
    std::list<GLuint>::const_iterator it2=this->LayerList->List.begin();
    size_t layer=0;
    while(layer<c)
      {
      ids[layer]=(*it2);
      ++layer;
      ++it2;
      }
    glDeleteTextures(static_cast<GLsizei>(c),ids);
    delete[] ids;
    delete this->LayerList;
    this->LayerList=0;
    
    glDeleteTextures(1,&opaqueLayerRgba);
    glDeleteTextures(1,&opaqueLayerZ);
    this->NumberOfRenderedProps=this->TranslucentPass->GetNumberOfRenderedProps();
}

// ----------------------------------------------------------------------------
void vtkDepthPeelingPass::CheckSupport(vtkOpenGLRenderWindow *w)
{
  assert("pre: w_exists" && w!=0);

  if(!this->IsChecked || w->GetContextCreationTime()>this->CheckTime)
    {
      this->IsChecked=true;
      this->CheckTime.Modified();
      vtkOpenGLExtensionManager *extensions=w->GetExtensionManager();
      
      bool supports_GL_1_3=extensions->ExtensionSupported("GL_VERSION_1_3")==1;
      bool supports_GL_1_4=extensions->ExtensionSupported("GL_VERSION_1_4")==1;
      bool supports_GL_1_5=extensions->ExtensionSupported("GL_VERSION_1_5")==1;
      bool supports_GL_2_0=extensions->ExtensionSupported("GL_VERSION_2_0")==1;
      
      bool supports_vertex_shader;
      bool supports_fragment_shader;
      bool supports_shader_objects;
      if(supports_GL_2_0)
        {
        supports_vertex_shader=true;
        supports_fragment_shader=true;
        supports_shader_objects=true;
        }
      else
        {
        supports_vertex_shader=extensions->ExtensionSupported("GL_ARB_vertex_shader")==1;
        supports_fragment_shader=extensions->ExtensionSupported("GL_ARB_fragment_shader")==1;
        supports_shader_objects=extensions->ExtensionSupported("GL_ARB_shader_objects")==1;
        }
      bool supports_multitexture=supports_GL_1_3 || extensions->ExtensionSupported("GL_ARB_multitexture");
      bool supports_occlusion_query;
      bool supports_shadow_funcs;
      if(supports_GL_1_5)
        {
        supports_occlusion_query=true;
        supports_shadow_funcs=true;
        }
      else
        {
        supports_occlusion_query=extensions->ExtensionSupported("GL_ARB_occlusion_query")==1;
        supports_shadow_funcs=extensions->ExtensionSupported("GL_EXT_shadow_funcs")==1;
        }
      
      bool supports_depth_texture;
      bool supports_shadow;
      bool supports_blend_func_separate;
      if(supports_GL_1_4)
        {
        supports_depth_texture=true;
        supports_blend_func_separate=true;
        supports_shadow=true;
        }
      else
        {
        supports_depth_texture=extensions->ExtensionSupported("GL_ARB_depth_texture")==1;
        supports_shadow=extensions->ExtensionSupported("GL_ARB_shadow")==1;
        supports_blend_func_separate=extensions->ExtensionSupported("GL_EXT_blend_func_separate")==1;
        }
      
      bool supports_GL_ARB_texture_rectangle=extensions->ExtensionSupported("GL_ARB_texture_rectangle")==1;
      
      // spec claims it is GL_SGIS_texture_edge_clamp, reality shows it is
      // GL_EXT_texture_edge_clamp on Nvidia.
      // part of OpenGL 1.2 core
      // there is no new function with this extension, we don't need to load
      // it.
      bool supports_edge_clamp=extensions->ExtensionSupported("GL_VERSION_1_2")==1;
      if(!supports_edge_clamp)
        {
        supports_edge_clamp=extensions->ExtensionSupported("GL_SGIS_texture_edge_clamp")==1;
        if(!supports_edge_clamp)
          {
          // nvidia cards.
          supports_edge_clamp=extensions->ExtensionSupported("GL_EXT_texture_edge_clamp")==1;
          }
        }
      
      GLint alphaBits;
      glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
      bool supportsAtLeast8AlphaBits=alphaBits>=8;
      
      this->IsSupported =
        supports_depth_texture &&
        supports_shadow &&
        supports_blend_func_separate &&
        supports_shadow_funcs &&
        supports_vertex_shader &&
        supports_fragment_shader &&
        supports_shader_objects &&
        supports_occlusion_query &&
        supports_multitexture &&
        supports_GL_ARB_texture_rectangle &&
        supports_edge_clamp &&
        supportsAtLeast8AlphaBits;
      
      if(this->IsSupported)
        {
        vtkDebugMacro("depth peeling supported");
        if(supports_GL_1_3)
          {
          extensions->LoadExtension("GL_VERSION_1_3");
          }
        else
          {
          extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
          }
        // GL_ARB_depth_texture, GL_ARB_shadow and GL_EXT_shadow_funcs
        // don't introduce new functions.
        if(supports_GL_1_4)
          {
          extensions->LoadExtension("GL_VERSION_1_4");
          }
        else
          {
          extensions->LoadCorePromotedExtension("GL_EXT_blend_func_separate");
          }
        
        if(supports_GL_2_0)
          {
          extensions->LoadExtension("GL_VERSION_2_0");
          }
        else
          {
          extensions->LoadCorePromotedExtension("GL_ARB_vertex_shader");
          extensions->LoadCorePromotedExtension("GL_ARB_fragment_shader");
          extensions->LoadCorePromotedExtension("GL_ARB_shader_objects");
          }
        if(supports_GL_1_5)
          {
          extensions->LoadExtension("GL_VERSION_1_5");
          }
        else
          {
          extensions->LoadCorePromotedExtension("GL_ARB_occlusion_query");
          }
        
        extensions->LoadExtension("GL_ARB_texture_rectangle");
        }
      else
        {
        vtkDebugMacro(<<"depth peeling is not supported.");
        if(!supports_depth_texture)
          {
          vtkDebugMacro(<<"neither OpenGL 1.4 nor GL_ARB_depth_texture is supported");
          }
        if(!supports_shadow)
          {
          vtkDebugMacro(<<"neither OpenGL 1.4 nor GL_ARB_shadow is supported");
          }
        if(!supports_shadow_funcs)
          {
          vtkDebugMacro(<<"neither OpenGL 1.5 nor GL_EXT_shadow_funcs is supported");
          }
        if(!supports_vertex_shader)
          {
          vtkDebugMacro(<<"neither OpenGL 2.0 nor GL_ARB_vertex_shader is supported");
          }
        if(!supports_fragment_shader)
          {
          vtkDebugMacro(<<"neither OpenGL 2.0 nor GL_ARB_fragment_shader is supported");
          }
        if(!supports_shader_objects)
          {
          vtkDebugMacro(<<"neither OpenGL 2.0 nor GL_ARB_shader_objects is supported");
          }
        if(!supports_occlusion_query)
          {
          vtkDebugMacro(<<"neither OpenGL 1.5 nor GL_ARB_occlusion_query is supported");
          }
        if(!supports_multitexture)
          {
          vtkDebugMacro(<<"neither OpenGL 1.3 nor GL_ARB_multitexture is supported");
          }
        if(!supports_GL_ARB_texture_rectangle)
          {
          vtkDebugMacro(<<"GL_ARB_texture_rectangle is not supported");
          }
        if(!supports_edge_clamp)
          {
          vtkDebugMacro(<<"neither OpenGL 1.2 nor GL_SGIS_texture_edge_clamp nor GL_EXT_texture_edge_clamp is not supported");
          }
        if(!supportsAtLeast8AlphaBits)
          {
          vtkDebugMacro(<<"at least 8 alpha bits is not supported");
          }
        }

      if(this->IsSupported)
        {
        // Some OpenGL implementations are buggy so depth peeling does not
        // work:
        //  - ATI
        //  - Mesa git does not support true linking of shaders (VTK bug 8135)
        //    and Mesa 7.2 just crashes during the try-compile.
        // Do alpha blending always.
        const char* gl_renderer =
          reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        int isATI = strstr(gl_renderer, "ATI") != 0;
        
        bool isMesa=strstr(gl_renderer, "Mesa") != 0;
        
        if(isMesa || isATI)
          {
          this->IsSupported = false;
          }
        }
      
      if(this->IsSupported)
        {
        // Some OpenGL implementations such as ATI
        // claim to support both GLSL and GL_ARB_texture_rectangle but
        // don't actually support sampler2DRectShadow in a GLSL code.
        // Others (like Mesa) claim to support shaders but don't actually
        // support true linking of shaders (and declaration of functions).
        // To test that, we compile the shader, if it fails, we don't use
        // deph peeling
        GLuint shader =
          vtkgl::CreateShader(vtkgl::FRAGMENT_SHADER);
        vtkgl::ShaderSource(
          shader, 1,
          const_cast<const char **>(&vtkDepthPeeling_fs), 0);
        vtkgl::CompileShader(shader);
        GLint params;
        vtkgl::GetShaderiv(shader,vtkgl::COMPILE_STATUS,
                           &params);
        this->IsSupported = params==GL_TRUE;
        vtkgl::DeleteShader(shader);
        if(!this->IsSupported)
          {
          vtkDebugMacro("this OpenGL implementation does not support "
                        "GL_ARB_texture_rectangle in GLSL code or does"
                        "not support true linking of shaders.");
          }
        }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Check the compilation status of some fragment shader source.
void vtkDepthPeelingPass::CheckCompilation(
  unsigned int fragmentShader)
{
  GLuint fs=static_cast<GLuint>(fragmentShader);
  GLint params;
  vtkgl::GetShaderiv(fs,vtkgl::COMPILE_STATUS,&params);
  if(params==GL_TRUE)
    {
    vtkDebugMacro(<<"shader source compiled successfully");
    }
  else
    {
    vtkErrorMacro(<<"shader source compile error");
    // include null terminator
    vtkgl::GetShaderiv(fs,vtkgl::INFO_LOG_LENGTH,&params);
    if(params>0)
      {
      char *buffer=new char[params];
      vtkgl::GetShaderInfoLog(fs,params,0,buffer);
      vtkErrorMacro(<<"log: "<<buffer);
      delete[] buffer;
      }
    else
      {
      vtkErrorMacro(<<"no log");
      }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Render a peel layer. If there is no more GPU RAM to save the texture,
// return false otherwise returns true. Also if layer==0 and no prop have
// been rendered (there is no translucent geometry), it returns false.
// \pre s_exists: s!=0
// \pre positive_layer: layer>=0
int vtkDepthPeelingPass::RenderPeel(const vtkRenderState *s,
                                    int layer)
{
  assert("pre: s_exists" && s!=0);
  assert("pre: positive_layer" && layer>=0);
  
  GLbitfield mask=GL_COLOR_BUFFER_BIT;
  if(layer>0)
    {
    mask=mask|GL_DEPTH_BUFFER_BIT;
    }
  
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(mask);
  
//  vtkgl::ActiveTexture(vtkgl::TEXTURE0+this->OpaqueShadowTexUnit);
//  glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->OpaqueLayerZ);
//  vtkgl::ActiveTexture(vtkgl::TEXTURE0+this->ShadowTexUnit);
  
  vtkOpenGLRenderer *oRenderer=
    static_cast<vtkOpenGLRenderer *>(s->GetRenderer());
  
  if(layer>0)
    {
    if(layer==1)
      {
    // allocate texture units.
      vtkTextureUnitManager *m=
        this->Prog->GetContext()->GetTextureUnitManager();
      this->ShadowTexUnit=m->Allocate();
      if(this->ShadowTexUnit==-1)
        {
        vtkErrorMacro(<<"Ought. No texture unit left!");
        return 0;
        }
      this->OpaqueShadowTexUnit=m->Allocate();
      if(this->OpaqueShadowTexUnit==-1)
        {
        vtkErrorMacro(<<"Ought. No texture unit left!");
        return 0;
        }
      vtkUniformVariables *v=this->Shader->GetUniformVariables();
      int ivalue;
      ivalue=this->ShadowTexUnit; // 1
      v->SetUniformi("shadowTex",1,&ivalue);
      ivalue=this->OpaqueShadowTexUnit; //2
      v->SetUniformi("opaqueShadowTex",1,&ivalue);
      }
    
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+this->OpaqueShadowTexUnit);
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->OpaqueLayerZ);
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+this->ShadowTexUnit);
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->TransparentLayerZ);
    oRenderer->SetShaderProgram(this->Prog);
//    this->Prog->Use();
    }
  
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  this->DepthPeelingHigherLayer=layer>0;
  this->TranslucentPass->Render(s);
  int numberOfRenderedProps=this->TranslucentPass->GetNumberOfRenderedProps();
  if(layer>0)
    {
    this->DepthPeelingHigherLayer=0;
    oRenderer->SetShaderProgram(0);
    }
  
  GLint width;
//  vtkgl::ActiveTexture(vtkgl::TEXTURE0+this->ShadowTexUnit);
  if(layer==0)
    {
    if(numberOfRenderedProps>0)
      {
      GLuint transparentLayerZ;
      glGenTextures(1,&transparentLayerZ);
      this->TransparentLayerZ=static_cast<unsigned int>(transparentLayerZ);
      glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->TransparentLayerZ);
      
      glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,
                      GL_NEAREST);
      glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,
                      GL_NEAREST);
      glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,
                      vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,
                      vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,
                      vtkgl::TEXTURE_COMPARE_MODE,
                      vtkgl::COMPARE_R_TO_TEXTURE);
      glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,
                      vtkgl::TEXTURE_COMPARE_FUNC,
                      GL_GREATER);
      
      // Allocate memory
      glTexImage2D(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB,0,this->DepthFormat,
                   this->ViewportWidth,this->ViewportHeight,
                   0,GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
      glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB,0,
                               GL_TEXTURE_WIDTH,&width);
      if(width==0)
        {
        // not enough GPU RAM. Use alpha blending technique instead
        glDeleteTextures(1,&transparentLayerZ);
        this->TransparentLayerZ=0;
        return 0;
        }
      glTexImage2D(vtkgl::TEXTURE_RECTANGLE_ARB,0,this->DepthFormat,
                   this->ViewportWidth,this->ViewportHeight, 0,
                   GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
      }
    }
  else
    {
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->TransparentLayerZ);
    }
  if((layer==0 && numberOfRenderedProps>0) || layer>0)
    {
    // Grab the z-buffer
    glCopyTexSubImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, this->ViewportX,
                        this->ViewportY,this->ViewportWidth,
                        this->ViewportHeight);
    
    // Grab the rgba buffer
    GLuint rgba;
    glGenTextures(1,&rgba);
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,rgba);
    // rgba format
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    
    // Allocate memory
    glTexImage2D(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
                 this->ViewportWidth,this->ViewportHeight,
                 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGetTexLevelParameteriv(vtkgl::PROXY_TEXTURE_RECTANGLE_ARB,0,
                             GL_TEXTURE_WIDTH,&width);
    if(width==0)
      {
      // not enough GPU RAM. Do alpha blending technique instead
      glDeleteTextures(1,&rgba);
      return 0;
      }
    
    glTexImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8,
                 this->ViewportWidth,this->ViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, 0);
    
    // Grab the rgba-buffer
    glCopyTexSubImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, this->ViewportX,
                        this->ViewportY,this->ViewportWidth,
                        this->ViewportHeight);
    this->LayerList->List.push_back(rgba);
    return 1;
    }
  else
    {
    return 0;
    }
}
