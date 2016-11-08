/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDualDepthPeelingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDualDepthPeelingPass.h"

#include "vtkFrameBufferObject2.h"
#include "vtkInformation.h"
#include "vtkInformationKey.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTypeTraits.h"

#include <algorithm>

// Define to print debug statements to the OpenGL CS stream (useful for e.g.
// apitrace debugging):
//#define ANNOTATE_STREAM

// Define to output details about each peel:
//#define DEBUG_PEEL

// Define to output details about each frame:
//#define DEBUG_FRAME

// Recent OSX/ATI drivers perform some out-of-order execution that's causing
// the dFdx/dFdy calls to be conditionally executed. Specifically, it looks
// like the early returns when the depth is not on a current peel layer
// (Peeling pass, VTK::PreColor::Impl hook) are moved before the dFdx/dFdy
// calls used to compute normals. Disable the early returns on apple for now, I
// don't think most GPUs really benefit from them anyway at this point.
#ifdef __APPLE__
#define NO_PRECOLOR_EARLY_RETURN
#endif


vtkStandardNewMacro(vtkDualDepthPeelingPass)

namespace
{
void annotate(const std::string &str)
{
#ifdef ANNOTATE_STREAM
  vtkOpenGLStaticCheckErrorMacro("Error before glDebug.")
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER,
                       GL_DEBUG_SEVERITY_NOTIFICATION,
                       0, str.size(), str.c_str());
  vtkOpenGLClearErrorMacro();
#else // ANNOTATE_STREAM
  (void)str;
#endif // ANNOTATE_STREAM
}
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Render(const vtkRenderState *s)
{
  // Setup vtkOpenGLRenderPass
  this->PreRender(s);

  this->Initialize(s);
  this->Prepare();

  while (!this->PeelingDone())
  {
    this->Peel();
  }

  this->Finalize();

  this->PostRender(s);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::ReleaseGraphicsResources(vtkWindow *)
{
  this->FreeGLObjects();
}

//------------------------------------------------------------------------------
bool vtkDualDepthPeelingPass::ReplaceShaderValues(std::string &,
                                                  std::string &,
                                                  std::string &fragmentShader,
                                                  vtkAbstractMapper *,
                                                  vtkProp *)
{
  switch (this->CurrentStage)
  {
    case vtkDualDepthPeelingPass::InitializingDepth:
      // Set gl_FragDepth if it isn't set already. It may have already been
      // replaced by the mapper, in which case the substitution will fail and
      // the previously set depth value will be used.
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::Depth::Impl",
            "gl_FragDepth = gl_FragCoord.z;");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Dec",
            "uniform sampler2D opaqueDepth;\n");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::PreColor",
            "ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
            "  float oDepth = texelFetch(opaqueDepth, pixel, 0).y;\n"
            "  if (oDepth != -1. && gl_FragDepth > oDepth)\n"
            "    { // Ignore fragments that are occluded by opaque geometry:\n"
            "    gl_FragData[1].xy = vec2(-1., oDepth);\n"
            "    return;\n"
            "    }\n"
            "  else\n"
            "    {\n"
            "    gl_FragData[1].xy = vec2(-gl_FragDepth, gl_FragDepth);\n"
            "    return;\n"
            "    }\n"
            );
      break;

    case vtkDualDepthPeelingPass::Peeling:
      // Set gl_FragDepth if it isn't set already. It may have already been
      // replaced by the mapper, in which case the substitution will fail and
      // the previously set depth value will be used.
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::Depth::Impl",
            "gl_FragDepth = gl_FragCoord.z;");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Dec",
            "uniform sampler2D lastFrontPeel;\n"
            "uniform sampler2D lastDepthPeel;\n");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::PreColor",
            "  ivec2 pixelCoord = ivec2(gl_FragCoord.xy);\n"
            "  vec4 front = texelFetch(lastFrontPeel, pixelCoord, 0);\n"
            "  vec2 minMaxDepth = texelFetch(lastDepthPeel, pixelCoord, 0).xy;\n"
            "  float minDepth = -minMaxDepth.x;\n"
            "  float maxDepth = minMaxDepth.y;\n"
            "  // Use a tolerance when checking if we're on a current peel.\n"
            "  // Some OSX drivers compute slightly different fragment depths\n"
            "  // from one pass to the next. This value was determined\n"
            "  // through trial-and-error -- it may need to be increased at\n"
            "  // some point. See also the comment in vtkDepthPeelingPass's\n"
            "  // shader.\n"
            "  float epsilon = 0.000001;\n"
            "\n"
            "  // Default outputs (no data/change):\n"
            "  gl_FragData[0] = vec4(0.);\n"
            "  gl_FragData[1] = front;\n"
            "  gl_FragData[2].xy = vec2(-1.);\n"
            "\n"
            "  // Is this fragment outside the current peels?\n"
            "  if (gl_FragDepth < minDepth - epsilon ||\n"
            "      gl_FragDepth > maxDepth + epsilon)\n"
            "    {\n"
