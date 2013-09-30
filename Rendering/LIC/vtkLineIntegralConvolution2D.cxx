/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineIntegralConvolution2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLineIntegralConvolution2D.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkShader2.h"
#include "vtkShaderProgram2.h"
#include "vtkUniformVariables.h"
#include "vtkShader2Collection.h"
#include "vtkTextureObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkFrameBufferObject2.h"
#include "vtkPixelExtent.h"
#include "vtkPainterCommunicator.h"
#include "vtkFloatArray.h"
#include "vtkTimerLog.h"
#include "vtkMath.h"
#include "vtkgl.h"
#include "vtkOpenGLError.h"

#include <vector>
#include <string>
#include <algorithm>

using std::deque;
using std::vector;
using std::string;

// Enable stream min/max computations. Streaming is accomplished
// via PBO+glReadPixels to read just the regions we are updating.
// Without streaming PBO+glGetTexImage is used to uplaod the entire
// screen sized texture, of which (in parallel) we are updating only
// a small part of.
#define STREAMING_MIN_MAX

// if you don't explicitly bind to 0 before swapping on some
// systems (intel hd4000) then things get whacky. nvidia devices
// are fine without this.
#define NOT_NVIDIA

// here have to setup the activate textures *before* calling use
// program looks like it's a bug in the intel driver.
// Intel GL 4.0.0 - Build 9.17.10.2932 GLSL 4.00 - Build 9.17.10.2932
#define INTEL_BUG

// if defined write intermediate results to disk
// for debugging. (1 results, 2 +steps, 3 +fbo status )
#define vtkLineIntegralConvolution2DDEBUG 0
#if (vtkLineIntegralConvolution2DDEBUG >= 1)
#include "vtkTextureIO.h"
#include <sstream>
using std::ostringstream;
//----------------------------------------------------------------------------
static
string mpifn(int rank, const char *fn)
{
  ostringstream oss;
  oss << rank << "_" << fn;
  return oss.str();
}
#endif
#if vtkLineIntegralConvolution2DDEBUG >= 3
#define vtkLICCheckFrameBufferStatusMacro(mode) vtkCheckFrameBufferStatusMacro(mode)
#else
#define vtkLICCheckFrameBufferStatusMacro(mode)
#endif

// shader sources
extern const char *vtkLineIntegralConvolution2D_VT;   // normalized image space transform
extern const char *vtkLineIntegralConvolution2D_LIC0; // initialization for lic
extern const char *vtkLineIntegralConvolution2D_LICI; // compute i'th lic step
extern const char *vtkLineIntegralConvolution2D_LICN; // finalize lic
extern const char *vtkLineIntegralConvolution2D_EE;   // Laplace edge-enhance
extern const char *vtkLineIntegralConvolution2D_CE;   // contrast enhance
extern const char *vtkLineIntegralConvolution2D_AAH;  // horizontal part of anti-alias filter
extern const char *vtkLineIntegralConvolution2D_AAV;  // vertical part of anti-alias filter

#if defined(NDEBUG) || (vtkLineIntegralConvolution2DDEBUG < 3)
# define DEBUG3CheckFrameBufferStatusMacro(mode)
#else
# define DEBUG3CheckFrameBufferStatusMacro(mode) \
    vtkStaticCheckFrameBufferStatusMacro(mode)
#endif

/// vtkLICPingPongBufferManager -- gpgpu buffer manager
/**
Helper that manages state for the ping-pong buffer strategy
employed during LIC integration. This class encapsulates all
of the knowledge of our use of the FBO and texture units. Care
is taken to avoid feedback loops.
*/
class vtkLICPingPongBufferManager
{
public:
  vtkLICPingPongBufferManager(
      vtkFrameBufferObject2 *fbo,
      unsigned int *bufSize,
      vtkTextureObject *vectorTexture,
      vtkTextureObject *maskVectorTexture,
      vtkTextureObject *noiseTexture,
      int doEEPass,
      int doVTPass)
    {
    this->VectorTexture = vectorTexture;
    this->MaskVectorTexture = maskVectorTexture;
    this->NoiseTexture = noiseTexture;

    // allocate buffers
    vtkRenderWindow *context = fbo->GetContext();
    this->LICTexture0 = this->AllocateLICBuffer(context, bufSize);
    this->SeedTexture0 = this->AllocateLICBuffer(context, bufSize);
    this->LICTexture1 = this->AllocateLICBuffer(context, bufSize);
    this->SeedTexture1 = this->AllocateLICBuffer(context, bufSize);
    this->EETexture = doEEPass ? this->AllocateNoiseBuffer(context, bufSize) : NULL;
    this->ImageVectorTexture = doVTPass ? this->AllocateVectorBuffer(context, bufSize) : NULL;

    this->DettachBuffers();

    // setup pairs for buffer ping-pong
    this->PingTextures[0] = this->LICTexture0->GetHandle();
    this->PingTextures[1] = this->SeedTexture0->GetHandle();

    this->PongTextures[0] = this->LICTexture1->GetHandle();
    this->PongTextures[1] = this->SeedTexture1->GetHandle();

    this->Textures[0] = this->PingTextures;
    this->Textures[1] = this->PongTextures;

    this->ReadIndex = 0;

    #if vtkLineIntegralConvolution2DDEBUG >= 3
    this->Print(cerr);
    #endif
    }

  ~vtkLICPingPongBufferManager()
    {
    // free buffers
    this->LICTexture0->Delete();
    this->SeedTexture0->Delete();
    this->LICTexture1->Delete();
    this->SeedTexture1->Delete();
    if (this->EETexture)
      {
      this->EETexture->Delete();
      }
    if (this->ImageVectorTexture)
      {
      this->ImageVectorTexture->Delete();
      }
    }

  // Description:
  // Get the unit/unit id for the given texture
  // Here is how we use tetxure units.
  //   name         | unit
  //   -------------+--------
  //   vectors      | 0
  //   mask vectors | 0/1
  //   noise        | 2
  //   lic          | 3
  //   seeds        | 4
  int GetVectorTextureUnit(){ return 0; }
  int GetMaskVectorTextureUnit(){ return this->MaskVectorUnit; }
  int GetNoiseTextureUnit(){ return 2; }
  int GetLICTextureUnit(){ return 3; }
  int GetSeedTextureUnit(){ return 4; }

  // Description:
  // Switch input and output buffers
  void Swap(){ this->ReadIndex = 1 - this->ReadIndex; }

  // Description:
  // Get the last output (assumes a swap has been done).
  vtkTextureObject *GetLastLICBuffer()
    {
    return this->ReadIndex == 0 ? this->LICTexture0 : this->LICTexture1;
    }

  // Description:
  // Get the last output (assumes a swap has been done).
  vtkTextureObject *GetLastSeedBuffer()
    {
    return this->ReadIndex == 0 ? this->SeedTexture0 : this->SeedTexture1;
    }

  // Description:
  // Get the last output (assumes a swap has been done).
  vtkTextureObject *GetLICBuffer()
    {
    return 1-this->ReadIndex == 0 ? this->LICTexture0 : this->LICTexture1;
    }

  // Description:
  // Get the last output (assumes a swap has been done).
  vtkTextureObject *GetSeedBuffer()
    {
    return 1-this->ReadIndex == 0 ? this->SeedTexture0 : this->SeedTexture1;
    }

