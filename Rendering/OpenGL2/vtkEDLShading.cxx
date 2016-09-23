/*=========================================================================

   Program: ParaView
   Module:    vtkEDLShading.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/

#include "vtkEDLShading.h"

#include "vtkCamera.h"
#include "vtkFrameBufferObject.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkPropCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtk_glew.h"
#include <cassert>
#include <sstream>
#include <string>


#include "vtkTextureObjectVS.h"
#include "vtkEDLShadeFS.h"
#include "vtkEDLComposeFS.h"
#include "vtkEDLBilateralFilterFS.h"

//#define VTK_EDL_SHADING_DEBUG

vtkStandardNewMacro(vtkEDLShading);

// ----------------------------------------------------------------------------
vtkEDLShading::vtkEDLShading()
{

  this->ProjectionFBO = 0;
  this->ProjectionColorTexture = 0;
  this->ProjectionDepthTexture = 0;

  this->EDLHighFBO = 0;
  this->EDLHighShadeTexture = 0;
  this->EDLLowFBO = 0;
  this->EDLLowShadeTexture = 0;
  this->EDLLowBlurTexture = 0;

  this->EDLIsFiltered = true;
  // init neighbours in image space
  for (int c = 0; c < 8; c++)
  {
    float x, y;
    x = cos(2* 3.14159 * float (c)/8.);
    y = sin(2*3.14159*float(c)/8.);
    this->EDLNeighbours[c][0] = x / sqrt(x*x+y*y);
    this->EDLNeighbours[c][1] = y / sqrt(x*x+y*y);
    this->EDLNeighbours[c][2] = 0.;
    this->EDLNeighbours[c][3] = 0.;
  }
  this->EDLLowResFactor = 2;
  this->Zn = 0.1;
  this->Zf = 1.0;
}

// ----------------------------------------------------------------------------
vtkEDLShading::~vtkEDLShading()
{
  if (this->ProjectionFBO != 0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->ProjectionColorTexture != 0)
  {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->ProjectionDepthTexture != 0)
  {
    vtkErrorMacro(<<"DepthTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->EDLHighFBO != 0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->EDLHighShadeTexture != 0)
  {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->EDLLowFBO != 0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->EDLLowShadeTexture != 0)
  {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
  if (this->EDLLowBlurTexture != 0)
  {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void vtkEDLShading::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DelegatePass:";
  if (this->DelegatePass != 0)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Initialize framebuffers and associated texture objects,
// with link to render state s
void vtkEDLShading::EDLInitializeFramebuffers(vtkRenderState &s)
{
  vtkRenderer *r = s.GetRenderer();

  vtkOpenGLCheckErrorMacro("failed before Initialize");

  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());

  //  PROJECTION FBO and TEXTURES
  //
  if (this->ProjectionFBO == 0)
  {
    this->ProjectionFBO = vtkFrameBufferObject::New();
    this->ProjectionFBO->SetContext(renWin);
  }
  s.SetFrameBuffer(this->ProjectionFBO);
  this->ProjectionFBO->Bind();
  // Color texture
  if (this->ProjectionColorTexture == 0)
  {
    this->ProjectionColorTexture = vtkTextureObject::New();
    this->ProjectionColorTexture->SetContext(this->ProjectionFBO->GetContext());
  }
  if (this->ProjectionColorTexture->GetWidth() != static_cast<unsigned int>
    (this->W)
      || this->ProjectionColorTexture->GetHeight()
          != static_cast<unsigned int> (this->H))
  {
    this->ProjectionColorTexture->Create2D(this->W, this->H, 4, VTK_FLOAT, false);
  }
  // Depth texture
  if (this->ProjectionDepthTexture == 0)
  {
    this->ProjectionDepthTexture = vtkTextureObject::New();
    this->ProjectionDepthTexture->SetContext(this->ProjectionFBO->GetContext());
  }
  if (this->ProjectionDepthTexture->GetWidth() != static_cast<unsigned int> (this->W)
      || this->ProjectionDepthTexture->GetHeight()
          != static_cast<unsigned int> (this->H))
  {
    this->ProjectionDepthTexture->AllocateDepth(
      this->W, this->H, vtkTextureObject::Float32);
  }

  // Apply textures
  // to make things clear, we write all
  this->ProjectionFBO->SetNumberOfRenderTargets(1);
  this->ProjectionFBO->SetColorBuffer(0, this->ProjectionColorTexture);
  this->ProjectionFBO->SetActiveBuffer(0);
  this->ProjectionFBO->SetDepthBuffer(this->ProjectionDepthTexture);

  this->ProjectionDepthTexture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->ProjectionDepthTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  this->ProjectionDepthTexture->SetMinificationFilter(vtkTextureObject::Linear);
  this->ProjectionDepthTexture->SetLinearMagnification(true);
  this->ProjectionDepthTexture->Bind();
  this->ProjectionDepthTexture->SendParameters();

  this->ProjectionFBO->UnBind();

  //  EDL-RES1 FBO and TEXTURE
  //
  if (this->EDLHighFBO == 0)
  {
    this->EDLHighFBO = vtkFrameBufferObject::New();
    this->EDLHighFBO->SetContext(renWin);
  }
  s.SetFrameBuffer(EDLHighFBO);
  // Color texture
  if (this->EDLHighShadeTexture == 0)
  {
    this->EDLHighShadeTexture = vtkTextureObject::New();
    this->EDLHighShadeTexture->SetContext(this->EDLHighFBO->GetContext());
  }
  if (this->EDLHighShadeTexture->GetWidth() != static_cast<unsigned int> (this->W)
      || this->EDLHighShadeTexture->GetHeight()
          != static_cast<unsigned int> (this->H))
  {
    this->EDLHighShadeTexture->Create2D(this->W, this->H, 4, VTK_FLOAT, false);
  }
  this->EDLHighFBO->SetNumberOfRenderTargets(1);
  this->EDLHighFBO->SetColorBuffer(0, this->EDLHighShadeTexture);
  this->EDLHighFBO->SetActiveBuffer(0);
  this->EDLHighFBO->SetDepthBufferNeeded(false);
  this->EDLHighFBO->UnBind();

  //  EDL-RES2 FBO and TEXTURE
  //
  if (this->EDLLowFBO == 0)
  {
    this->EDLLowFBO = vtkFrameBufferObject::New();
    this->EDLLowFBO->SetContext(renWin);
  }
  s.SetFrameBuffer(EDLLowFBO);
  // Color texture
  if (this->EDLLowShadeTexture == 0)
  {
    this->EDLLowShadeTexture = vtkTextureObject::New();
    this->EDLLowShadeTexture->SetContext(this->EDLLowFBO->GetContext());
  }
  if (this->EDLLowShadeTexture->GetWidth() != static_cast<unsigned int> (this->W
      / EDLLowResFactor) || this->EDLLowShadeTexture->GetHeight()
      != static_cast<unsigned int> (this->H / EDLLowResFactor))
  {
    this->EDLLowShadeTexture->Create2D(this->W / EDLLowResFactor,
        this->H / EDLLowResFactor, 4, VTK_FLOAT, false);
  }
  // Blur texture
  if (this->EDLLowBlurTexture == 0)
  {
    this->EDLLowBlurTexture = vtkTextureObject::New();
    this->EDLLowBlurTexture->SetContext(this->EDLLowFBO->GetContext());
  }
  if (this->EDLLowBlurTexture->GetWidth() != static_cast<unsigned int> (this->W
      / EDLLowResFactor) || this->EDLLowBlurTexture->GetHeight()
      != static_cast<unsigned int> (this->H / EDLLowResFactor))
  {
    this->EDLLowBlurTexture->Create2D(this->W / EDLLowResFactor, this->H / EDLLowResFactor,
        4, VTK_FLOAT, false);
  }
  this->EDLLowFBO->SetNumberOfRenderTargets(1);
  this->EDLLowFBO->SetColorBuffer(0, this->EDLLowShadeTexture);
  this->EDLLowFBO->SetActiveBuffer(0);
  this->EDLLowFBO->SetDepthBufferNeeded(false);

  this->EDLLowShadeTexture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->EDLLowShadeTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  this->EDLLowShadeTexture->SetMinificationFilter(vtkTextureObject::Linear);
  this->EDLLowShadeTexture->SetLinearMagnification(true);
  this->EDLLowShadeTexture->Bind();
  this->EDLLowShadeTexture->SendParameters();

  this->EDLLowBlurTexture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->EDLLowBlurTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  this->EDLLowBlurTexture->SetMinificationFilter(vtkTextureObject::Linear);
  this->EDLLowBlurTexture->SetLinearMagnification(true);
  this->EDLLowBlurTexture->Bind();
  this->EDLLowBlurTexture->SendParameters();

  this->EDLLowFBO->UnBind();

  vtkOpenGLCheckErrorMacro("failed after Initialize");
}
// ----------------------------------------------------------------------------
// Description:
// Initialize shaders
//
void vtkEDLShading::EDLInitializeShaders(vtkOpenGLRenderWindow *renWin)
{
#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: INITIALIZE SHADERS" << endl;
#endif

  //  EDL SHADE
  //
  if (this->EDLShadeProgram.Program == 0)
  {
    this->EDLShadeProgram.Program =
      renWin->GetShaderCache()->ReadyShaderProgram(
        vtkTextureObjectVS,
        vtkEDLShadeFS,
        "");
  }

  //  EDL COMPOSE
  //
  if (this->EDLComposeProgram.Program == 0)
  {
    this->EDLComposeProgram.Program =
      renWin->GetShaderCache()->ReadyShaderProgram(
        vtkTextureObjectVS,
        vtkEDLComposeFS,
        "");
  }

  //  BILATERAL FILTER
  //
  if (this->BilateralProgram.Program == 0)
  {
    this->BilateralProgram.Program =
      renWin->GetShaderCache()->ReadyShaderProgram(
        vtkTextureObjectVS,
        vtkEDLBilateralFilterFS,
        "");
  }

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... done" << endl;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Render EDL in full resolution
//
bool vtkEDLShading::EDLShadeHigh(
  vtkRenderState &s,
  vtkOpenGLRenderWindow *renWin)
{
  //  VARIABLES
  //
  float d = 1.0;
  float F_scale = 5.0;
  float SX = 1. / float(this->W);
  float SY = 1. / float(this->H);
  float L[3] =
    { 0., 0., -1. };

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->EDLShadeProgram.Program);

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLHighFBO);
  this->EDLHighShadeTexture->Activate();
  this->EDLHighFBO->SetColorBuffer(0, this->EDLHighShadeTexture);
  this->EDLHighFBO->SetActiveBuffer(0);
  this->EDLHighFBO->Start(this->W, this->H, false);

  // DEPTH TEXTURE PARAMETERS
  this->ProjectionDepthTexture->Activate();
  vtkShaderProgram *prog = this->EDLShadeProgram.Program;

  // shader parameters
  prog->SetUniformi("s2_depth", this->ProjectionDepthTexture->GetTextureUnit());
  prog->SetUniformf("d", d);
  prog->SetUniformf("F_scale", F_scale);
  prog->SetUniformf("SX", SX);
  prog->SetUniformf("SY", SY);
  prog->SetUniform3f("L", L);
  prog->SetUniform4fv("N", 8, this->EDLNeighbours);
  prog->SetUniformf("Znear", this->Zn);
  prog->SetUniformf("Zfar", this->Zf);

  // compute the scene bounding box, and set the scene size to the diagonal of it.
  double bb[6];
  vtkMath::UninitializeBounds(bb);
  bool boundsSet = false;
  for(int i=0; i<s.GetPropArrayCount(); i++)
  {
    double* bounds = s.GetPropArray()[i]->GetBounds();
    if (bounds)
    {
      if(!boundsSet)
      {
        bb[0] = bounds[0];
        bb[1] = bounds[1];
        bb[2] = bounds[2];
        bb[3] = bounds[3];
        bb[4] = bounds[4];
        bb[5] = bounds[5];
        boundsSet = true;
      }
      else
      {
        bb[0] = (bb[0] < bounds[0] ? bb[0] : bounds[0]);
        bb[1] = (bb[1] > bounds[1] ? bb[1] : bounds[1]);
        bb[2] = (bb[2] < bounds[2] ? bb[2] : bounds[2]);
        bb[3] = (bb[3] > bounds[3] ? bb[3] : bounds[3]);
        bb[4] = (bb[4] < bounds[4] ? bb[4] : bounds[4]);
        bb[5] = (bb[5] > bounds[5] ? bb[5] : bounds[5]);
      }
    }
  }
  float diag = (bb[1]-bb[0])*(bb[1]-bb[0]) + (bb[3]-bb[2])*(bb[3]-bb[2])
               + (bb[5]-bb[4])*(bb[5]-bb[4]);
  diag = sqrt(diag);
  prog->SetUniformf("SceneSize", diag);

  // RENDER AND FREE ALL
  this->EDLHighFBO->RenderQuad(0, this->W - 1, 0, this->H - 1,
    prog, this->EDLShadeProgram.VAO);

  //
  this->ProjectionDepthTexture->Deactivate();
  this->EDLHighShadeTexture->Deactivate();
  this->EDLHighFBO->UnBind();

  return true; // succeeded
}

// ----------------------------------------------------------------------------
// Description:
// Render EDL in low resolution
//
bool vtkEDLShading::EDLShadeLow(
  vtkRenderState &s,
  vtkOpenGLRenderWindow *renWin)
{
  //  VARIABLES
  //
  float d = 2.0;
  float F_scale = 5.0;
  float SX = 1. / float(this->W / this->EDLLowResFactor);
  float SY = 1. / float(this->H / this->EDLLowResFactor);
  float L[3] =
    { 0., 0., -1. };

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLLowFBO);
  this->EDLLowShadeTexture->Activate();
  this->EDLLowShadeTexture->SetLinearMagnification(true);
  this->EDLLowShadeTexture->SendParameters();
  this->EDLLowFBO->SetColorBuffer(0, this->EDLLowShadeTexture);
  this->EDLLowFBO->SetActiveBuffer(0);
  this->EDLLowFBO->Start(this->W / this->EDLLowResFactor,
    this->H / this->EDLLowResFactor, false);

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->EDLShadeProgram.Program);
  // DEPTH TEXTURE PARAMETERS
  vtkShaderProgram *prog = this->EDLShadeProgram.Program;
  this->ProjectionDepthTexture->Activate();
  // shader parameters
  prog->SetUniformi("s2_depth", this->ProjectionDepthTexture->GetTextureUnit());
  prog->SetUniformf("d", d);
  prog->SetUniformf("F_scale", F_scale);
  prog->SetUniformf("SX", SX);
  prog->SetUniformf("SY", SY);
  prog->SetUniform3f("L", L);
  prog->SetUniform4fv("N", 8, this->EDLNeighbours); // USELESS, ALREADY DEFINED IN FULL RES
  prog->SetUniformf("Znear", this->Zn);
  prog->SetUniformf("Zfar", this->Zf);

  // RENDER AND FREE ALL
  //
  this->EDLLowFBO->RenderQuad(0, this->W / this->EDLLowResFactor - 1,
    0, this->H / this->EDLLowResFactor - 1,
    prog, this->EDLShadeProgram.VAO);

  this->ProjectionDepthTexture->Deactivate();
  this->EDLLowShadeTexture->Deactivate();
  this->EDLLowFBO->UnBind();

  return true; // succeeded
}

// ----------------------------------------------------------------------------
// Description:
// Bilateral Filter low resolution shaded image
//
bool vtkEDLShading::EDLBlurLow(vtkRenderState &s,
  vtkOpenGLRenderWindow *renWin)
{
  // shader parameters
  float SX = 1. / float(this->W / this->EDLLowResFactor);
  float SY = 1. / float(this->H / this->EDLLowResFactor);
  int EDL_Bilateral_N = 5;
  float EDL_Bilateral_Sigma = 2.5;

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->BilateralProgram.Program);

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLLowFBO);
  this->EDLLowBlurTexture->Activate();
  this->EDLLowFBO->SetColorBuffer(0, this->EDLLowBlurTexture);
  this->EDLLowFBO->SetActiveBuffer(0);
  this->EDLLowFBO->Start(this->W / EDLLowResFactor, this->H / EDLLowResFactor,
      false);

  // DEPTH TEXTURE PARAMETERS
  vtkShaderProgram *prog = this->BilateralProgram.Program;

  // DEPTH TEXTURE PARAMETERS
  this->EDLLowShadeTexture->Activate();
  this->ProjectionDepthTexture->Activate();

  // shader parameters
  prog->SetUniformi("s2_I", this->EDLLowShadeTexture->GetTextureUnit());
  prog->SetUniformi("s2_D", this->ProjectionDepthTexture->GetTextureUnit());
  prog->SetUniformf("SX", SX);
  prog->SetUniformf("SY", SY);
  prog->SetUniformi("N", EDL_Bilateral_N);
  prog->SetUniformf("sigma", EDL_Bilateral_Sigma);

  this->EDLLowFBO->RenderQuad(
    0, this->W / this->EDLLowResFactor - 1,
    0, this->H / this->EDLLowResFactor - 1,
    prog, this->BilateralProgram.VAO);

  this->EDLLowBlurTexture->Deactivate();
  this->EDLLowShadeTexture->Deactivate();
  this->ProjectionDepthTexture->Deactivate();

  this->EDLLowFBO->UnBind();

  return EDLIsFiltered;
}

// ----------------------------------------------------------------------------
// Description:
// Compose color and shaded images
//
bool vtkEDLShading::EDLCompose(const vtkRenderState *,
  vtkOpenGLRenderWindow *renWin)
{
  //  this->EDLIsFiltered = true;

  // ACTIVATE SHADER
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->EDLComposeProgram.Program);
  // DEPTH TEXTURE PARAMETERS
  vtkShaderProgram *prog = this->EDLComposeProgram.Program;

  //  EDL shaded texture - full res
  this->EDLHighShadeTexture->Activate();
  prog->SetUniformi("s2_S1", this->EDLHighShadeTexture->GetTextureUnit());

  //  EDL shaded texture - low res
  //this->EDLLowBlurTexture->SetLinearMagnification(true);
  //this->EDLLowBlurTexture->SendParameters();
  if (this->EDLIsFiltered)
  {
    this->EDLLowBlurTexture->Activate();
    prog->SetUniformi("s2_S2", this->EDLLowBlurTexture->GetTextureUnit());
  }
  else
  {
    this->EDLLowShadeTexture->Activate();
    prog->SetUniformi("s2_S2", this->EDLLowShadeTexture->GetTextureUnit());
  }

  //  initial color texture
  this->ProjectionColorTexture->Activate();
  prog->SetUniformi("s2_C", this->ProjectionColorTexture->GetTextureUnit());

  //  initial depth texture
  this->ProjectionDepthTexture->Activate();
  prog->SetUniformi("s2_Z", this->ProjectionDepthTexture->GetTextureUnit());

  //  DRAW CONTEXT - prepare blitting
  //
  // Prepare blitting
  glClearColor(1., 1., 1., 1.);
#if GL_ES_VERSION_2_0 == 1
  glClearDepthf(static_cast<GLclampf>(1.0));
#else
  glClearDepth(static_cast<GLclampf>(1.0));
#endif
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // IMPORTANT since we enable depth writing hereafter
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  // IMPORTANT : so that depth information is propagated
  glDisable(GL_SCISSOR_TEST);

  this->EDLHighShadeTexture->CopyToFrameBuffer( 0,  0,
      this->W - 1 - 2 * this->ExtraPixels,
      this->H - 1 - 2 * this->ExtraPixels, 0, 0,
      this->Width, this->Height,
      prog, this->EDLComposeProgram.VAO );

  //  FREE ALL
  //
  if (this->EDLIsFiltered)
  {
    this->EDLLowBlurTexture->Deactivate();
  }
  else
  {
    this->EDLLowShadeTexture->Deactivate();
  }
  this->EDLHighShadeTexture->Deactivate();
  this->ProjectionColorTexture->Deactivate();
  this->ProjectionDepthTexture->Deactivate();

  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkEDLShading::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps = 0;
  vtkRenderer *r = s->GetRenderer();
  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());

  if (this->DelegatePass != 0)
  {
#if GL_ES_VERSION_2_0 != 1
    GLint savedCurrentDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER,&savedCurrentDrawBuffer);
#endif

    //////////////////////////////////////////////////////
    //
    //  2. DEFINE SIZE and ACCORDING RENDER STATE
    //
    this->ReadWindowSize(s);
    // this->extraPixels = 20; Obsolete
    this->ExtraPixels = 0; // extra pixels to zero in the new system
    this->W = this->Width + 2* this ->ExtraPixels;
    this->H=this->Height+2*this->ExtraPixels;
    vtkRenderState s2(r);
    s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

    //////////////////////////////////////////////////////
    //
    // 3. INITIALIZE FBOs and SHADERS
    //
    //  FBOs
    //
    this->EDLInitializeFramebuffers(s2);
    //  Shaders
    //
    this->EDLInitializeShaders(renWin);

    //////////////////////////////////////////////////////
    //
    // 4. DELEGATE RENDER IN PROJECTION FBO
    //
    //
    double znear,zfar;
    r->GetActiveCamera()->GetClippingRange(znear,zfar);
    this->Zf = zfar;
    this->Zn = znear;
    //cout << " -- ZNEAR/ZFAR : " << Zn << " || " << Zf << endl;
    this->ProjectionFBO->Bind();
    this->RenderDelegate(s,this->Width,this->Height,
         this->W,this->H,this->ProjectionFBO,
         this->ProjectionColorTexture,this->ProjectionDepthTexture);

    this->ProjectionFBO->UnBind();

    //system("PAUSE");

    //////////////////////////////////////////////////////
    //
    // 5. EDL SHADING PASS - FULL RESOLUTION
    //
#if EDL_HIGH_RESOLUTION_ON
    if(! this->EDLShadeHigh(s2,renWin) )
    {
      glDrawBuffer(savedCurrentDrawBuffer);
    }
#endif // EDL_HIGH_RESOLUTION_ON

    //////////////////////////////////////////////////////
    //
    // 6. EDL SHADING PASS - LOW RESOLUTION + blur pass
    //
#if EDL_LOW_RESOLUTION_ON
    if(! this->EDLShadeLow(s2, renWin) )
    {
#if GL_ES_VERSION_2_0 != 1
      glDrawBuffer(static_cast<GLenum>(savedCurrentDrawBuffer));
#endif
    }
    if (this->EDLIsFiltered)
    {
      this->EDLBlurLow(s2, renWin);
    }
#endif // EDL_LOW_RESOLUTION_ON

    //////////////////////////////////////////////////////
    //
    // 7. COMPOSITING PASS (in original framebuffer)
    //
    if(s->GetFrameBuffer() != NULL)
    {
      vtkFrameBufferObject::SafeDownCast(s->GetFrameBuffer())->Bind();
    }
#if GL_ES_VERSION_2_0 != 1
    glDrawBuffer(static_cast<GLenum>(savedCurrentDrawBuffer));
#endif

    if( ! this->EDLCompose(s, renWin))
    {
      return;
    }
  }
  else
  {
    vtkWarningMacro(<<" no delegate.");
  }
}

// --------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkEDLShading::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  //  SHADERS
  this->EDLShadeProgram.ReleaseGraphicsResources(w);
  this->EDLComposeProgram.ReleaseGraphicsResources(w);
  this->BilateralProgram.ReleaseGraphicsResources(w);

  // FBOs and TOs
  //
  if (this->ProjectionFBO != 0)
  {
    this->ProjectionFBO->Delete();
    this->ProjectionFBO = 0;
  }
  if (this->ProjectionColorTexture != 0)
  {
    this->ProjectionColorTexture->Delete();
    this->ProjectionColorTexture = 0;
  }
  if (this->ProjectionDepthTexture != 0)
  {
    this->ProjectionDepthTexture->Delete();
    this->ProjectionDepthTexture = 0;
  }
  if (this->EDLHighFBO != 0)
  {
    this->EDLHighFBO->Delete();
    this->EDLHighFBO = 0;
  }
  if (this->EDLHighShadeTexture != 0)
  {
    this->EDLHighShadeTexture->Delete();
    this->EDLHighShadeTexture = 0;
  }
  if (this->EDLLowFBO != 0)
  {
    this->EDLLowFBO->Delete();
    this->EDLLowFBO = 0;
  }
  if (this->EDLLowShadeTexture != 0)
  {
    this->EDLLowShadeTexture->Delete();
    this->EDLLowShadeTexture = 0;
  }
  if (this->EDLLowBlurTexture != 0)
  {
    this->EDLLowBlurTexture->Delete();
    this->EDLLowBlurTexture = 0;
  }

  this->Superclass::ReleaseGraphicsResources(w);
}