#ifndef NO_PRECOLOR_EARLY_RETURN
            "    return;\n"
#else
            "    // Early return removed to avoid instruction-reordering bug\n"
            "    // with dFdx/dFdy on OSX drivers.\n"
            "    // return;\n"
#endif
            "    }\n"
            "\n"
            "  // Is this fragment inside the current peels?\n"
            "  if (gl_FragDepth > minDepth + epsilon &&\n"
            "      gl_FragDepth < maxDepth - epsilon)\n"
            "    {\n"
            "    // Write out depth so this frag will be peeled later:\n"
            "    gl_FragData[2].xy = vec2(-gl_FragDepth, gl_FragDepth);\n"
#ifndef NO_PRECOLOR_EARLY_RETURN
            "    return;\n"
#else
            "    // Early return removed to avoid instruction-reordering bug\n"
            "    // with dFdx/dFdy on OSX drivers.\n"
            "    // return;\n"
#endif
            "    }\n"
            "\n"
            "  // Continue processing for fragments on the current peel:\n"
            );
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Impl",
            "vec4 frag = gl_FragData[0];\n"
            "  // Default outputs (no data/change):\n"
            "\n"
            "  // This fragment is on a current peel:\n"
            "  if (gl_FragDepth >= minDepth - epsilon &&\n"
            "      gl_FragDepth <= minDepth + epsilon)\n"
            "    { // Front peel:\n"
            "    // Clear the back color:\n"
            "    gl_FragData[0] = vec4(0.);\n"
            "\n"
            "    // We store the front alpha value as (1-alpha) to allow MAX\n"
            "    // blending. This also means it is really initialized to 1,\n"
            "    // as it should be for under-blending.\n"
            "    front.a = 1. - front.a;\n"
            "\n"
            "    // Use under-blending to combine fragment with front color:\n"
            "    gl_FragData[1].rgb = front.a * frag.a * frag.rgb + front.rgb;\n"
            "    // Write out (1-alpha):\n"
            "    gl_FragData[1].a = 1. - (front.a * (1. - frag.a));\n"
            "    }\n"
#ifndef NO_PRECOLOR_EARLY_RETURN
            // just 'else' is ok. We'd return earlier in this case.
            "  else // (gl_FragDepth == maxDepth)\n"
#else
            // Need to explicitly test if this is the back peel, since early
            // returns are removed.
            "  else if (gl_FragDepth >= maxDepth - epsilon &&\n"
            "           gl_FragDepth <= maxDepth + epsilon)\n"
#endif
            "    { // Back peel:\n"
            "    // Dump premultiplied fragment, it will be blended later:\n"
            "    frag.rgb *= frag.a;\n"
            "    gl_FragData[0] = frag;\n"
            "    }\n"
#ifdef NO_PRECOLOR_EARLY_RETURN
            // Since the color outputs now get clobbered without the early
            // returns, reset them here.
            "  else\n"
            "    { // Need to clear the colors if not on a current peel.\n"
            "    gl_FragData[0] = vec4(0.);\n"
            "    gl_FragData[1] = front;\n"
            "    }\n"