  // Description:
  // Clear all the buffers used for writing.
  void ClearBuffers(
        vtkFrameBufferObject2 *fbo,
        const vtkPixelExtent &viewExt,
        const deque<vtkPixelExtent> &extents,
        int clearEETex = 0)
    {
    //attach
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, this->LICTexture0);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U, this->SeedTexture0);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 2U, this->LICTexture1);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 3U, this->SeedTexture1);
    unsigned int num = 4U;
    if (clearEETex)
      {
      fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 4U, this->EETexture);
      num = 5U;
      }
    fbo->ActivateDrawBuffers(num);
    DEBUG3CheckFrameBufferStatusMacro(vtkgl::DRAW_FRAMEBUFFER_EXT);

    // clear the parts of the screen which we will modify
    // initially mask all fragments
    glClearColor(0.0, 1.0, 0.0, 0.0);
    #if 0
    glClear(GL_COLOR_BUFFER_BIT);
    #else
    glEnable(GL_SCISSOR_TEST);
    size_t nBlocks = extents.size();
    for (size_t e=0; e<nBlocks; ++e)
      {
      vtkPixelExtent ext = extents[e];
      // add halo for linear filtering
      // since at most linear filtering requires
      // 4 pixels , clearing an extra 4 here
      // ensures we never access uninitialized
      // memory.
      ext.Grow(4);
      ext &= viewExt;

      unsigned int extSize[2];
      ext.Size(extSize);

      glScissor(ext[0], ext[2], extSize[0], extSize[1]);
      glClear(GL_COLOR_BUFFER_BIT);
      }
    glDisable(GL_SCISSOR_TEST);
    #endif
    // detach
    fbo->RemoveTexColorAttachments(vtkgl::DRAW_FRAMEBUFFER_EXT, num);
    fbo->DeactivateDrawBuffers();
    }

  // Description:
  // Clear the given buffer
  void ClearBuffer(
        vtkFrameBufferObject2 *fbo,
        vtkTextureObject *tex,
        const vtkPixelExtent &viewExt,
        const deque<vtkPixelExtent> &extents)
    {
    //attach
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, tex);
    fbo->ActivateDrawBuffers(1);
    DEBUG3CheckFrameBufferStatusMacro(vtkgl::DRAW_FRAMEBUFFER_EXT);

    // clear the parts of the screen which we will modify
    // initially mask all fragments
    glClearColor(0.0, 1.0, 0.0, 0.0);
    #if 0
    glClear(GL_COLOR_BUFFER_BIT);
    #else
    glEnable(GL_SCISSOR_TEST);
    size_t nBlocks = extents.size();
    for (size_t e=0; e<nBlocks; ++e)
      {
      vtkPixelExtent ext = extents[e];
      // add halo for linear filtering
      // since at most linear filtering requires
      // 4 pixels , clearing an extra 4 here
      // ensures we never access uninitialized
      // memory.
      ext.Grow(4);
      ext &= viewExt;

      unsigned int extSize[2];
      ext.Size(extSize);

      glScissor(ext[0], ext[2], extSize[0], extSize[1]);
      glClear(GL_COLOR_BUFFER_BIT);
      }
    glDisable(GL_SCISSOR_TEST);
    #endif
    // detach
    fbo->RemoveTexColorAttachments(vtkgl::DRAW_FRAMEBUFFER_EXT, 1);
    fbo->DeactivateDrawBuffers();
    }

  // Description:
  // Activates the input textures. These are read only.
  void AttachVectorTextures()
    {
    // vector
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    if (this->ImageVectorTexture)
      {
      glBindTexture( GL_TEXTURE_2D, this->ImageVectorTexture->GetHandle());
      }
    else
      {
      glBindTexture( GL_TEXTURE_2D, this->VectorTexture->GetHandle());
      }
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");

    // mask vectors (optional)
    vtkgl::ActiveTexture(vtkgl::TEXTURE1);
    if (this->MaskVectorTexture)
      {
      glBindTexture(GL_TEXTURE_2D, this->MaskVectorTexture->GetHandle());
      vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
      this->MaskVectorUnit = 1;
      }
    else
      {
      glBindTexture(GL_TEXTURE_2D, 0);
      this->MaskVectorUnit = 0;
      }
    }

  // Description:
  // Deactivates the input noise textures.
  void DettachVectorTextures()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    vtkgl::ActiveTexture(vtkgl::TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    }

  // Description:
  // Activate the read only noise texture. It's active for
  // the entirety of each LIC pass.
  void AttachNoiseTexture(int LICPassNum = 0)
    {
    switch (LICPassNum)
      {
      case 0:
        this->NoiseTexture->Activate(vtkgl::TEXTURE2);
        break;
      case 1:
        this->EETexture->Activate(vtkgl::TEXTURE2);
        break;
      }
    }

  // Dsecription:
  // Deactivate the inpyt noise tetxure.
  void DettachNoiseTexture()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    }

  // Description:
  // Setup read/write from/to the active lic/seed buffer texture pair
  // for LIC pass.
  void AttachLICBuffers()
    {
    unsigned int *readTex = this->Textures[this->ReadIndex];
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, readTex[0]);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");

    vtkgl::ActiveTexture(vtkgl::TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, readTex[1]);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");

    unsigned int *writeTex = this->Textures[1-this->ReadIndex];
    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          writeTex[0],
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebuffereadTexture2D");

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT1,
          GL_TEXTURE_2D,
          writeTex[1],
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebuffereadTexture2D");

    GLenum atts[2] = {
          vtkgl::COLOR_ATTACHMENT0,
          vtkgl::COLOR_ATTACHMENT1
          };
    vtkgl::DrawBuffers(2, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");

    DEBUG3CheckFrameBufferStatusMacro(vtkgl::DRAW_FRAMEBUFFER_EXT);
    }

  // Description:
  // Remove input/output bufers used for computing the LIC.
  void DettachLICBuffers()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    vtkgl::ActiveTexture(vtkgl::TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT1,
          GL_TEXTURE_2D,
          0U,
          0);

    GLenum atts[1] = {GL_NONE};
    vtkgl::DrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
    }

  // Description:
  // Attach read/write buffers for transform pass.
  void AttachImageVectorBuffer()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->VectorTexture->GetHandle());
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          this->ImageVectorTexture->GetHandle(),
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    GLenum atts[1] = {vtkgl::COLOR_ATTACHMENT0};
    vtkgl::DrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");

    DEBUG3CheckFrameBufferStatusMacro(vtkgl::DRAW_FRAMEBUFFER_EXT);
    }

  // Description:
  // Attach read/write buffers for transform pass.
  void DettachImageVectorBuffer()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);

    GLenum atts[1] = {GL_NONE};
    vtkgl::DrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
    }

  // Description:
  // Attach read/write buffers for EE pass.
  void AttachEEBuffer()
    {
    unsigned int *readTex = this->Textures[this->ReadIndex];
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, readTex[0]);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          this->EETexture->GetHandle(),
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    GLenum atts[1] = {vtkgl::COLOR_ATTACHMENT0};
    vtkgl::DrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");

    DEBUG3CheckFrameBufferStatusMacro(vtkgl::DRAW_FRAMEBUFFER_EXT);
    }

  // Description:
  // Attach read/write buffers for EE pass.
  void DettachEEBuffer()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);

    GLenum atts[1] = {GL_NONE};
    vtkgl::DrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
    }

  // Description:
  // Deactivates and removes all read/write buffers that were in
  // use during the run, restoring a pristine FBO/texture unit state.
  void DettachBuffers()
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0U);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    vtkgl::ActiveTexture(vtkgl::TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0U);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    vtkgl::ActiveTexture(vtkgl::TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0U);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0U);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");
    vtkgl::ActiveTexture(vtkgl::TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0U);
    vtkOpenGLStaticCheckErrorMacro("failed at glBindTexture");

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    vtkgl::FramebufferTexture2DEXT(
          vtkgl::DRAW_FRAMEBUFFER_EXT,
          vtkgl::COLOR_ATTACHMENT1,
          GL_TEXTURE_2D,
          0U,
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    GLenum none = GL_NONE;
    vtkgl::DrawBuffers(1, &none);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
    }

  // Description:
  // Get the read/write ids
  int GetReadIndex(){ return this->ReadIndex; }
  int GetWriteIndex(){ return 1 - this->ReadIndex; }

  // Description:
  // Allocate a texture of the given size.
  // with parameters for LIC lookups
  vtkTextureObject *AllocateLICBuffer(
        vtkRenderWindow *context,
        unsigned int texSize[2])
    {
    float border[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    return this->AllocateBuffer(
          context,
          texSize,
          vtkTextureObject::Nearest,
          vtkTextureObject::ClampToBorder,
          border);
    }

  // Description:
  // Allocate a texture of the given size.
  // with parameters for noise lookups
  vtkTextureObject *AllocateNoiseBuffer(
        vtkRenderWindow *context,
        unsigned int texSize[2])
    {
    float border[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    return this->AllocateBuffer(
          context,
          texSize,
          vtkTextureObject::Nearest,
          vtkTextureObject::ClampToEdge,
          border);
    }

  // Description:
  // Allocate a texture of the given size.
  // with parameters for LIC
  vtkTextureObject *AllocateVectorBuffer(
        vtkRenderWindow *context,
        unsigned int texSize[2])
    {
    float border[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    return this->AllocateBuffer(
          context,
          texSize,
          vtkTextureObject::Linear,
          vtkTextureObject::ClampToBorder,
          border);
    }

  // Description:
  // Allocate a texture of the given size.
  vtkTextureObject *AllocateBuffer(
        vtkRenderWindow *context,
        unsigned int texSize[2],
        int filter,
        int wrapping,
        float *borderColor)
    {
    vtkTextureObject *tex = vtkTextureObject::New();
    tex->SetContext(context);
    tex->SetBaseLevel(0);
    tex->SetMaxLevel(0);
    tex->SetBorderColor(borderColor);
    tex->SetWrapS(wrapping);
    tex->SetWrapT(wrapping);
    tex->SetMinificationFilter(filter);  // no guard pixels
    tex->SetMagnificationFilter(filter); // no guard pixels
    tex->Create2D(texSize[0], texSize[1], 4, VTK_FLOAT, false);
    tex->SetAutoParameters(0);
    return tex;
    }

  // Description:
  // Render screen aligned quad. Texture coordinates are
  // always assigned on TEXTURE0, it's hardcoded in the
  // shaders.
  void RenderQuad(
          float computeBoundsPt0[2],
          float computeBoundsPt1[2],
          vtkPixelExtent computeExtent)
    {
    float computeBounds[4] = {
          computeBoundsPt0[0], computeBoundsPt1[0],
          computeBoundsPt0[1], computeBoundsPt1[1]
          };
     this->RenderQuad(computeBounds, computeExtent);
    }

  // Description:
  // Render screen aligned quad. Texture coordinates are
  // always assigned on TEXTURE0, it's hardcoded in the
  // shaders.
  void RenderQuad(
          float computeBounds[4],
          vtkPixelExtent computeExtent)
    {
    int quadPtIds[8] = {0,2, 1,2, 1,3, 0,3};

    float quadBounds[4];
    computeExtent.CellToNode();
    computeExtent.GetData(quadBounds);

    glBegin(GL_QUADS);
    for (int q=0; q<4; ++q)
      {
      int qq = 2*q;

      vtkgl::MultiTexCoord2f(
            vtkgl::TEXTURE0,
            computeBounds[quadPtIds[qq]],
            computeBounds[quadPtIds[qq+1]]);

      glVertex2f(
            quadBounds[quadPtIds[qq]],
            quadBounds[quadPtIds[qq+1]]);
      }
    glEnd();
    vtkOpenGLStaticCheckErrorMacro("failed at render quad");
    }

  #if (vtkLineIntegralConvolution2DDEBUG >= 1)
  // Description:
  // Write the last output buffers to disk (assumes a swap has
  // already been done)
  void WriteBuffers(
      int rank,
      const char *licFileName,
      const char *seedFileName,
      const deque<vtkPixelExtent>& exts)
    {
    if (licFileName)
      {
      vtkTextureIO::Write(
              mpifn(rank, licFileName),
              this->GetLastLICBuffer(),
              exts);
      }
    if (seedFileName)
      {
      vtkTextureIO::Write(
              mpifn(rank, seedFileName),
              this->GetLastSeedBuffer(),
              exts);
      }
    }
  void WriteEEBuffer(int rank, const deque<vtkPixelExtent> &exts)
    {
    vtkTextureIO::Write(
          mpifn(rank,"lic2d_ee.vtm"),
          this->EETexture,
          exts);
    }
  void WriteImageVectorBuffer(int rank, const deque<vtkPixelExtent> &exts)
    {
    vtkTextureIO::Write(
          mpifn(rank,"lic2d_ivec.vtm"),
          this->ImageVectorTexture,
          exts);
    }
  void WriteInputs(int rank, const deque<vtkPixelExtent>& exts)
    {
    vtkTextureIO::Write(
              mpifn(rank,"lic2d_vec.vtm"),
              this->VectorTexture,
              exts);
    if (this->MaskVectorTexture)
      {
      vtkTextureIO::Write(
                mpifn(rank,"lic2d_mask.vtm"),
                this->MaskVectorTexture,
                exts);
      }
    vtkTextureIO::Write(
              mpifn(rank,"lic2d_noise.vtk"),
              this->NoiseTexture);
    }

  // Description:
  // Print current state to the given stream
  void Print(ostream &os)
    {
    os
      << "Vectors = " << this->VectorTexture->GetHandle() << endl
      << "ImageVectors = " << this->ImageVectorTexture->GetHandle() << endl
      << "MaskVectors = " << (this->MaskVectorTexture ? this->MaskVectorTexture->GetHandle() : 0U) << endl
      << "Noise = " << this->NoiseTexture->GetHandle() << endl
      << "EE = " << (this->EETexture ? this->EETexture->GetHandle() : 0U) << endl
      << "LIC0 = " << this->LICTexture0->GetHandle() << endl
      << "Seed0 = " << this->SeedTexture0->GetHandle() << endl
      << "LIC1 = " << this->LICTexture1->GetHandle() << endl
      << "Seed1 = " << this->SeedTexture1->GetHandle() << endl
      << "ReadIndex=" << this->ReadIndex << endl
      << "PingTextures[0]=" << this->Textures[0][0] << ", " << this->Textures[0][1] << endl
      << "PongTextures[1]=" << this->Textures[1][0] << ", " << this->Textures[1][1] << endl;
    }
  #endif

private:
  vtkTextureObject *VectorTexture;
  vtkTextureObject *ImageVectorTexture;
  vtkTextureObject *MaskVectorTexture;
  vtkTextureObject *NoiseTexture;
  vtkTextureObject *EETexture;
  vtkTextureObject *LICTexture0;
  vtkTextureObject *SeedTexture0;
  vtkTextureObject *LICTexture1;
  vtkTextureObject *SeedTexture1;
  int MaskVectorUnit;

  int  ReadIndex;
  unsigned int PingTextures[2];
  unsigned int PongTextures[2];
  unsigned int *Textures[2];
};

namespace vtkLineIntegralConvolution2DUtil
{
/**
glsl shader code for selecting vector comps.
*/
string GetComponentSelectionProgram(int *compIds)
{
  // swizles at 45,46
  string srcCode("vec2 getSelectedComponents(vec4 V){ return V.$$; }");
  const char *compNames = "xyzw";
  srcCode[45] = compNames[compIds[0]];
  srcCode[46] = compNames[compIds[1]];
  return srcCode;
}

/*
Shader code for looking up vectors
*/
const char *GetVectorLookupProgram(int normalize)
{
  // lookup the vector and normalize
  const char *getNormVecSrc = " \
    uniform sampler2D texVectors;\n \
    vec2 getVector( vec2 vectc )\n \
      {\n \
      vec2 V = texture2D( texVectors, vectc ).xy;\n \
      // normalize if |V| not 0\n \
      float lenV = length( V );\n \
      if ( lenV > 1.0e-8 )\n \
        {\n \
        return V/lenV;\n \
        }\n \
      else\n \
        {\n \
        return vec2( 0.0, 0.0 );\n \
        }\n \
      }\n \
    ";

   // lookup the vector
   const char *getVecSrc = " \
    uniform sampler2D texVectors;\n \
    vec2 getVector( vec2 vectc )\n \
      {\n \
      return texture2D( texVectors, vectc ).xy;\n \
      }\n \
    ";

  if ( normalize )
    {
    return getNormVecSrc;
    }
 return getVecSrc;
}

// Description
// find min/max of unmasked fragments across all regions
// download the entire screen then search each region
void FindMinMax(
      vtkTextureObject *tex,
      const deque<vtkPixelExtent> &extents,
      float &min,
      float &max)
{
  // download entire screen
  int size0 = tex->GetWidth();
  vtkPixelBufferObject *colors = tex->Download();
  float *pColors = static_cast<float*>(colors->MapPackedBuffer());
  // search each region
  size_t nExtents = extents.size();
  for (size_t q=0; q<nExtents; ++q)
    {
    const vtkPixelExtent &extent = extents[q];
    for (int j=extent[2]; j<=extent[3]; ++j)
      {
      for (int i=extent[0]; i<=extent[1]; ++i)
        {
        int id = 4*(j*size0+i);
        bool masked = pColors[id+1] != 0.0f;
        bool ceskip = pColors[id+2] != 0.0f;
        if ( !masked && !ceskip )
          {
          float color = pColors[id];
          min = min > color ? color : min;
          max = max < color ? color : max;
          }
        }
      }
    }
  colors->UnmapPackedBuffer();
  colors->Delete();
  #if  vtkLineIntegralConvolution2DDEBUG>=1
  cerr << "min=" << min << " max=" << max << endl;
  #endif
}

// Description
// find min/max of unmasked fragments across all regions
// download each search each region individually
void StreamingFindMinMax(
      vtkFrameBufferObject2 *fbo,
      vtkTextureObject *tex,
      const deque<vtkPixelExtent> &extents,
      float &min,
      float &max)
{
  size_t nExtents = extents.size();
  // initiate download of each region
  fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, tex);
  fbo->AddColorAttachment(vtkgl::READ_FRAMEBUFFER_EXT, 0U, tex);
  fbo->ActivateDrawBuffer(0U);
  fbo->ActivateReadBuffer(0U);
  fbo->CheckFrameBufferStatus(vtkgl::FRAMEBUFFER_EXT);
  vector<vtkPixelBufferObject*> pbos(nExtents, NULL);
  for (size_t q=0; q<nExtents; ++q)
    {
    pbos[q] = fbo->Download(
          const_cast<int*>(extents[q].GetData()),
          VTK_FLOAT,
          4,
          GL_FLOAT,
          GL_RGBA);
    }
  fbo->DeactivateDrawBuffers();
  fbo->DeactivateReadBuffer();
  fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U);
  fbo->RemoveTexColorAttachment(vtkgl::READ_FRAMEBUFFER_EXT, 0U);
  // search each region
  for (size_t q=0; q<nExtents; ++q)
    {
    vtkPixelBufferObject *&pbo = pbos[q];
    float *pColors = (float*)pbo->MapPackedBuffer();

    size_t n = extents[q].Size();
    for (size_t i = 0; i<n; ++i)
      {
      bool masked = pColors[4*i+1] != 0.0f;
      bool ceskip = pColors[4*i+2] != 0.0f;
      if ( !masked && !ceskip )
        {
        float color = pColors[4*i];
        min = min > color ? color : min;
        max = max < color ? color : max;
        }
      }

    pbo->UnmapPackedBuffer();
    pbo->Delete();
    pbo = NULL;
    }
  pbos.clear();
  #if  vtkLineIntegralConvolution2DDEBUG >= 1
  cerr << "min=" << min << " max=" << max << endl;
  #endif
}

};
using namespace vtkLineIntegralConvolution2DUtil;

