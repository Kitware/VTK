/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenGLRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderer.h"

#include "vtkCuller.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h" // vtkgl namespace
#include "vtkImageImport.h"
#include "vtkPNGWriter.h"
#include "vtkOpenGLTexture.h"
#include "vtkTimerLog.h"
#include "vtkRenderPass.h"
#include "vtkRenderState.h"

#include "vtkOpenGL.h"

#include "vtkShaderProgram2.h"

#include <math.h>
#include <assert.h>
#include <list>

class vtkGLPickInfo
{
public:
  GLuint* PickBuffer;
  GLuint PickedId;
  GLuint NumPicked;
};

vtkStandardNewMacro(vtkOpenGLRenderer);

vtkCxxSetObjectMacro(vtkOpenGLRenderer,ShaderProgram,vtkShaderProgram2);
vtkCxxSetObjectMacro(vtkOpenGLRenderer, Pass, vtkRenderPass);

#define VTK_MAX_LIGHTS 8

// List of rgba layers, id are 2D rectangle texture Ids.
class vtkOpenGLRendererLayerList
{
public:
  std::list<GLuint> List;
};

extern const char *vtkOpenGLRenderer_PeelingFS;

vtkOpenGLRenderer::vtkOpenGLRenderer()
{
  this->PickInfo = new vtkGLPickInfo;
  this->NumberOfLightsBound = 0;
  this->PickInfo->PickBuffer = 0;
  this->PickInfo->PickedId = 0;
  this->PickInfo->NumPicked = 0;
  this->PickedZ = 0;

  this->DepthPeelingIsSupported=0;
  this->DepthPeelingIsSupportedChecked=0;
  this->LayerList=0;
  this->OpaqueLayerZ=0;
  this->TransparentLayerZ=0;
  this->ProgramShader=0;
  this->DepthFormat=0;
  this->DepthPeelingHigherLayer=0;

  this->ShaderProgram=0;
  this->BackgroundTexture = 0;
  this->Pass = 0;
}

// Internal method temporarily removes lights before reloading them
// into graphics pipeline.
void vtkOpenGLRenderer::ClearLights (void)
{
  short curLight;
  float Info[4];

  // define a lighting model and set up the ambient light.
  // use index 11 for the heck of it. Doesn't matter except for 0.

  // update the ambient light
  Info[0] = this->Ambient[0];
  Info[1] = this->Ambient[1];
  Info[2] = this->Ambient[2];
  Info[3] = 1.0;
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Info);

  if ( this->TwoSidedLighting )
    {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    }
  else
    {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    }

  // now delete all the old lights
  for (curLight = GL_LIGHT0; curLight < GL_LIGHT0 + VTK_MAX_LIGHTS; curLight++)
    {
    glDisable(static_cast<GLenum>(curLight));
    }

  this->NumberOfLightsBound = 0;
}

// Ask lights to load themselves into graphics pipeline.
int vtkOpenGLRenderer::UpdateLights ()
{
  vtkLight *light;
  short curLight;
  float status;
  int count;

  // Check if a light is on. If not then make a new light.
  count = 0;
  curLight= this->NumberOfLightsBound + GL_LIGHT0;

  vtkCollectionSimpleIterator sit;
  for(this->Lights->InitTraversal(sit);
      (light = this->Lights->GetNextLight(sit)); )
    {
    status = light->GetSwitch();
    if ((status > 0.0)&& (curLight < (GL_LIGHT0+VTK_MAX_LIGHTS)))
      {
      curLight++;
      count++;
      }
    }

  if( !count )
    {
    vtkDebugMacro(<<"No lights are on, creating one.");
    this->CreateLight();
    }

  count = 0;
  curLight= this->NumberOfLightsBound + GL_LIGHT0;

  // set the matrix mode for lighting. ident matrix on viewing stack
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  for(this->Lights->InitTraversal(sit);
      (light = this->Lights->GetNextLight(sit)); )
    {

    status = light->GetSwitch();

    // if the light is on then define it and bind it.
    // also make sure we still have room.
    if ((status > 0.0)&& (curLight < (GL_LIGHT0+VTK_MAX_LIGHTS)))
      {
      light->Render(this,curLight);
      glEnable(static_cast<GLenum>(curLight));

      // increment the current light by one
      curLight++;
      count++;
      }
    }

  this->NumberOfLightsBound = curLight - GL_LIGHT0;

  glPopMatrix();
  glEnable(GL_LIGHTING);
  return count;
}