#endif
            );
      break;

    case vtkDualDepthPeelingPass::AlphaBlending:
      // Set gl_FragDepth if it isn't set already. It may have already been
      // replaced by the mapper, in which case the substitution will fail and
      // the previously set depth value will be used.
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::Depth::Impl",
            "gl_FragDepth = gl_FragCoord.z;");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Dec",
            "uniform sampler2D lastDepthPeel;\n");
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::PreColor",
            "  ivec2 pixelCoord = ivec2(gl_FragCoord.xy);\n"
            "  vec2 minMaxDepth = texelFetch(lastDepthPeel, pixelCoord, 0).xy;\n"
            "  float minDepth = -minMaxDepth.x;\n"
            "  float maxDepth = minMaxDepth.y;\n"
            "\n"
            "  // Discard all fragments outside of the last set of peels:\n"
            "  if (gl_FragDepth < minDepth || gl_FragDepth > maxDepth)\n"
            "    {\n"
            "    discard;\n"
            "    }\n"
            );
      vtkShaderProgram::Substitute(
            fragmentShader, "//VTK::DepthPeeling::Impl",
            "\n"
            "  // Pre-multiply alpha for depth peeling:\n"
            "  gl_FragData[0].rgb *= gl_FragData[0].a;\n"
            );
      break;

    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkDualDepthPeelingPass::SetShaderParameters(vtkShaderProgram *program,
                                                  vtkAbstractMapper *,
                                                  vtkProp *)
{
  switch (this->CurrentStage)
  {
    case vtkDualDepthPeelingPass::InitializingDepth:
      program->SetUniformi(
            "opaqueDepth",
            this->Textures[this->DepthDestination]->GetTextureUnit());
      break;

    case vtkDualDepthPeelingPass::Peeling:
      program->SetUniformi(
            "lastDepthPeel",
            this->Textures[this->DepthSource]->GetTextureUnit());
      program->SetUniformi(
            "frontDepthPeel",
            this->Textures[this->FrontSource]->GetTextureUnit());
      break;

    case vtkDualDepthPeelingPass::AlphaBlending:
      program->SetUniformi(
            "lastDepthPeel",
            this->Textures[this->DepthSource]->GetTextureUnit());
      break;

    default:
      break;
  }

  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDualDepthPeelingPass::GetShaderStageMTime()
{
  return this->CurrentStageTimeStamp.GetMTime();
}

//------------------------------------------------------------------------------
vtkDualDepthPeelingPass::vtkDualDepthPeelingPass()
  : RenderState(NULL),
    CopyDepthProgram(NULL),
    CopyDepthVAO(NULL),
    CopyDepthVBO(NULL),
    BackBlendProgram(NULL),
    BackBlendVAO(NULL),
    BackBlendVBO(NULL),
    BlendProgram(NULL),
    BlendVAO(NULL),
    BlendVBO(NULL),
    Framebuffer(NULL),
    FrontSource(FrontA),
    FrontDestination(FrontB),
    DepthSource(DepthA),
    DepthDestination(DepthB),
    CurrentStage(Inactive),
    CurrentPeel(0),
    OcclusionQueryId(0),
    WrittenPixels(0),
    OcclusionThreshold(0),
    RenderCount(0)
{
  std::fill(this->Textures, this->Textures + static_cast<int>(NumberOfTextures),
            static_cast<vtkTextureObject*>(NULL));
}

//------------------------------------------------------------------------------
vtkDualDepthPeelingPass::~vtkDualDepthPeelingPass()
{
  this->FreeGLObjects();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::SetCurrentStage(ShaderStage stage)
{
  if (stage != this->CurrentStage)
  {
    this->CurrentStage = stage;
    this->CurrentStageTimeStamp.Modified();
  }
}

//------------------------------------------------------------------------------
// Delete the vtkObject subclass pointed at by ptr if it is set.
namespace {
template <typename T> void DeleteHelper(T *& ptr)
{
  if (ptr)
  {
    ptr->Delete();
    ptr = NULL;
  }
}
} // end anon namespace

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::FreeGLObjects()
{
  if (this->Framebuffer)
  {
    this->Framebuffer->Delete();
    this->Framebuffer = NULL;

    for (int i = 0; i < static_cast<int>(NumberOfTextures); ++i)
    {
      this->Textures[i]->Delete();
      this->Textures[i] = NULL;
    }
  }

  DeleteHelper(this->CopyDepthVAO);
  DeleteHelper(this->CopyDepthVBO);
  DeleteHelper(this->BackBlendVAO);
  DeleteHelper(this->BackBlendVBO);
  DeleteHelper(this->BlendVAO);
  DeleteHelper(this->BlendVBO);

  // don't delete the shader programs -- let the cache clean them up.
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::RenderTranslucentPass()
{
  this->TranslucentPass->Render(this->RenderState);
  ++this->RenderCount;
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Initialize(const vtkRenderState *s)
{
  this->RenderState = s;

  // Get current viewport size:
  vtkRenderer *r=s->GetRenderer();
  if(s->GetFrameBuffer()==0)
  {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(&this->ViewportWidth, &this->ViewportHeight,
                             &this->ViewportX, &this->ViewportY);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth = size[0];
    this->ViewportHeight = size[1];
    this->ViewportX =0 ;
    this->ViewportY = 0;
  }

  // See if we can reuse existing textures:
  if (this->Textures[Back] &&
      (static_cast<int>(this->Textures[Back]->GetHeight()) !=
       this->ViewportHeight ||
       static_cast<int>(this->Textures[Back]->GetWidth()) !=
       this->ViewportWidth))
  {
    this->FreeGLObjects();
  }

  // Allocate new textures if needed:
  if (!this->Framebuffer)
  {
    this->Framebuffer = vtkFrameBufferObject2::New();

    std::generate(this->Textures,
                  this->Textures + static_cast<int>(NumberOfTextures),
                  &vtkTextureObject::New);

    this->InitColorTexture(this->Textures[BackTemp], s);
    this->InitColorTexture(this->Textures[Back], s);
    this->InitColorTexture(this->Textures[FrontA], s);
    this->InitColorTexture(this->Textures[FrontB], s);
    this->InitDepthTexture(this->Textures[DepthA], s);
    this->InitDepthTexture(this->Textures[DepthB], s);
    this->InitOpaqueDepthTexture(this->Textures[OpaqueDepth], s);

    this->InitFramebuffer(s);
  }
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitColorTexture(vtkTextureObject *tex,
                                               const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->SetFormat(GL_RGBA);
  tex->SetInternalFormat(GL_RGBA8);
  tex->Allocate2D(this->ViewportWidth, this->ViewportHeight, 4,
                  vtkTypeTraits<vtkTypeUInt8>::VTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitDepthTexture(vtkTextureObject *tex,
                                               const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->SetFormat(GL_RG);
  tex->SetInternalFormat(GL_RG32F);
  tex->Allocate2D(this->ViewportWidth, this->ViewportHeight, 2,
                  vtkTypeTraits<vtkTypeFloat32>::VTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitOpaqueDepthTexture(vtkTextureObject *tex,
                                                     const vtkRenderState *s)
{
  tex->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                    s->GetRenderer()->GetRenderWindow()));
  tex->AllocateDepth(this->ViewportWidth, this->ViewportHeight,
                     vtkTextureObject::Float32);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitFramebuffer(const vtkRenderState *s)
{
  this->Framebuffer->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                                  s->GetRenderer()->GetRenderWindow()));

  // Save the current FBO bindings to restore them later.
  this->Framebuffer->SaveCurrentBindings();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, BackTemp,
                                        this->Textures[BackTemp]);
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, Back,
                                        this->Textures[Back]);

  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, FrontA,
                                        this->Textures[FrontA]);
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, FrontB,
                                        this->Textures[FrontB]);

  // The depth has to be treated like a color attachment, since it's a 2
  // component min-max texture.
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, DepthA,
                                        this->Textures[DepthA]);
  this->Framebuffer->AddColorAttachment(GL_DRAW_FRAMEBUFFER, DepthB,
                                        this->Textures[DepthB]);

  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Prepare()
{
  // Prevent vtkOpenGLActor from messing with the depth mask:
  size_t numProps = this->RenderState->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    vtkProp *prop = this->RenderState->GetPropArray()[i];
    vtkInformation *info = prop->GetPropertyKeys();
    if (!info)
    {
      info = vtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
    }
    info->Set(vtkOpenGLActor::GLDepthMaskOverride(), -1);
  }

  // Setup GL state:
  glDisable(GL_DEPTH_TEST);
  this->InitializeOcclusionQuery();
  this->CurrentPeel = 0;
  this->RenderCount = 0;

  // Save the current FBO bindings to restore them later.
  this->Framebuffer->SaveCurrentBindings();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  // The source front buffer must be initialized, since it simply uses additive
  // blending.
  // The back-blending may discard fragments, so the back peel accumulator needs
  // initialization as well.
  unsigned int targets[2] = { static_cast<unsigned int>(Back),
                              static_cast<unsigned int>(this->FrontSource) };
  this->Framebuffer->ActivateDrawBuffers(targets, 2);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Fill both depth buffers with -1, -1. This lets us discard fragments in
  // CopyOpaqueDepthBuffers, which gives a moderate performance boost.
  targets[0] = static_cast<unsigned int>(this->DepthSource);
  targets[1] = static_cast<unsigned int>(this->DepthDestination);
  this->Framebuffer->ActivateDrawBuffers(targets, 2);
  glClearColor(-1, -1, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  // Pre-fill the depth buffer with opaque pass data:
  this->CopyOpaqueDepthBuffer();

  // Initialize the transparent depths for the peeling algorithm:
  this->InitializeDepth();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeOcclusionQuery()
{
  glGenQueries(1, &this->OcclusionQueryId);

  int numPixels = this->ViewportHeight * this->ViewportWidth;
  this->OcclusionThreshold = numPixels * this->OcclusionRatio;
  this->WrittenPixels = this->OcclusionThreshold + 1;
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::CopyOpaqueDepthBuffer()
{
  // Initialize the peeling depth buffer using the existing opaque depth buffer.
  // Note that the min component is stored as -depth, allowing
  // glBlendEquation = GL_MAX to be used during peeling.

  // Copy from the current (default) framebuffer's depth buffer into a texture:
  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
  this->Textures[OpaqueDepth]->CopyFromFrameBuffer(
        this->ViewportX, this->ViewportY, 0, 0,
        this->ViewportWidth, this->ViewportHeight);
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);

  // Fill both depth buffers with the opaque fragment depths. InitializeDepth
  // will compare translucent fragment depths with values in DepthDestination
  // and write to DepthSource using MAX blending, so we need both to have opaque
  // fragments (src/dst seem reversed because they're named for their usage in
  // PeelRender).
  unsigned int targets[2] = { static_cast<unsigned int>(this->DepthSource),
                              static_cast<unsigned int>(this->DepthDestination)
                            };
  this->Framebuffer->ActivateDrawBuffers(targets, 2);
  this->Textures[OpaqueDepth]->Activate();

  glDisable(GL_BLEND);

  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->CopyDepthProgram)
  {
    std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Decl",
          "uniform float clearValue;\n"
          "uniform sampler2D oDepth;\n");
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Impl",
          "  float d = texture2D(oDepth, texCoord).x;\n"
          "  if (d == clearValue)\n"
          "    { // If no depth value has been written, discard the frag:\n"
          "    discard;\n"
          "    }\n"
          "  gl_FragData[0] = gl_FragData[1] = vec4(-1, d, 0., 0.);\n"
          );
    this->CopyDepthProgram = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->CopyDepthProgram);
  }

  if (!this->CopyDepthVAO)
  {
    this->CopyDepthVBO = vtkOpenGLBufferObject::New();
    this->CopyDepthVAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->CopyDepthVBO, this->CopyDepthVAO,
                              this->CopyDepthProgram);
  }

  // Get the clear value. We don't set this, so it should still be what the
  // opaque pass uses:
  GLfloat clearValue = 1.f;
  glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearValue);
  this->CopyDepthProgram->SetUniformf("clearValue", clearValue);
  this->CopyDepthProgram->SetUniformi(
        "oDepth", this->Textures[OpaqueDepth]->GetTextureUnit());

  this->CopyDepthVAO->Bind();

  annotate("Copying opaque depth!");
  GLUtil::DrawFullScreenQuad();
  annotate("Opaque depth copied!");

  this->CopyDepthVAO->Release();

  this->Textures[OpaqueDepth]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeDepth()
{
  // Add the translucent geometry to our depth peeling buffer:

  // We bind the front destination buffer as render target 0 -- the data we
  // write to it isn't used, but this makes it easier to work with the existing
  // polydata shaders as they expect gl_FragData[0] to be RGBA. The front
  // destination buffer is cleared prior to peeling, so it's just a dummy
  // buffer at this point.
  unsigned int targets[2] = { static_cast<unsigned int>(this->FrontDestination),
                              static_cast<unsigned int>(this->DepthSource)
                            };
  this->Framebuffer->ActivateDrawBuffers(targets, 2);

  this->SetCurrentStage(InitializingDepth);
  this->Textures[this->DepthDestination]->Activate();

  glEnable(GL_BLEND);
  glBlendEquation(GL_MAX);
  annotate("Initializing depth.");
  this->RenderTranslucentPass();
  annotate("Depth initialized");

  this->Textures[this->DepthDestination]->Deactivate();
}