// ----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkLineIntegralConvolution2D);

// ----------------------------------------------------------------------------
vtkLineIntegralConvolution2D::vtkLineIntegralConvolution2D()
{
  this->Comm = NULL;

  this->Context = NULL;
  this->FBO = vtkFrameBufferObject2::New();

  this->ShadersNeedBuild = 1;
  this->VTShader = NULL;
  this->LIC0Shader = NULL;
  this->LICIShader = NULL;
  this->LICNShader = NULL;
  this->EEShader = NULL;
  this->CEShader = NULL;
  this->AAHShader = NULL;
  this->AAVShader = NULL;

  this->StepSize = 0.01;
  this->NumberOfSteps = 1;
  this->NormalizeVectors = 1;
  this->ComponentIds[0] = 0;
  this->ComponentIds[1] = 1;

  this->EnhancedLIC = 1;

  this->EnhanceContrast = 0;
  this->LowContrastEnhancementFactor = 0.0;
  this->HighContrastEnhancementFactor = 0.0;
  this->AntiAlias = 0;
  this->MaskThreshold = 0.0;

  this->TransformVectors = 1;
}

// ----------------------------------------------------------------------------
vtkLineIntegralConvolution2D::~vtkLineIntegralConvolution2D()
{
  if (this->Comm)
    {
    delete this->Comm;
    this->Comm = NULL;
    }
  this->SetContext(NULL);
  this->SetVTShader(NULL);
  this->SetLIC0Shader(NULL);
  this->SetLICIShader(NULL);
  this->SetLICNShader(NULL);
  this->SetEEShader(NULL);
  this->SetCEShader(NULL);
  this->SetAAHShader(NULL);
  this->SetAAVShader(NULL);
  this->FBO->Delete();
}