// ----------------------------------------------------------------------------
// Description:
// Access to the OpenGL program shader uniform variable "useTexture" from the
// vtkOpenGLProperty or vtkOpenGLTexture.
int vtkOpenGLRenderer::GetUseTextureUniformVariable()
{
  GLint result=vtkgl::GetUniformLocation(this->ProgramShader,"useTexture");
  if(result==-1)
    {
    vtkErrorMacro(<<"useTexture is not a uniform variable");
    }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Access to the OpenGL program shader uniform variable "texture" from the
// vtkOpenGLProperty or vtkOpenGLTexture.
int vtkOpenGLRenderer::GetTextureUniformVariable()
{
  GLint result=vtkgl::GetUniformLocation(this->ProgramShader,"texture");
  if(result==-1)
    {
    vtkErrorMacro(<<"texture is not a uniform variable");
    }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Is rendering at translucent geometry stage using depth peeling and
// rendering a layer other than the first one? (Boolean value)
// If so, the uniform variables UseTexture and Texture can be set.
// (Used by vtkOpenGLProperty or vtkOpenGLTexture)
int vtkOpenGLRenderer::GetDepthPeelingHigherLayer()
{
  return this->DepthPeelingHigherLayer;
}

// ----------------------------------------------------------------------------
// Concrete open gl render method.
void vtkOpenGLRenderer::DeviceRender(void)
{
  vtkTimerLog::MarkStartEvent("OpenGL Dev Render");

  if(this->Pass!=0)
    {
    vtkRenderState s(this);
    s.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
    s.SetFrameBuffer(0);
    this->Pass->Render(&s);
    }
  else
    {
    // Do not remove this MakeCurrent! Due to Start / End methods on
    // some objects which get executed during a pipeline update,
    // other windows might get rendered since the last time
    // a MakeCurrent was called.
    this->RenderWindow->MakeCurrent();

    // standard render method
    this->ClearLights();

    this->UpdateCamera();
    this->UpdateLightGeometry();
    this->UpdateLights();

    // set matrix mode for actors
    glMatrixMode(GL_MODELVIEW);

    this->UpdateGeometry();

    // clean up the model view matrix set up by the camera
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }

  vtkTimerLog::MarkEndEvent("OpenGL Dev Render");
}

// ----------------------------------------------------------------------------
// Description:
// Render translucent polygonal geometry. Default implementation just call
// UpdateTranslucentPolygonalGeometry().
// Subclasses of vtkRenderer that can deal with depth peeling must
// override this method.
void vtkOpenGLRenderer::DeviceRenderTranslucentPolygonalGeometry()
{
  if(this->UseDepthPeeling)
    {
    if(!this->DepthPeelingIsSupportedChecked)
      {
      this->DepthPeelingIsSupportedChecked=1;
      vtkOpenGLExtensionManager *extensions=vtkOpenGLExtensionManager::New();
      extensions->SetRenderWindow(this->RenderWindow);

      int supports_GL_1_3=extensions->ExtensionSupported("GL_VERSION_1_3");
      int supports_GL_1_4=extensions->ExtensionSupported("GL_VERSION_1_4");
      int supports_GL_1_5=extensions->ExtensionSupported("GL_VERSION_1_5");
      int supports_GL_2_0=extensions->ExtensionSupported("GL_VERSION_2_0");

      int supports_vertex_shader;
      int supports_fragment_shader;
      int supports_shader_objects;
      if(supports_GL_2_0)
        {
        supports_vertex_shader=1;
        supports_fragment_shader=1;
        supports_shader_objects=1;
        }
      else
        {
        supports_vertex_shader=extensions->ExtensionSupported("GL_ARB_vertex_shader");
        supports_fragment_shader=extensions->ExtensionSupported("GL_ARB_fragment_shader");
        supports_shader_objects=extensions->ExtensionSupported("GL_ARB_shader_objects");
        }
      int supports_multitexture=supports_GL_1_3 || extensions->ExtensionSupported("GL_ARB_multitexture");
      int supports_occlusion_query;
      int supports_shadow_funcs;
      if(supports_GL_1_5)
        {
        supports_occlusion_query=1;
        supports_shadow_funcs=1;
        }
      else
        {
        supports_occlusion_query=extensions->ExtensionSupported("GL_ARB_occlusion_query");
        supports_shadow_funcs=extensions->ExtensionSupported("GL_EXT_shadow_funcs");
        }

      int supports_depth_texture;
      int supports_shadow;
      int supports_blend_func_separate;
      if(supports_GL_1_4)
        {
        supports_depth_texture=1;
        supports_blend_func_separate=1;
        supports_shadow=1;
        }
      else
        {
        supports_depth_texture=extensions->ExtensionSupported("GL_ARB_depth_texture");
        supports_shadow=extensions->ExtensionSupported("GL_ARB_shadow");
        supports_blend_func_separate=extensions->ExtensionSupported("GL_EXT_blend_func_separate");
        }

      int supports_GL_ARB_texture_rectangle=extensions->ExtensionSupported("GL_ARB_texture_rectangle");

      // spec claims it is GL_SGIS_texture_edge_clamp, reality shows it is
      // GL_EXT_texture_edge_clamp on Nvidia.
      // part of OpenGL 1.2 core
      // there is no new function with this extension, we don't need to load
      // it.
      int supports_edge_clamp=extensions->ExtensionSupported("GL_VERSION_1_2");
      if(!supports_edge_clamp)
        {
        supports_edge_clamp=extensions->ExtensionSupported("GL_SGIS_texture_edge_clamp");
        if(!supports_edge_clamp)
          {
          // nvidia cards.
          supports_edge_clamp=extensions->ExtensionSupported("GL_EXT_texture_edge_clamp");
          }
        }

      GLint alphaBits;
      glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
      int supportsAtLeast8AlphaBits=alphaBits>=8;

      this->DepthPeelingIsSupported =
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

      if(this->DepthPeelingIsSupported)
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
      extensions->Delete();

      if(this->DepthPeelingIsSupported)
        {
        // Some OpenGL implementations such as Mesa or ATI
        // claim to support both GLSL and GL_ARB_texture_rectangle but
        // don't actually support sampler2DRectShadow in a GLSL code.
        // To test that, we compile the shader, if it fails, we don't use
        // deph peeling
        GLuint shader =
          vtkgl::CreateShader(vtkgl::FRAGMENT_SHADER);
        vtkgl::ShaderSource(
          shader, 1,
          const_cast<const char **>(&vtkOpenGLRenderer_PeelingFS), 0);
        vtkgl::CompileShader(shader);
        GLint params;
        vtkgl::GetShaderiv(shader,vtkgl::COMPILE_STATUS,
                           &params);
        this->DepthPeelingIsSupported = params==GL_TRUE;
        vtkgl::DeleteShader(shader);
        if(!this->DepthPeelingIsSupported)
          {
          vtkDebugMacro("this OpenGL implementation does not support "
                        "GL_ARB_texture_rectangle in GLSL code");
          }
        }
      if(this->DepthPeelingIsSupported)
        {
        // Some OpenGL implementations are buggy so depth peeling does not work:
        //  - ATI
        //  - Mesa 6.5.2 and lower
        // Do alpha blending always.
        const char* gl_renderer =
          reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        int isATI = strstr(gl_renderer, "ATI") != 0;

        const char* gl_version =
          reinterpret_cast<const char *>(glGetString(GL_VERSION));
        if(const char* mesa_version = strstr(gl_version, "Mesa"))
          {
          // Mesa versions 6.5.3 and higher work.  Versions much lower
          // than 6.5.2 do not report support for the extensions to
          // get this far.  Therefore if parsing of the version fails
          // just assume it is a higher version that changed the
          // format of the version string.
          int mesa_major = 0;
          int mesa_minor = 0;
          int mesa_patch = 0;
          if(sscanf(mesa_version, "Mesa %d.%d.%d",
                    &mesa_major, &mesa_minor, &mesa_patch) >= 2)
            {
            if(mesa_major  < 6 ||
               (mesa_major == 6 && mesa_major  < 5) ||
               (mesa_major == 6 && mesa_minor == 5 && mesa_patch < 3))
              {
              this->DepthPeelingIsSupported = 0;
              }
            }
          }
        else if(isATI)
          {
          this->DepthPeelingIsSupported = 0;
          }
        }
      }
    }

  if(!this->UseDepthPeeling || !this->DepthPeelingIsSupported)
    {
    // just alpha blending
    this->LastRenderingUsedDepthPeeling=0;
    this->UpdateTranslucentPolygonalGeometry();
    }
  else
    {
    // depth peeling.

    // get the viewport dimensions
    this->GetTiledSizeAndOrigin(&this->ViewportWidth,&this->ViewportHeight,
                                &this->ViewportX,&this->ViewportY);

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
    vtkgl::ActiveTexture(vtkgl::TEXTURE1 );
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
      this->LastRenderingUsedDepthPeeling=0;
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
      this->UpdateTranslucentPolygonalGeometry();
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
      this->LastRenderingUsedDepthPeeling=0;
      vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
      this->UpdateTranslucentPolygonalGeometry();
      return;
      }

    // Have to be set before a call to UpdateTranslucentPolygonalGeometry()
    // because UpdateTranslucentPolygonalGeometry() will eventually call
    // vtkOpenGLActor::Render() that uses this flag.
    this->LastRenderingUsedDepthPeeling=1;

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
    this->LayerList=new vtkOpenGLRendererLayerList;

    // save the default blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    int multiSampleStatus=glIsEnabled(vtkgl::MULTISAMPLE);

    if(multiSampleStatus)
      {
      glDisable(vtkgl::MULTISAMPLE);
      }
    glDisable(GL_BLEND);
    GLuint nbPixels=0;
    GLuint previousNbPixels=0;
    int l=0;
    while(!stop)
      {
      vtkgl::BeginQuery(vtkgl::SAMPLES_PASSED,queryId);
      stop=!this->RenderPeel(l);
      vtkgl::EndQuery(vtkgl::SAMPLES_PASSED);
      // blocking call
      previousNbPixels=nbPixels;
      if(!stop || l>0) // stop && l==0 <=> no translucent geometry
        {
        vtkgl::GetQueryObjectuiv(queryId,vtkgl::QUERY_RESULT,&nbPixels);
        if(!stop)
          {
          stop=(nbPixels<=threshold) || (nbPixels==previousNbPixels);
          ++l;
          if(!stop && !infiniteLoop)
            {
            stop=l>=this->MaximumNumberOfPeels;
            }
          }
        }
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
    // Destroy the shader
    if(this->ProgramShader!=0)
      {
      vtkgl::DeleteProgram(this->ProgramShader);
      this->ProgramShader=0;
      }

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
    }
}

// ----------------------------------------------------------------------------
// Description:
// Check the compilation status of some fragment shader source.
void vtkOpenGLRenderer::CheckCompilation(
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
// \pre positive_layer: layer>=0
int vtkOpenGLRenderer::RenderPeel(int layer)
{
  assert("pre: positive_layer" && layer>=0);

  GLbitfield mask=GL_COLOR_BUFFER_BIT;
  if(layer>0)
    {
    mask=mask|GL_DEPTH_BUFFER_BIT;
    }

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(mask);

  vtkgl::ActiveTexture(vtkgl::TEXTURE2);
  glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->OpaqueLayerZ);
  vtkgl::ActiveTexture(vtkgl::TEXTURE1 );

  if(this->ProgramShader==0)
    {
    this->ProgramShader=vtkgl::CreateProgram();
    GLuint shader=vtkgl::CreateShader(vtkgl::FRAGMENT_SHADER);
    vtkgl::ShaderSource(shader,1,const_cast<const char **>(&vtkOpenGLRenderer_PeelingFS),0);
    vtkgl::CompileShader(shader);
    this->CheckCompilation(shader);
    vtkgl::AttachShader(this->ProgramShader,shader);
    vtkgl::LinkProgram(this->ProgramShader);

    GLint params;
    vtkgl::GetProgramiv(static_cast<GLuint>(this->ProgramShader),vtkgl::LINK_STATUS,&params);
    if(params==GL_TRUE)
      {
      vtkDebugMacro(<<"program linked successfully");
      }
    else
      {
      vtkErrorMacro(<<"program link error");
      // include null terminator
      vtkgl::GetProgramiv(static_cast<GLuint>(this->ProgramShader),vtkgl::INFO_LOG_LENGTH,&params);
      if(params>0)
        {
#if 1
        char *buffer=new char[params];
        vtkgl::GetProgramInfoLog(static_cast<GLuint>(this->ProgramShader),params,0,buffer);
        vtkErrorMacro(<<"log: "<<buffer);
        delete[] buffer;
#endif
        }
      else
        {
        vtkErrorMacro(<<"no log: ");
        }
      }
    vtkgl::DeleteShader(shader); // reference counting
    }

  if(layer>0)
    {
    glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB,this->TransparentLayerZ);
    vtkgl::UseProgram(this->ProgramShader);
    GLint uShadowTex=vtkgl::GetUniformLocation(this->ProgramShader,"shadowTex");
    if(uShadowTex!=-1)
      {
      vtkgl::Uniform1i(uShadowTex,1);
      }
    else
      {
      vtkErrorMacro(<<"error: shadowTex is not a uniform.");
      }
    GLint uOpaqueShadowTex=vtkgl::GetUniformLocation(this->ProgramShader,"opaqueShadowTex");
    if(uOpaqueShadowTex!=-1)
      {
      vtkgl::Uniform1i(uOpaqueShadowTex,2);
      }
    else
      {
      vtkErrorMacro(<<"error: opaqueShadowTex is not a uniform.");
      }

    GLint uOffsetX=vtkgl::GetUniformLocation(this->ProgramShader,"offsetX");
    if(uOffsetX!=-1)
      {
      vtkgl::Uniform1f(uOffsetX,this->ViewportX);
      }
    else
      {
      vtkErrorMacro(<<"error: offsetX is not a uniform.");
      }

    GLint uOffsetY=vtkgl::GetUniformLocation(this->ProgramShader,"offsetY");
    if(uOffsetY!=-1)
      {
      vtkgl::Uniform1f(uOffsetY,this->ViewportY);
      }
    else
      {
      vtkErrorMacro(<<"error: offsetY is not a uniform.");
      }
    }
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 );
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  this->DepthPeelingHigherLayer=layer>0;
  int numberOfRenderedProps=this->UpdateTranslucentPolygonalGeometry();
  if(layer>0)
    {
    this->DepthPeelingHigherLayer=0;
    vtkgl::UseProgram(0);
    }

  GLint width;
  vtkgl::ActiveTexture(vtkgl::TEXTURE1 );
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

// ----------------------------------------------------------------------------
void vtkOpenGLRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " <<
    this->NumberOfLightsBound << "\n";
  os << indent << "PickBuffer " << this->PickInfo->PickBuffer << "\n";
  os << indent << "PickedId" << this->PickInfo->PickedId<< "\n";
  os << indent << "NumPicked" << this->PickInfo->NumPicked<< "\n";
  os << indent << "PickedZ " << this->PickedZ << "\n";
  os << indent << "Pass:";
  if(this->Pass!=0)
    {
      os << "exists" << endl;
    }
  else
    {
      os << "null" << endl;
    }
}