//------------------------------------------------------------------------------
bool vtkDualDepthPeelingPass::PeelingDone()
{
  return this->CurrentPeel >= this->MaximumNumberOfPeels ||
         this->WrittenPixels <= this->OcclusionThreshold;
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Peel()
{
  this->InitializeTargets();
  this->PeelRender();
  this->BlendBackBuffer();
  this->SwapTargets();
  ++this->CurrentPeel;

#ifdef DEBUG_PEEL
  std::cout << "Peel " << this->CurrentPeel << ": Pixels written: "
            << this->WrittenPixels << " (threshold: "
            << this->OcclusionThreshold << ")\n";
#endif // DEBUG_PEEL
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::InitializeTargets()
{
  // Initialize destination buffers to their minima, since we're MAX blending,
  // this ensures that valid outputs are captured.
  unsigned int destColorBuffers[2] =
                        {
                        static_cast<unsigned int>(this->FrontDestination),
                        static_cast<unsigned int>(BackTemp)
                        };
  this->Framebuffer->ActivateDrawBuffers(destColorBuffers, 2);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  this->Framebuffer->ActivateDrawBuffer(this->DepthDestination);
  glClearColor(-1.f, -1.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::PeelRender()
{
  // Enable the destination targets:
  unsigned int targets[3] = { static_cast<unsigned int>(BackTemp),
                              static_cast<unsigned int>(this->FrontDestination),
                              static_cast<unsigned int>(this->DepthDestination)
                            };
  this->Framebuffer->ActivateDrawBuffers(targets, 3);

  // Use MAX blending to capture peels:
  glEnable(GL_BLEND);
  glBlendEquation(GL_MAX);

  this->SetCurrentStage(Peeling);
  this->Textures[this->FrontSource]->Activate();
  this->Textures[this->DepthSource]->Activate();

  annotate("Start peeling!");
  this->RenderTranslucentPass();
  annotate("Peeling done!");

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BlendBackBuffer()
{
  this->Framebuffer->ActivateDrawBuffer(Back);
  this->Textures[BackTemp]->Activate();

  /* For this step, we blend the last peel's back fragments into a back-
   * accumulation buffer. The full over-blending equations are:
   *
   * (f = front frag (incoming peel); b = back frag (current accum. buffer))
   *
   * a = f.a + (1. - f.a) * b.a
   *
   * if a == 0, C == (0, 0, 0). Otherwise,
   *
   * C = ( f.a * f.rgb + (1. - f.a) * b.a * b.rgb ) / a
   *
   * We use premultiplied alphas to save on computations, resulting in:
   *
   * [a * C] = [f.a * f.rgb] + (1 - f.a) * [ b.a * b.rgb ]
   * a = f.a + (1. - f.a) * b.a
   */

  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->BackBlendProgram)
  {
    std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Decl",
          "uniform sampler2D newPeel;\n"
          );
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Impl",
          "  vec4 f = texture2D(newPeel, texCoord); // new frag\n"
          "  if (f.a == 0.)\n"
          "    {\n"
          "    discard;\n"
          "    }\n"
          "\n"
          "  gl_FragData[0] = f;\n"
          );
    this->BackBlendProgram = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BackBlendProgram);
  }

  if (!this->BackBlendVAO)
  {
    this->BackBlendVBO = vtkOpenGLBufferObject::New();
    this->BackBlendVAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->BackBlendVBO, this->BackBlendVAO,
                              this->BackBlendProgram);
  }

  this->BackBlendProgram->SetUniformi(
        "newPeel", this->Textures[BackTemp]->GetTextureUnit());

  this->BackBlendVAO->Bind();

  this->StartOcclusionQuery();
  annotate("Start blending back!");
  GLUtil::DrawFullScreenQuad();
  annotate("Back blended!");
  this->EndOcclusionQuery();

  this->BackBlendVAO->Release();

  this->Textures[BackTemp]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::StartOcclusionQuery()
{
  // ES 3.0 only supports checking if *any* samples passed. We'll just use
  // that query to stop peeling once all frags are processed, and ignore the
  // requested occlusion ratio.
#if GL_ES_VERSION_3_0 == 1
  glBeginQuery(GL_ANY_SAMPLES_PASSED, this->OcclusionQueryId);
#else // GL ES 3.0
  glBeginQuery(GL_SAMPLES_PASSED, this->OcclusionQueryId);
#endif // GL ES 3.0
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::EndOcclusionQuery()
{
#if GL_ES_VERSION_3_0 == 1
  glEndQuery(GL_ANY_SAMPLES_PASSED);
  GLuint anySamplesPassed;
  glGetQueryObjectuiv(this->OcclusionQueryId, GL_QUERY_RESULT,
                      &anySamplesPassed);
  this->WrittenPixels = anySamplesPassed ? this->OcclusionThreshold + 1
                                         : 0;
#else // GL ES 3.0
  glEndQuery(GL_SAMPLES_PASSED);
  glGetQueryObjectuiv(this->OcclusionQueryId, GL_QUERY_RESULT,
                      &this->WrittenPixels);
#endif // GL ES 3.0
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::SwapTargets()
{
  std::swap(this->FrontSource, this->FrontDestination);
  std::swap(this->DepthSource, this->DepthDestination);
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::Finalize()
{
  // Mop up any unrendered fragments using simple alpha blending into the back
  // buffer.
  if (this->WrittenPixels > 0)
  {
    this->AlphaBlendRender();
  }

  this->NumberOfRenderedProps =
      this->TranslucentPass->GetNumberOfRenderedProps();

  this->Framebuffer->UnBind(GL_DRAW_FRAMEBUFFER);
  this->BlendFinalImage();

  // Restore blending parameters:
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  size_t numProps = this->RenderState->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    vtkProp *prop = this->RenderState->GetPropArray()[i];
    vtkInformation *info = prop->GetPropertyKeys();
    if (info)
    {
      info->Remove(vtkOpenGLActor::GLDepthMaskOverride());
    }
  }

  this->RenderState = NULL;
  this->DeleteOcclusionQueryId();
  this->SetCurrentStage(Inactive);

#ifdef DEBUG_FRAME
  std::cout << "Depth peel done:\n"
            << "  - Number of peels: " << this->CurrentPeel << "\n"
            << "  - Number of geometry passes: " << this->RenderCount << "\n"
            << "  - Occlusion Ratio: "
            << static_cast<float>(this->WrittenPixels) /
               static_cast<float>(this->ViewportWidth * this->ViewportHeight)
            << " (target: " << this->OcclusionRatio << ")\n";
#endif // DEBUG_FRAME
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::AlphaBlendRender()
{
  /* This pass is mopping up the remaining fragments when we exceed the max
   * number of peels or hit the occlusion limit. We'll simply render all of the
   * remaining fragments into the back destination buffer using the
   * premultiplied-alpha over-blending equations:
   *
   * aC = f.a * f.rgb + (1 - f.a) * b.a * b.rgb
   * a = f.a + (1 - f.a) * b.a
   */
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  this->SetCurrentStage(AlphaBlending);
  this->Framebuffer->ActivateDrawBuffer(Back);
  this->Textures[this->DepthSource]->Activate();

  annotate("Alpha blend render start");
  this->RenderTranslucentPass();
  annotate("Alpha blend render end");

  this->Textures[this->DepthSource]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::BlendFinalImage()
{
  this->Textures[this->FrontSource]->Activate();
  this->Textures[Back]->Activate();

  /* Peeling is done, time to blend the front and back peel textures with the
   * opaque geometry in the existing framebuffer. First, we'll underblend the
   * back texture beneath the front texture in the shader:
   *
   * Blend 'b' under 'f' to form 't':
   * t.rgb = f.a * b.a * b.rgb + f.rgb
   * t.a   = (1 - b.a) * f.a
   *
   * ( t = translucent layer (back + front), f = front layer, b = back layer )
   *
   * Also in the shader, we adjust the translucent layer's alpha so that it
   * can be used for back-to-front blending, so
   *
   * alphaOverBlend = 1. - alphaUnderBlend
   *
   * To blend the translucent layer over the opaque layer, use regular
   * overblending via glBlendEquation/glBlendFunc:
   *
   * Blend 't' over 'o'
   * C = t.rgb + o.rgb * (1 - t.a)
   * a = t.a + o.a * (1 - t.a)
   *
   * These blending parameters and fragment shader perform this work.
   * Note that the opaque fragments are assumed to have premultiplied alpha
   * in this implementation. */
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->RenderState->GetRenderer()->GetRenderWindow());
  if (!this->BlendProgram)
  {
    std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Decl",
          "uniform sampler2D frontTexture;\n"
          "uniform sampler2D backTexture;\n"
          );
    vtkShaderProgram::Substitute(
          fragShader, "//VTK::FSQ::Impl",
          "  vec4 front = texture2D(frontTexture, texCoord);\n"
          "  vec4 back = texture2D(backTexture, texCoord);\n"
          "  front.a = 1. - front.a; // stored as (1 - alpha)\n"
          "  // Underblend. Back color is premultiplied:\n"
          "  gl_FragData[0].rgb = (front.rgb + back.rgb * front.a);\n"
          "  // The first '1. - ...' is to convert the 'underblend' alpha to\n"
          "  // an 'overblend' alpha, since we'll be letting GL do the\n"
          "  // transparent-over-opaque blending pass.\n"
          "  gl_FragData[0].a = (1. - front.a * (1. - back.a));\n"
          );
    this->BlendProgram = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BlendProgram);
  }

  if (!this->BlendVAO)
  {
    this->BlendVBO = vtkOpenGLBufferObject::New();
    this->BlendVAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->BlendVBO, this->BlendVAO,
                              this->BlendProgram);
  }

  this->BlendProgram->SetUniformi(
        "frontTexture", this->Textures[this->FrontSource]->GetTextureUnit());
  this->BlendProgram->SetUniformi(
        "backTexture", this->Textures[Back]->GetTextureUnit());

  this->BlendVAO->Bind();

  annotate("blending final!");
  GLUtil::DrawFullScreenQuad();
  annotate("final blended!");

  this->BlendVAO->Release();

  this->Textures[this->FrontSource]->Deactivate();
  this->Textures[Back]->Deactivate();
}

//------------------------------------------------------------------------------
void vtkDualDepthPeelingPass::DeleteOcclusionQueryId()
{
  glDeleteQueries(1, &this->OcclusionQueryId);
}