// ----------------------------------------------------------------------------
vtkPainterCommunicator *vtkLineIntegralConvolution2D::GetCommunicator()
{
  if (this->Comm == NULL)
    {
    this->Comm = new vtkPainterCommunicator;
    }
  return this->Comm;
}

// ----------------------------------------------------------------------------
vtkRenderWindow *vtkLineIntegralConvolution2D::GetContext()
{
  return this->Context;
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetContext(vtkRenderWindow *renWin)
{
  if (this->Context == renWin)
    {
    return;
    }

  this->Context = renWin;
  this->ShadersNeedBuild = 1;
  this->FBO->SetContext(renWin);
  this->Modified();

  if (renWin && !this->IsSupported(renWin))
    {
    vtkErrorMacro("The required OpenGL extensions are not present");
    }
}

// ----------------------------------------------------------------------------
bool vtkLineIntegralConvolution2D::IsSupported(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  if (!context)
    {
    return false;
    }

#if defined(__APPLE__) || defined(_WIN32)
  vtkOpenGLExtensionManager *manager = context->GetExtensionManager();
#endif
#if defined(__APPLE__)
  if (manager->DriverIsNvidia() && manager->DriverVersionIs(1,6))
    {
    // Mac OSX 10.6 GLSL doesn't support array initializer
    return false;
    }
#endif
#if defined(_WIN32)
  if ( manager->DriverIsIntel() && manager->DriverGLRendererHas("HD Graphics")
    && !manager->GetIgnoreDriverBugs("Intel HD 2k,3k,4k incorrect results") )
    {
    // Intel drivers produce close but not pixel for pixel identical
    // results. Windows: yes. Linux: untested. Mac: no.
    return false;
    }
#endif

  return vtkTextureObject::IsSupported(renWin, true, false, false)
     && vtkFrameBufferObject2::IsSupported(renWin)
     && vtkShaderProgram2::IsSupported(renWin)
     && vtkPixelBufferObject::IsSupported(renWin);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetNoiseTexParameters(vtkTextureObject * tex)
{
  tex->SetBaseLevel(0);
  tex->SetMaxLevel(0);
  tex->SetWrapS(vtkTextureObject::Repeat);
  tex->SetWrapT(vtkTextureObject::Repeat);
  tex->SetMinificationFilter(vtkTextureObject::Nearest);
  tex->SetMagnificationFilter(vtkTextureObject::Nearest);
  // note : as a side affect it sets the parameters
  // that is needed here.
  tex->Bind();
  tex->UnBind();
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetVectorTexParameters(vtkTextureObject *tex)
{
  tex->SetBaseLevel(0);
  tex->SetMaxLevel(0);
  tex->SetWrapS(vtkTextureObject::ClampToBorder);
  tex->SetWrapT(vtkTextureObject::ClampToBorder);
  tex->SetBorderColor(0.0, 0.0, 0.0, 0.0);
  tex->SetMinificationFilter(vtkTextureObject::Linear);
  tex->SetMagnificationFilter(vtkTextureObject::Linear);
  // note : as a side affect it sets the parameters
  // that is needed here.
  tex->Bind();
  tex->UnBind();
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetComponentIds(int c0, int c1)
{
  if ((this->ComponentIds[0] == c0) && (this->ComponentIds[1] == c1))
    {
    return;
    }
  this->ComponentIds[0] = c0;
  this->ComponentIds[1] = c1;
  this->ShadersNeedBuild = 1;
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetTransformVectors(int val)
{
  val = val < 0 ? 0 : val;
  val = val > 1 ? 1 : val;
  if (this->TransformVectors == val)
    {
    return;
    }
  this->TransformVectors = val;
  this->ShadersNeedBuild = 1;
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetNormalizeVectors(int val)
{
  val = val < 0 ? 0 : val;
  val = val > 1 ? 1 : val;
  if (this->NormalizeVectors == val)
    {
    return;
    }
  this->NormalizeVectors = val;
  this->ShadersNeedBuild = 1;
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetVTShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(VTShader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLIC0Shader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(LIC0Shader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLICIShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(LICIShader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLICNShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(LICNShader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetEEShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(EEShader, vtkShaderProgram2, prog);
}
// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetCEShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(CEShader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetAAHShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(AAHShader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetAAVShader(vtkShaderProgram2 * prog)
{
  vtkSetObjectBodyMacro(AAVShader, vtkShaderProgram2, prog);
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::BuildShaders()
{
  // normalized image space transform shader
  vtkShaderProgram2 *prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  string selectCompsSrc
   = GetComponentSelectionProgram(this->ComponentIds);

  vtkShader2 *selectComps = vtkShader2::New();
  selectComps->SetContext(this->Context);
  selectComps->SetType(VTK_SHADER_TYPE_FRAGMENT);
  selectComps->SetSourceCode(selectCompsSrc.c_str());
  prog->GetShaders()->AddItem(selectComps);
  selectComps->Delete();

  vtkShader2 *glslVT = vtkShader2::New();
  glslVT->SetContext(this->Context);
  glslVT->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslVT->SetSourceCode(vtkLineIntegralConvolution2D_VT);
  prog->GetShaders()->AddItem(glslVT);
  glslVT->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the VT fragment shader");
    }
  else
    {
    this->SetVTShader(prog);
    }
  prog->Delete();

  // LIC0 shader
  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *glslLIC0 = vtkShader2::New();
  glslLIC0->SetContext(this->Context);
  glslLIC0->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslLIC0->SetSourceCode(vtkLineIntegralConvolution2D_LIC0);
  prog->GetShaders()->AddItem(glslLIC0);
  glslLIC0->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the LIC0 fragment shader");
    }
  else
    {
    this->SetLIC0Shader(prog);
    }
  prog->Delete();

  // LICI shader
  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *getVectors = vtkShader2::New();
  getVectors->SetContext(this->Context);
  getVectors->SetType(VTK_SHADER_TYPE_FRAGMENT);
  getVectors->SetSourceCode(GetVectorLookupProgram(this->NormalizeVectors));
  prog->GetShaders()->AddItem(getVectors);
  getVectors->Delete();

  vtkShader2 *glslLICI = vtkShader2::New();
  glslLICI->SetContext(this->Context);
  glslLICI->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslLICI->SetSourceCode(vtkLineIntegralConvolution2D_LICI);
  prog->GetShaders()->AddItem(glslLICI);
  glslLICI->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the LICI fragment shader");
    }
  else
    {
    this->SetLICIShader(prog);
    }
  prog->Delete();

  // LICN shader
  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *glslLICN = vtkShader2::New();
  glslLICN->SetContext(this->Context);
  glslLICN->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslLICN->SetSourceCode(vtkLineIntegralConvolution2D_LICN);
  prog->GetShaders()->AddItem(glslLICN);
  glslLICN->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the LICN fragment shader");
    }
  else
    {
    this->SetLICNShader(prog);
    }
  prog->Delete();

  // Edge Enhancement(EE) shader
  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *glslEE = vtkShader2::New();
  glslEE->SetContext(this->Context);
  glslEE->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslEE->SetSourceCode(vtkLineIntegralConvolution2D_EE);
  prog->GetShaders()->AddItem(glslEE);
  glslEE->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the EE fragment shader");
    }
  else
    {
    this->SetEEShader(prog);
    }
  prog->Delete();

  // Contrast Enhancement(CE) shader
  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *glslCE = vtkShader2::New();
  glslCE->SetContext(this->Context);
  glslCE->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslCE->SetSourceCode(vtkLineIntegralConvolution2D_CE);
  prog->GetShaders()->AddItem(glslCE);
  glslCE->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the CE fragment shader");
    }
  else
    {
    this->SetCEShader(prog);
    }
  prog->Delete();

  // Anti-Alias(AA) shader
  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *glslAAH = vtkShader2::New();
  glslAAH->SetContext(this->Context);
  glslAAH->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslAAH->SetSourceCode(vtkLineIntegralConvolution2D_AAH);
  prog->GetShaders()->AddItem(glslAAH);
  glslAAH->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the AAH fragment shader");
    }
  else
    {
    this->SetAAHShader(prog);
    }
  prog->Delete();

  prog = vtkShaderProgram2::New();
  prog->SetContext(this->Context);

  vtkShader2 *glslAAV = vtkShader2::New();
  glslAAV->SetContext(this->Context);
  glslAAV->SetType(VTK_SHADER_TYPE_FRAGMENT);
  glslAAV->SetSourceCode(vtkLineIntegralConvolution2D_AAV);
  prog->GetShaders()->AddItem(glslAAV);
  glslAAV->Delete();

  prog->Build();
  if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("falied to build the AAV fragment shader");
    }
  else
    {
    this->SetAAVShader(prog);
    }
  prog->Delete();
}

// ----------------------------------------------------------------------------
vtkTextureObject *vtkLineIntegralConvolution2D::Execute(
        vtkTextureObject *vectorTex,
        vtkTextureObject *noiseTex)
{
  // execute over the entire vector field, no guard pixels are present
  // parallel results will be incorrect.

  vtkPixelExtent vectorTexExtent(
        vectorTex->GetWidth(),
        vectorTex->GetHeight());

  return this->Execute(
        vectorTexExtent.GetData(),
        vectorTex,
        noiseTex);
}

// ----------------------------------------------------------------------------
vtkTextureObject *vtkLineIntegralConvolution2D::Execute(
      const int ext[4],
      vtkTextureObject *vectorTex,
      vtkTextureObject *noiseTex)
{
  // execute over a subset of the input texture, no guard pixels are present
  // composite data and parallel results will be incorrect.

  this->SetVectorTexParameters(vectorTex);
  this->SetNoiseTexParameters(noiseTex);

  vtkPixelExtent vectorTexExtent(vectorTex->GetWidth(), vectorTex->GetHeight());
  vtkPixelExtent vectorExtent(ext);
  vtkPixelExtent licExtent(ext);
  vtkPixelExtent outputTexExtent(ext);
  vtkPixelExtent outputExtent(ext);

  deque<vtkPixelExtent> vectorExtents;
  vectorExtents.push_back(vectorExtent);

  deque<vtkPixelExtent> licExtents;
  licExtents.push_back(licExtent);

  unsigned int licSize[2];
  licExtent.Size(licSize);

  return this->Execute(
        vectorTexExtent,
        vectorExtents,
        licExtents,
        vectorTex,
        NULL,
        noiseTex);
}

// ----------------------------------------------------------------------------
vtkTextureObject *vtkLineIntegralConvolution2D::Execute(
      const vtkPixelExtent &inputTexExtent,              // screen space extent of the input texture
      const deque<vtkPixelExtent> &vectorExtents,        // disjoint set describing vector extents
      const deque<vtkPixelExtent> &licExtents,           // disjoint set describing desired lic extents
      vtkTextureObject *vectorTex,
      vtkTextureObject *maskVectorTex,
      vtkTextureObject *noiseTex)
{
  // validate inputs, internal state, etc...
  if (!this->Context)
    {
    vtkErrorMacro("invalid this->Context");
    return NULL;
    }
  if (this->NumberOfSteps < 0)
    {
    vtkErrorMacro("Number of integration steps should be positive.");
    return NULL;
    }
  if (this->StepSize < 0.0)
    {
    vtkErrorMacro("Streamline integration step size should be positive.");
    return NULL;
    }
  if (vectorTex->GetComponents() < 2)
    {
    vtkErrorMacro("VectorField must have at least 2 components.");
    return NULL;
    }

  #if defined(vtkLineIntegralConvolution2DTIME) && !defined(vtkSurfaceLICPainterTIME)
  this->StartTimerEvent("vtkLineIntegralConvolution::Execute");
  #elif defined(USE_VTK_TIMER)
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  #endif

  // initialize shaders
  if (this->ShadersNeedBuild)
    {
    this->BuildShaders();
    this->ShadersNeedBuild = 0;
    }

  // input
  unsigned int inputTexSize[2];
  inputTexExtent.Size(inputTexSize);

  float dx = 1.0f/((float)inputTexSize[0]);
  float dy = 1.0f/((float)inputTexSize[1]);

  // texture coordinates and bounds for compute regions
  unsigned int computeTexSize[2];
  inputTexExtent.Size(computeTexSize);

  // at slight expense to the serial case
  // compute LIC,EE, and AA over the entire vector
  // extents (this is why it's critical that they
  // are disjoint)this allows us to forgo expensive
  // halo exchanges when running in parallel.
  size_t nComputeExtents1 = vectorExtents.size();
  const deque<vtkPixelExtent> &computeExtents1 = vectorExtents;

  size_t nbds = 4*nComputeExtents1;
  vector<float> computeBounds1(nbds, 0.0f);
  for (size_t i=0; i<nComputeExtents1; ++i)
    {
    const vtkPixelExtent &computeExtent1 = computeExtents1[i];
    float *bound = &computeBounds1[4*i];
    bound[0] = ((float)computeExtent1[0])/((float)inputTexSize[0]);
    bound[1] = ((float)computeExtent1[1]+1.0f)/((float)inputTexSize[0]);
    bound[2] = ((float)computeExtent1[2])/((float)inputTexSize[1]);
    bound[3] = ((float)computeExtent1[3]+1.0f)/((float)inputTexSize[1]);
    }

  // for CE only compute on valid extents
  // because there is bleeding at the extent
  // edges that will result in correct scaling
  // if it's used.
  size_t nComputeExtents2 = licExtents.size();
  const deque<vtkPixelExtent> &computeExtents2 = licExtents;

  nbds = 4*nComputeExtents2;
  vector<float> computeBounds2(nbds, 0.0f);

  for (size_t i=0; i<nComputeExtents2; ++i)
    {
    const vtkPixelExtent &computeExtent2 = computeExtents2[i];
    float *bound = &computeBounds2[4*i];
    bound[0] = ((float)computeExtent2[0])/((float)inputTexSize[0]);
    bound[1] = ((float)computeExtent2[1]+1.0f)/((float)inputTexSize[0]);
    bound[2] = ((float)computeExtent2[2])/((float)inputTexSize[1]);
    bound[3] = ((float)computeExtent2[3]+1.0f)/((float)inputTexSize[1]);
    }

  // during integration texture coordinates for
  // noise lookup is computed using the vector
  // texture coordinate this ensures that on any
  // rank we get the same noise value
  unsigned int noiseTexSize[2] = {
        noiseTex->GetWidth(),
        noiseTex->GetHeight()
        };

  vtkPixelExtent noiseExtent(noiseTexSize[0], noiseTexSize[1]);

  float noiseBoundsPt1[2];
  noiseBoundsPt1[0] = ((float)noiseTexSize[0]+1.0f)/((float)inputTexSize[0]);
  noiseBoundsPt1[1] = ((float)noiseTexSize[1]+1.0f)/((float)inputTexSize[1]);

  // bind our fbo
  this->FBO->SaveCurrentBindings();
  this->FBO->Bind(vtkgl::FRAMEBUFFER_EXT);
  this->FBO->InitializeViewport(computeTexSize[0], computeTexSize[1]);

  // initialize the buffer mananger. Textures are assigned
  // and bound to individual units. These textures and units
  // are active and bound for the remainder of this execution.
  vtkLICPingPongBufferManager bufs(
        this->FBO,
        computeTexSize,
        vectorTex,
        maskVectorTex,
        noiseTex,
        this->EnhancedLIC,
        this->TransformVectors);

  #if  vtkLineIntegralConvolution2DDEBUG >= 1
  int rank = this->GetCommunicator()->GetRank();
  #endif
  #if vtkLineIntegralConvolution2DDEBUG >= 3
  bufs.WriteInputs(rank, vectorExtents);
  #endif

  if (this->TransformVectors)
    {
    // ------------------------------------------- begin normalized image space transform
    #if defined(vtkLineIntegralConvolution2DTIME)
    this->StartTimerEvent("vtkLineIntegralConvolution::TransformVectors");
    #endif

    this->VTShader->UseProgram();
    this->VTShader->SetUniformi("texVectors", bufs.GetVectorTextureUnit());
    this->VTShader->SetUniform2ft("uTexSize", inputTexSize);

    bufs.AttachImageVectorBuffer();
    // essential to initialize the entire buffer
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    size_t nVectorExtents = vectorExtents.size();
    for (size_t q=0; q<nVectorExtents; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachImageVectorBuffer();

    this->VTShader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteImageVectorBuffer(rank, vectorExtents);
    #endif

    #if defined(vtkLineIntegralConvolution2DTIME)
    this->EndTimerEvent("vtkLineIntegralConvolution::TransformVectors");
    #endif
    // ------------------------------------------- end normalized image space transform
    }

  // --------------------------------------------- begin first-pass LIC
  #if defined(vtkLineIntegralConvolution2DTIME)
  this->StartTimerEvent("vtkLineIntegralConvolution::Integrate1");
  #endif

  //
  // initialize convolution and seeds
  //
  bufs.ClearBuffers(this->FBO, inputTexExtent, vectorExtents, this->EnhancedLIC);
  bufs.AttachVectorTextures();
  bufs.AttachNoiseTexture(0);

  this->LIC0Shader->UseProgram();
  this->LIC0Shader->SetUniformi("uStepNo", 0);
  this->LIC0Shader->SetUniformi("uPassNo", 0);
  this->LIC0Shader->SetUniformf("uMaskThreshold", this->MaskThreshold);
  this->LIC0Shader->SetUniform2f("uNoiseBoundsPt1", noiseBoundsPt1);
  this->LIC0Shader->SetUniformi("texMaskVectors", bufs.GetMaskVectorTextureUnit());
  this->LIC0Shader->SetUniformi("texNoise", bufs.GetNoiseTextureUnit());
  this->LIC0Shader->SetUniformi("texLIC", bufs.GetLICTextureUnit());

  bufs.AttachLICBuffers();
  for (size_t q=0; q<nComputeExtents1; ++q)
    {
    bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
    }
  bufs.DettachLICBuffers();
  bufs.Swap();

  this->LIC0Shader->UnuseProgram();

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_lic0b_a.vtm","lic2d_lic0b_s.vtm",computeExtents1);
  #endif

  //
  // backward LIC
  //
  this->LICIShader->UseProgram();
  this->LICIShader->SetUniformi("uPassNo", 0);
  this->LICIShader->SetUniformf("uStepSize", -this->StepSize);
  this->LICIShader->SetUniform2f("uNoiseBoundsPt1", noiseBoundsPt1);
  this->LICIShader->SetUniformi("texVectors", bufs.GetVectorTextureUnit());
  this->LICIShader->SetUniformi("texNoise", bufs.GetNoiseTextureUnit());
  this->LICIShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());
  this->LICIShader->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());

  int stepNum = 0;
  for (int stepIdx=0; stepIdx<this->NumberOfSteps; ++stepIdx, ++stepNum)
    {
    bufs.AttachLICBuffers();
    for (size_t q=0; q<nComputeExtents1; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachLICBuffers();
    bufs.Swap();
    }
  this->LICIShader->UnuseProgram();

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_licib_a.vtm", "lic2d_licib_s.vtm", computeExtents1);
  #endif

  //
  // initialize seeds
  //
  this->LIC0Shader->UseProgram();
  this->LIC0Shader->SetUniformi("uStepNo", 1);

  bufs.AttachLICBuffers();
  for (size_t q=0; q<nComputeExtents1; ++q)
    {
    bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
    }
  bufs.DettachLICBuffers();
  bufs.Swap();

  this->LIC0Shader->UnuseProgram();

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_lic0f_a.vtm", "lic2d_lic0f_s.vtm", computeExtents1);
  #endif

  //
  // forward LIC
  //
  this->LICIShader->UseProgram();
  this->LICIShader->SetUniformf("uStepSize", this->StepSize);

  for (int stepIdx=0; stepIdx<this->NumberOfSteps; ++stepIdx, ++stepNum)
    {
    bufs.AttachLICBuffers();
    for (size_t q=0; q<nComputeExtents1; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachLICBuffers();
    bufs.Swap();
    }

  this->LICIShader->UnuseProgram();

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_licif_a.vtm", "lic2d_licif_s.vtm", computeExtents1);
  #endif

  //
  // finalize LIC
  //
  this->LICNShader->UseProgram();
  this->LICNShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());

  bufs.AttachLICBuffers();
  for (size_t q=0; q<nComputeExtents1; ++q)
    {
    bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
    }
  bufs.DettachBuffers();
  bufs.Swap();

  this->LICNShader->UnuseProgram();

  #if (vtkLineIntegralConvolution2DDEBUG >= 1)
  bufs.WriteBuffers(rank,"lic2d_licn_a.vtm", "lic2d_licn_s.vtm", computeExtents1);
  #endif

  #if defined(vtkLineIntegralConvolution2DTIME)
  this->EndTimerEvent("vtkLineIntegralConvolution::Integrate1");
  #endif

  // ----------------------------------------------- end first-pass LIC
  if (this->EnhancedLIC)
    {
    if (this->EnhanceContrast == ENHANCE_CONTRAST_ON)
      {
      // ------------------------------------------- begin contrast enhance
      #if defined(vtkLineIntegralConvolution2DTIME) || defined(vtkSurfaceLICPainterTIME)
      this->StartTimerEvent("vtkLineIntegralConvolution::ContrastEnhance1");
      #endif

      vtkPainterCommunicator *comm = this->GetCommunicator();

      // find the min and max only on the valid extents
      // because there will be bleeding at the edges.
      float grayMin = VTK_FLOAT_MAX;
      float grayMax = -VTK_FLOAT_MAX;
      float grayMaxMinDiff = VTK_FLOAT_MAX;
      vtkTextureObject *licTex = bufs.GetLastLICBuffer();
      #ifdef STREAMING_MIN_MAX
      StreamingFindMinMax(this->FBO, licTex, computeExtents2, grayMin, grayMax);
      #else
      FindMinMax(licTex, computeExtents2, grayMin, grayMax);
      #endif

      if ( computeExtents2.size()
        && ((grayMax <= grayMin) || (grayMax > 1.0f) || (grayMin < 0.0f)) )
        {
        vtkErrorMacro(
          << comm->GetRank()
          << " : Invalid color range " << grayMin << ", " << grayMax
          << ". Normlaization pass skipped");
        grayMin = 0.0;
        grayMax = 1.0;
        }

      // in parallel use a reduction to obtain the image
      // wide min/max
      this->GetGlobalMinMax(comm, grayMin, grayMax);

      // its critical to update on the entire extent to
      //ensure correct values in the guard pixles because
      // we don't do a halo exchange
      grayMaxMinDiff = grayMax-grayMin;

      this->CEShader->UseProgram();
      this->CEShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());
      this->CEShader->SetUniformf("uMin", grayMin );
      this->CEShader->SetUniformf("uMaxMinDiff", grayMaxMinDiff);

      bufs.AttachLICBuffers();
      for (size_t q=0; q<nComputeExtents1; ++q)
        {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
        }
      bufs.DettachLICBuffers();
      bufs.Swap();

      this->CEShader->UnuseProgram();

      #if (vtkLineIntegralConvolution2DDEBUG >= 1)
      bufs.WriteBuffers(rank,"lic2d_1ce.vtm", NULL, computeExtents1);
      #endif

      #if defined(vtkLineIntegralConvolution2DTIME) || defined(vtkSurfaceLICPainterTIME)
      this->EndTimerEvent("vtkLineIntegralConvolution::ContrastEnhance1");
      #endif
      // --------------------------------------------- end contrast enhance
      }

    // --------------------------------------------- begin high-pass filtering
    #if defined(vtkLineIntegralConvolution2DTIME)
    this->StartTimerEvent("vtkLineIntegralConvolution::EdgeEnahnce");
    #endif

    #ifdef INTEL_BUG
    bufs.AttachEEBuffer();
    #endif

    this->EEShader->UseProgram();
    this->EEShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->EEShader->SetUniformf("uDx", dx);
    this->EEShader->SetUniformf("uDy", dy);

    #ifndef INTEL_BUG
    bufs.AttachEEBuffer();
    #endif
    for (size_t q=0; q<nComputeExtents1; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachEEBuffer();

    this->EEShader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 1)
    bufs.WriteEEBuffer(rank, computeExtents1);
    #endif

    #if defined(vtkLineIntegralConvolution2DTIME)
    this->EndTimerEvent("vtkLineIntegralConvolution::EdgeEnahnce");
    #endif
    // --------------------------------------------- end high-pass filtering

    // --------------------------------------------- begin second-pass LIC
    #if defined(vtkLineIntegralConvolution2DTIME)
    this->StartTimerEvent("vtkLineIntegralConvolution::Integrate2");
    #endif

    // in pass 2 lic is comuted by convolving edge-enhanced result of pass 1
    // rather than noise. This gives the result a nice smooth look, since the
    // input is fairly smooth fewer steps are needed.

    // clear the buffers
    bufs.DettachBuffers();
    bufs.ClearBuffers(this->FBO, inputTexExtent, vectorExtents, /*clearEE=*/0);
    bufs.AttachVectorTextures();
    bufs.AttachNoiseTexture(1);

    //
    // initialize convolution and seeds
    //
    this->LIC0Shader->UseProgram();
    this->LIC0Shader->SetUniformi("uStepNo", 0);
    this->LIC0Shader->SetUniformi("uPassNo", 1);

    bufs.AttachLICBuffers();
    for (size_t q=0; q<nComputeExtents1; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachLICBuffers();
    bufs.Swap();

    this->LIC0Shader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteBuffers(rank,"lic2d_elic0b_a.vtm", "lic2d_elic0b_s.vtm", computeExtents1);
    #endif

    //
    // backward LIC
    //
    this->LICIShader->UseProgram();
    this->LICIShader->SetUniformi("uPassNo", 1);
    this->LICIShader->SetUniformf("uStepSize", -this->StepSize);

    int nSteps = this->NumberOfSteps/2;
    stepNum = 0;
    for (int stepIdx=0; stepIdx<nSteps; ++stepIdx, ++stepNum)
      {
      bufs.AttachLICBuffers();
      for (size_t q=0; q<nComputeExtents1; ++q)
        {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
        }
      bufs.DettachLICBuffers();
      bufs.Swap();
      }

    this->LICIShader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >=2 )
    bufs.WriteBuffers(rank,"lic2d_elicib_a.vtm", "lic2d_elicib_s.vtm",computeExtents1);
    #endif

    //
    // initialize seeds
    //
    this->LIC0Shader->UseProgram();
    this->LIC0Shader->SetUniformi("uStepNo", 1);

    bufs.AttachLICBuffers();
    for (size_t q=0; q<nComputeExtents1; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachLICBuffers();
    bufs.Swap();

    this->LIC0Shader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteBuffers(rank,"lic2d_elic0f_a.vtm", "lic2d_elic0f_s.vtm",computeExtents1);
    #endif

    //
    // forward LIC
    //
    this->LICIShader->UseProgram();
    this->LICIShader->SetUniformf("uStepSize", this->StepSize);

    for (int stepIdx=0; stepIdx<nSteps; ++stepIdx, ++stepNum)
      {
      bufs.AttachLICBuffers();
      for (size_t q=0; q<nComputeExtents1; ++q)
        {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
        }
      bufs.DettachLICBuffers();
      bufs.Swap();
      }

    this->LICIShader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteBuffers(rank,"lic2d_elicif_a.vtm", "lic2d_elicif_s.vtm",computeExtents1);
    #endif

    //
    // finalize LIC
    //
    this->LICNShader->UseProgram();
    this->LICNShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());

    bufs.AttachLICBuffers();
    for (size_t q=0; q<nComputeExtents1; ++q)
      {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
      }
    bufs.DettachLICBuffers();
    bufs.Swap();

    this->LICNShader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 1)
    bufs.WriteBuffers(rank,"lic2d_elicn_a.vtm", "lic2d_elicn_s.vtm",computeExtents1);
    #endif

    #if defined(vtkLineIntegralConvolution2DTIME)
    this->EndTimerEvent("vtkLineIntegralConvolution::Integrate2");
    #endif
    // --------------------------------------------- end second-pass LIC
    }

  if (this->AntiAlias)
    {
    // --------------------------------------------- begin anti-alias

    #if defined(vtkLineIntegralConvolution2DTIME)
    this->StartTimerEvent("vtkLineIntegralConvolution::AntiAlias");
    #endif

    this->AAHShader->UseProgram();
    this->AAHShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->AAHShader->SetUniformf("uDx", dx);
    this->AAHShader->UnuseProgram();

    this->AAVShader->UseProgram();
    this->AAVShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->AAVShader->SetUniformf("uDy", dy);
    this->AAVShader->UnuseProgram();

    // it's ok to compute over just the valid extent
    // we don't care here if guard pixels are smoothed
    // however computing over the entire extent avoids
    // bleeding at the edges when multiple passes are
    // requested.
    for (int i=0; i<this->AntiAlias; ++i)
      {
      // horizontal pass
      this->AAHShader->UseProgram();
      bufs.AttachLICBuffers();
      for (size_t q=0; q<nComputeExtents1; ++q)
        {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
        }
      bufs.DettachLICBuffers();
      bufs.Swap();
      this->AAHShader->UnuseProgram();

      // vertical pass
      this->AAVShader->UseProgram();
      bufs.AttachLICBuffers();
      for (size_t q=0; q<nComputeExtents1; ++q)
        {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q]);
        }
      bufs.DettachLICBuffers();
      bufs.Swap();
      this->AAVShader->UnuseProgram();
      }

    #if (vtkLineIntegralConvolution2DDEBUG >= 1)
    bufs.WriteBuffers(rank,"lic2d_aa.vtm", NULL, computeExtents1);
    #endif

    #if defined(vtkLineIntegralConvolution2DTIME)
    this->EndTimerEvent("vtkLineIntegralConvolution::AntiAlias");
    #endif
    // --------------------------------------------- end anti-alias
    }

  if (this->EnhanceContrast)
    {
    // ------------------------------------------- begin contrast enhance
    #if defined(vtkLineIntegralConvolution2DTIME) || defined(vtkSurfaceLICPainterTIME)
    this->StartTimerEvent("vtkLineIntegralConvolution::ContrastEnhance2");
    #endif

    vtkPainterCommunicator *comm = this->GetCommunicator();

    // the final contrast enhancement should
    // be applied only to the valid extents
    float grayMin = VTK_FLOAT_MAX;
    float grayMax = -VTK_FLOAT_MAX;
    float grayMaxMinDiff = 1.0f;

    vtkTextureObject *licTex = bufs.GetLastLICBuffer();
    #ifdef STREAMING_MIN_MAX
    StreamingFindMinMax(this->FBO, licTex, computeExtents2, grayMin, grayMax);
    #else
    FindMinMax(licTex, computeExtents2, grayMin, grayMax);
    #endif

    if ( computeExtents2.size()
      && ((grayMax <= grayMin) || (grayMax > 1.0f) || (grayMin < 0.0f)) )
      {
      vtkErrorMacro(
        << comm->GetRank()
        << " : Invalid intensity range " << grayMin << ", " << grayMax
        << "for contrast ehancement");
      grayMin = 0.0;
      grayMax = 1.0;
      }

    // in parallel use a reduction to obtain the image
    // wide min/max
    this->GetGlobalMinMax(comm, grayMin, grayMax);

    // select M and m as a fraction of the range.
    grayMaxMinDiff = grayMax-grayMin;
    grayMin += grayMaxMinDiff*this->LowContrastEnhancementFactor;
    grayMax -= grayMaxMinDiff*this->HighContrastEnhancementFactor;
    grayMaxMinDiff = grayMax-grayMin;

    this->CEShader->UseProgram();
    this->CEShader->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->CEShader->SetUniformf("uMin", grayMin );
    this->CEShader->SetUniformf("uMaxMinDiff", grayMaxMinDiff);

    bufs.AttachLICBuffers();
    for (size_t q=0; q<nComputeExtents2; ++q)
      {
      bufs.RenderQuad(&computeBounds2[4*q], computeExtents2[q]);
      }
    bufs.DettachLICBuffers();
    bufs.Swap();

    this->CEShader->UnuseProgram();

    #if (vtkLineIntegralConvolution2DDEBUG >= 1)
    bufs.WriteBuffers(rank,"lic2d_2ce.vtm", NULL, computeExtents2);
    #endif

    #if defined(vtkLineIntegralConvolution2DTIME) || defined(vtkSurfaceLICPainterTIME)
    this->EndTimerEvent("vtkLineIntegralConvolution::ContrastEnhance2");
    #endif

    // --------------------------------------------- end contrast enahnce
    }

  bufs.DettachBuffers();
  this->FBO->UnBind(vtkgl::FRAMEBUFFER_EXT);

  vtkTextureObject *outputTex = bufs.GetLastLICBuffer();
  outputTex->Register(0);

  #if defined(vtkLineIntegralConvolution2DTIME) && !defined(vtkSurfaceLICPainterTIME)
  this->EndTimerEvent("vtkLineIntegralConvolution::Execute");
  #elif defined(USE_VTK_TIMER)
  timer->StopTimer();
  #endif

  return outputTex;
}