void vtkOpenGLRenderer::Clear(void)
{
  GLbitfield  clear_mask = 0;

  if (! this->Transparent())
    {
    glClearColor( static_cast<GLclampf>(this->Background[0]),
                  static_cast<GLclampf>(this->Background[1]),
                  static_cast<GLclampf>(this->Background[2]),
                  static_cast<GLclampf>(0.0));
    clear_mask |= GL_COLOR_BUFFER_BIT;
    }

  if (!this->GetPreserveDepthBuffer())
    {
    glClearDepth(static_cast<GLclampf>(1.0));
    clear_mask |= GL_DEPTH_BUFFER_BIT;
    }

  vtkDebugMacro(<< "glClear\n");
  glClear(clear_mask);

  // If gradient background is turned on, draw it now.
  if (!this->Transparent() &&
      (this->GradientBackground || this->TexturedBackground))
    {
    double tile_viewport[4];
    this->GetRenderWindow()->GetTileViewport(tile_viewport);
    glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glShadeModel(GL_SMOOTH); // color interpolation

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    {
      glLoadIdentity();
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      {
        glLoadIdentity();
        glOrtho(
          tile_viewport[0],
          tile_viewport[2],
          tile_viewport[1],
          tile_viewport[3],
          -1.0, 1.0);

        //top vertices
        if(this->TexturedBackground && this->BackgroundTexture)
          {
          glEnable(GL_TEXTURE_2D);

          this->BackgroundTexture->Render(this);

          // NOTE: By default the mode is GL_MODULATE. Since the user
          // cannot set the mode, the default is set to replace.
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

          // NOTE: vtkTexture Render enables the alpha test
          // so that no buffer is affected if alpha of incoming fragment is
          // below the threshold. Here we have to enable it so that it won't
          // rejects the fragments of the quad as the alpha is set to 0 on it.
          glDisable(GL_ALPHA_TEST);
          }

        glBegin(GL_QUADS);
        glColor4d(this->Background[0],this->Background[1],this->Background[2],
                  0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex2f(0.0, 0.0);

        glTexCoord2f(1.0, 0.0);
        glVertex2f(1.0, 0);

        //bottom vertices
        glColor4d(this->Background2[0],this->Background2[1],
                  this->Background2[2],0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex2f(1.0, 1.0);

        glTexCoord2f(0.0, 1.0);
        glVertex2f(0.0, 1.0);

        glEnd();
      }
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
    }
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
    }
}

void vtkOpenGLRenderer::StartPick(unsigned int pickFromSize)
{

  int bufferSize = pickFromSize * 4;

  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update,
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();
  this->RenderWindow->IsPickingOn();
  if (this->PickInfo->PickBuffer)
    {
    delete [] this->PickInfo->PickBuffer;
    this->PickInfo->PickBuffer = 0;
    }
  this->PickInfo->PickBuffer = new GLuint[bufferSize];
  glSelectBuffer(bufferSize, this->PickInfo->PickBuffer);
  // change to selection mode
  (void)glRenderMode(GL_SELECT);
  // initialize the pick names and add a 0 name, for no pick
  glInitNames();
  glPushName(0);
}

void vtkOpenGLRenderer::ReleaseGraphicsResources(vtkWindow *w)
{
  if (w && this->Pass)
    {
    this->Pass->ReleaseGraphicsResources(w);
    }
}

void vtkOpenGLRenderer::UpdatePickId()
{
  glLoadName(this->CurrentPickId++);
}


void vtkOpenGLRenderer::DevicePickRender()
{
  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update,
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();

  // standard render method
  this->ClearLights();

  this->UpdateCamera();
  this->UpdateLightGeometry();
  this->UpdateLights();

  // set matrix mode for actors
  glMatrixMode(GL_MODELVIEW);

  this->PickGeometry();

  // clean up the model view matrix set up by the camera
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}


void vtkOpenGLRenderer::DonePick()
{
  glFlush();
  GLuint hits = glRenderMode(GL_RENDER);
  this->PickInfo->NumPicked = hits;

  unsigned int depth = static_cast<unsigned int>(-1);
  GLuint* ptr = this->PickInfo->PickBuffer;
  this->PickInfo->PickedId = 0;
  for(unsigned int k =0; k < hits; k++)
    {
    int num_names = *ptr;
    int save = 0;
    ptr++; // move to first depth value
    if(*ptr <= depth)
      {
      depth = *ptr;
      save = 1;
      }
    ptr++; // move to next depth value
    if(*ptr <= depth)
      {
      depth = *ptr;
      save = 1;
      }
    // move to first name picked
    ptr++;
    if(save)
      {
      this->PickInfo->PickedId = *ptr;
      }
    // skip additional names
    ptr += num_names;
    }
  // If there was a pick, then get the Z value
  if(this->PickInfo->PickedId)
    {
    // convert from pick depth described as:
    // Returned depth values are mapped such that the largest unsigned
    // integer value corresponds to window coordinate depth 1.0,
    // and zero corresponds to window coordinate depth 0.0.

    this->PickedZ = depth/static_cast<double>(VTK_UNSIGNED_INT_MAX);

    // Clamp to range [0,1]
    this->PickedZ = (this->PickedZ < 0.0) ? 0.0 : this->PickedZ;
    this->PickedZ = (this->PickedZ > 1.0) ? 1.0: this->PickedZ;
    }

  //Don't delete the list, keep it around in case caller wants all
  //of the hits. Delete it elsewhere when needed.
  //delete [] this->PickInfo->PickBuffer;
  //this->PickInfo->PickBuffer = 0;

  this->RenderWindow->IsPickingOff();
}

double vtkOpenGLRenderer::GetPickedZ()
{
  return this->PickedZ;
}

unsigned int vtkOpenGLRenderer::GetPickedId()
{
  return static_cast<unsigned int>(this->PickInfo->PickedId);
}

vtkOpenGLRenderer::~vtkOpenGLRenderer()
{
  if (this->PickInfo->PickBuffer)
    {
    delete [] this->PickInfo->PickBuffer;
    this->PickInfo->PickBuffer = 0;
    }
  delete this->PickInfo;

  if(this->ShaderProgram!=0)
    {
    this->ShaderProgram->Delete();
    }

  if(this->Pass!=0)
    {
    this->Pass->UnRegister(this);
    }
}

unsigned int vtkOpenGLRenderer::GetNumPickedIds()
{
  return static_cast<unsigned int>(this->PickInfo->NumPicked);
}

int vtkOpenGLRenderer::GetPickedIds(unsigned int atMost,
                                    unsigned int *callerBuffer)
{
  if (!this->PickInfo->PickBuffer)
    {
    return 0;
    }

  unsigned int max = (atMost < this->PickInfo->NumPicked) ? atMost : this->PickInfo->NumPicked;
  GLuint* iptr = this->PickInfo->PickBuffer;
  unsigned int *optr = callerBuffer;
  unsigned int k;
  for(k =0; k < max; k++)
    {
    int num_names = *iptr;
    iptr++; // move to first depth value
    iptr++; // move to next depth value
    iptr++; // move to first name picked
    *optr = static_cast<unsigned int>(*iptr);
    optr++;
    // skip additional names
    iptr += num_names;
    }
  return k;
}