//-----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os
    << indent << "Comm=" << this->Comm << endl
    << indent << "Context=" << this->Context << endl
    << indent << "FBO=" << this->FBO << endl
    << indent << "ShadersNeedBuild=" << this->ShadersNeedBuild << endl
    << indent << "VTShader=" << this->VTShader << endl
    << indent << "LIC0Shader=" << this->LIC0Shader << endl
    << indent << "LICIShader=" << this->LICIShader << endl
    << indent << "LICNShader=" << this->LICNShader << endl
    << indent << "EEShader=" << this->EEShader << endl
    << indent << "CEShader=" << this->CEShader << endl
    << indent << "AAHShader=" << this->AAHShader << endl
    << indent << "AAVShader=" << this->AAVShader << endl
    << indent << "NumberOfSteps=" << this->NumberOfSteps << endl
    << indent << "StepSize=" << this->StepSize << endl
    << indent << "EnhancedLIC=" << this->EnhancedLIC << endl
    << indent << "EnhanceContrast=" << this->EnhanceContrast << endl
    << indent << "LowContrastEnhancementFactor=" << this->LowContrastEnhancementFactor << endl
    << indent << "HighContrastEnhancementFactor=" << this->HighContrastEnhancementFactor << endl
    << indent << "AntiAlias=" << this->AntiAlias << endl
    << indent << "MaskThreshold=" << this->MaskThreshold << endl
    << indent << "TransformVectors=" << this->TransformVectors << endl
    << indent << "NormalizeVectors=" << this->NormalizeVectors << endl
    << indent << "ComponentIds=" << this->ComponentIds[0] << ", " << this->ComponentIds[1] << endl;
}
