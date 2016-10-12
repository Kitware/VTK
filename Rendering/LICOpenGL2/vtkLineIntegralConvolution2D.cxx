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

#include "vtk_glew.h"

#include "vtkOpenGLHelper.h"


#include "vtkFloatArray.h"
#include "vtkFrameBufferObject2.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkPainterCommunicator.h"
#include "vtkPixelBufferObject.h"
#include "vtkPixelExtent.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTextureObjectVS.h"
#include "vtkTimerLog.h"

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
#include "vtkLineIntegralConvolution2D_VT.h" // normalized image space transform
#include "vtkLineIntegralConvolution2D_LIC0.h" // initialization for lic
#include "vtkLineIntegralConvolution2D_LICI.h" // compute i'th lic step
#include "vtkLineIntegralConvolution2D_LICN.h" // finalize lic
#include "vtkLineIntegralConvolution2D_EE.h"   // Laplace edge-enhance
#include "vtkLineIntegralConvolution2D_CE.h"   // contrast enhance
#include "vtkLineIntegralConvolution2D_AAH.h"  // horizontal part of anti-alias filter
#include "vtkLineIntegralConvolution2D_AAV.h"  // vertical part of anti-alias filter

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

    this->ReadIndex = 0;

    // allocate buffers
    vtkOpenGLRenderWindow *context =
      vtkOpenGLRenderWindow::SafeDownCast(fbo->GetContext());
    this->LICTexture0 = this->AllocateLICBuffer(context, bufSize);
    this->SeedTexture0 = this->AllocateLICBuffer(context, bufSize);
    this->LICTexture1 = this->AllocateLICBuffer(context, bufSize);
    this->SeedTexture1 = this->AllocateLICBuffer(context, bufSize);
    this->EETexture = doEEPass ? this->AllocateNoiseBuffer(context, bufSize) : NULL;
    this->ImageVectorTexture = doVTPass ? this->AllocateVectorBuffer(context, bufSize) : NULL;

    // setup pairs for buffer ping-pong
    this->PingTextures[0] = this->LICTexture0;
    this->PingTextures[1] = this->SeedTexture0;

    this->PongTextures[0] = this->LICTexture1;
    this->PongTextures[1] = this->SeedTexture1;

    this->Textures[0] = this->PingTextures;
    this->Textures[1] = this->PongTextures;

    this->DettachBuffers(fbo);

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
  int GetVectorTextureUnit()
  {
    return this->VectorTexture->GetTextureUnit();
  }
  int GetImageVectorTextureUnit()
  {
    if (this->ImageVectorTexture)
    {
      this->ImageVectorTexture->Activate();
      return this->ImageVectorTexture->GetTextureUnit();
    }
    this->VectorTexture->Activate();
    return this->VectorTexture->GetTextureUnit();
  }
  int GetMaskVectorTextureUnit()
  {
    if (this->MaskVectorTexture)
    {
      this->MaskVectorTexture->Activate();
      return this->MaskVectorTexture->GetTextureUnit();
    }
    return this->GetImageVectorTextureUnit();
  }
  int GetNoiseTextureUnit(int LICPassNum)
  {
    if (LICPassNum == 0)
    {
      this->NoiseTexture->Activate();
      return this->NoiseTexture->GetTextureUnit();
    }
    this->EETexture->Activate();
    return this->EETexture->GetTextureUnit();
  }
  int GetLICTextureUnit()
  {
    this->Textures[this->ReadIndex][0]->Activate();
    return this->Textures[this->ReadIndex][0]->GetTextureUnit();
  }
  int GetSeedTextureUnit()
  {
    this->Textures[this->ReadIndex][1]->Activate();
    return this->Textures[this->ReadIndex][1]->GetTextureUnit();
  }

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
    fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, this->LICTexture0);
    fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 1U, this->SeedTexture0);
    fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 2U, this->LICTexture1);
    fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 3U, this->SeedTexture1);
    unsigned int num = 4U;
    if (clearEETex)
    {
      fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 4U, this->EETexture);
      num = 5U;
    }
    fbo->ActivateDrawBuffers(num);
    DEBUG3CheckFrameBufferStatusMacro(GL_DRAW_FRAMEBUFFER);

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
    // detach
    fbo->RemoveTexColorAttachments(GL_DRAW_FRAMEBUFFER, num);
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
    fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, tex);
    fbo->ActivateDrawBuffers(1);
    DEBUG3CheckFrameBufferStatusMacro(GL_DRAW_FRAMEBUFFER);

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
    fbo->RemoveTexColorAttachments(GL_DRAW_FRAMEBUFFER, 1);
    fbo->DeactivateDrawBuffers();
  }

  // Description:
  // Activates the input textures. These are read only.
  void AttachVectorTextures()
  {
    // vector
    if (this->ImageVectorTexture)
    {
      this->ImageVectorTexture->Activate();
    }
    else
    {
      this->VectorTexture->Activate();
    }

    // mask vectors (optional)
    if (this->MaskVectorTexture)
    {
      this->MaskVectorTexture->Activate();
    }
  }

  // Description:
  // Deactivates the input noise textures.
  void DettachVectorTextures()
  {
    if (this->ImageVectorTexture)
    {
      this->ImageVectorTexture->Deactivate();
    }
    else
    {
      this->VectorTexture->Deactivate();
    }

    // mask vectors (optional)
    if (this->MaskVectorTexture)
    {
      this->MaskVectorTexture->Deactivate();
    }
  }

  // Description:
  // Activate the read only noise texture. It's active for
  // the entirety of each LIC pass.
  void AttachNoiseTexture(int LICPassNum = 0)
  {
    switch (LICPassNum)
    {
      case 0:
        this->NoiseTexture->Activate();
        break;
      case 1:
        this->EETexture->Activate();
        break;
    }
  }

  // Dsecription:
  // Deactivate the inpyt noise tetxure.
  void DettachNoiseTexture(int LICPassNum = 0)
  {
    switch (LICPassNum)
    {
      case 0:
        this->NoiseTexture->Deactivate();
        break;
      case 1:
        this->EETexture->Deactivate();
        break;
    }
  }

  // Description:
  // Setup read/write from/to the active lic/seed buffer texture pair
  // for LIC pass.
  void AttachLICBuffers(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    // activate read textures
    vtkTextureObject **readTex = this->Textures[this->ReadIndex];
    readTex[0]->Activate();
    readTex[1]->Activate();

    // attach write textures
    vtkTextureObject **writeTex = this->Textures[1-this->ReadIndex];

    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          writeTex[0]->GetHandle(),
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebuffereadTexture2D");

    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT1,
          GL_TEXTURE_2D,
          writeTex[1]->GetHandle(),
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebuffereadTexture2D");

    GLenum atts[2] = {
          GL_COLOR_ATTACHMENT0,
          GL_COLOR_ATTACHMENT1
          };
    glDrawBuffers(2, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
  }

  // Description:
  // Remove input/output bufers used for computing the LIC.
  void DettachLICBuffers(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);

    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT1,
          GL_TEXTURE_2D,
          0U,
          0);

    GLenum atts[1] = {GL_NONE};
    glDrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");

    vtkTextureObject **readTex = this->Textures[this->ReadIndex];
    readTex[0]->Deactivate();
    readTex[1]->Deactivate();
  }

  // Description:
  // Attach read/write buffers for transform pass.
  void AttachImageVectorBuffer(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    this->VectorTexture->Activate();

    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          this->ImageVectorTexture->GetHandle(),
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    GLenum atts[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
  }

  // Description:
  // Attach read/write buffers for transform pass.
  void DettachImageVectorBuffer(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    this->VectorTexture->Deactivate();

    glFramebufferTexture2D(
      GL_DRAW_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D,
      0U,
      0);

    GLenum atts[1] = {GL_NONE};
    glDrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
  }

  // Description:
  // Attach read/write buffers for EE pass.
  void AttachEEBuffer(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    vtkTextureObject **readTex = this->Textures[this->ReadIndex];
    readTex[0]->Activate();

    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          this->EETexture->GetHandle(),
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    GLenum atts[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");

    DEBUG3CheckFrameBufferStatusMacro(GL_DRAW_FRAMEBUFFER);
  }

  // Description:
  // Attach read/write buffers for EE pass.
  void DettachEEBuffer(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    vtkTextureObject **readTex = this->Textures[this->ReadIndex];
    readTex[0]->Deactivate();

    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);

    GLenum atts[1] = {GL_NONE};
    glDrawBuffers(1, atts);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");
  }

  // Description:
  // Deactivates and removes all read/write buffers that were in
  // use during the run, restoring a pristine FBO/texture unit state.
  void DettachBuffers(vtkFrameBufferObject2 *vtkNotUsed(fbo))
  {
    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          0U,
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    glFramebufferTexture2D(
          GL_DRAW_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT1,
          GL_TEXTURE_2D,
          0U,
          0);
    vtkOpenGLStaticCheckErrorMacro("failed at glFramebufferTexture2D");

    GLenum none = GL_NONE;
    glDrawBuffers(1, &none);
    vtkOpenGLStaticCheckErrorMacro("failed at glDrawBuffers");

    // deactivate all textures?
    vtkTextureObject **readTex = this->Textures[this->ReadIndex];
    if (readTex[0]) { readTex[0]->Deactivate(); }
    if (readTex[1]) { readTex[1]->Deactivate(); }
    vtkTextureObject **writeTex = this->Textures[1-this->ReadIndex];
    if (writeTex[0]) { writeTex[0]->Deactivate(); }
    if (writeTex[1]) { writeTex[1]->Deactivate(); }
  }

  // Description:
  // Get the read/write ids
  int GetReadIndex(){ return this->ReadIndex; }
  int GetWriteIndex(){ return 1 - this->ReadIndex; }

  // Description:
  // Allocate a texture of the given size.
  // with parameters for LIC lookups
  vtkTextureObject *AllocateLICBuffer(
        vtkOpenGLRenderWindow *context,
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
        vtkOpenGLRenderWindow *context,
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
        vtkOpenGLRenderWindow *context,
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
        vtkOpenGLRenderWindow *context,
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
  // Render screen aligned quad.
  void RenderQuad(
          float computeBoundsPt0[2],
          float computeBoundsPt1[2],
          vtkPixelExtent computeExtent,
          vtkOpenGLHelper *cbo)
  {
    float computeBounds[4] = {
          computeBoundsPt0[0], computeBoundsPt1[0],
          computeBoundsPt0[1], computeBoundsPt1[1]
          };
     this->RenderQuad(computeBounds, computeExtent, cbo);
  }

  // Description:
  // Render screen aligned quad.
  void RenderQuad(
          float computeBounds[4],
          vtkPixelExtent computeExtent,
          vtkOpenGLHelper *cbo)
  {
    float quadBounds[4];
    computeExtent.CellToNode();
    computeExtent.GetData(quadBounds);

    float tcoords[] = {
      computeBounds[0], computeBounds[2],
      computeBounds[1], computeBounds[2],
      computeBounds[1], computeBounds[3],
      computeBounds[0], computeBounds[3]};

    float verts[] = {
      computeBounds[0]*2.0f-1.0f, computeBounds[2]*2.0f-1.0f, 0.0f,
      computeBounds[1]*2.0f-1.0f, computeBounds[2]*2.0f-1.0f, 0.0f,
      computeBounds[1]*2.0f-1.0f, computeBounds[3]*2.0f-1.0f, 0.0f,
      computeBounds[0]*2.0f-1.0f, computeBounds[3]*2.0f-1.0f, 0.0f};

    vtkOpenGLRenderUtilities::RenderQuad(verts, tcoords,
      cbo->Program, cbo->VAO);
    vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");
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

  int  ReadIndex;
  vtkTextureObject *PingTextures[2];
  vtkTextureObject *PongTextures[2];
  vtkTextureObject **Textures[2];
};

namespace vtkLineIntegralConvolution2DUtil
{
/**
glsl shader code for selecting vector comps.
*/
string GetComponentSelectionProgram(int *compIds)
{
  // swizles at 45,46
  string srcCode(".$$");
  const char *compNames = "xyzw";
  srcCode[1] = compNames[compIds[0]];
  srcCode[2] = compNames[compIds[1]];
  return srcCode;
}

/*
Shader code for looking up vectors
*/
const char *GetVectorLookupProgram(int normalize)
{
  // lookup the vector and normalize
  const char *getNormVecSrc = " \
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
  fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, tex);
  fbo->AddColorAttachment(GL_READ_FRAMEBUFFER, 0U, tex);
  fbo->ActivateDrawBuffer(0U);
  fbo->ActivateReadBuffer(0U);
  fbo->CheckFrameBufferStatus(GL_FRAMEBUFFER);
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
  fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 0U);
  fbo->RemoveTexColorAttachment(GL_READ_FRAMEBUFFER, 0U);
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
  delete this->Comm;
  this->SetContext(NULL);
  this->SetVTShader(NULL);
  this->SetLIC0Shader(NULL);
  this->SetLICIShader(NULL);
  this->SetLICNShader(NULL);
  this->SetEEShader(NULL);
  this->SetCEShader(NULL);
  this->SetAAHShader(NULL);
  this->SetAAVShader(NULL);

  if (this->VTShader)
  {
    delete this->VTShader;
  }
  if (this->LIC0Shader)
  {
    delete this->LIC0Shader;
  }
  if (this->LICIShader)
  {
    delete this->LICIShader;
  }
  if (this->LICNShader)
  {
    delete this->LICNShader;
  }
  if (this->EEShader)
  {
    delete this->EEShader;
  }
  if (this->CEShader)
  {
    delete this->CEShader;
  }
  if (this->AAHShader)
  {
    delete this->AAHShader;
  }
  if (this->AAVShader)
  {
    delete this->AAVShader;
  }

  this->FBO->Delete();
}

// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow *vtkLineIntegralConvolution2D::GetContext()
{
  return this->Context;
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetContext(vtkOpenGLRenderWindow *renWin)
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

  return vtkTextureObject::IsSupported(context, true, false, false)
     && vtkFrameBufferObject2::IsSupported(context)
     && vtkPixelBufferObject::IsSupported(context);
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
void vtkLineIntegralConvolution2D::SetVTShader(vtkShaderProgram * prog)
{
  if (this->VTShader)
  {
    this->VTShader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLIC0Shader(vtkShaderProgram * prog)
{
  if (this->LIC0Shader)
  {
    this->LIC0Shader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLICIShader(vtkShaderProgram * prog)
{
  if (this->LICIShader)
  {
    this->LICIShader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetLICNShader(vtkShaderProgram * prog)
{
  if (this->LICNShader)
  {
    this->LICNShader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetEEShader(vtkShaderProgram * prog)
{
  if (this->EEShader)
  {
    this->EEShader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetCEShader(vtkShaderProgram * prog)
{
  if (this->CEShader)
  {
    this->CEShader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetAAHShader(vtkShaderProgram * prog)
{
  if (this->AAHShader)
  {
    this->AAHShader->Program = prog;
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::SetAAVShader(vtkShaderProgram * prog)
{
  if (this->AAVShader)
  {
    this->AAVShader->Program = prog;
  }
}

namespace {
  void BuildAShader(vtkOpenGLRenderWindow *renWin,
    vtkOpenGLHelper **cbor, const char *frag)
  {
  if (*cbor == NULL)
  {
    *cbor = new vtkOpenGLHelper;
    std::string VSSource = vtkTextureObjectVS;
    std::string GSSource;
    (*cbor)->Program =
        renWin->GetShaderCache()->ReadyShaderProgram(VSSource.c_str(),
                                              frag,
                                              GSSource.c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram((*cbor)->Program);
  }
  }
}

// ----------------------------------------------------------------------------
void vtkLineIntegralConvolution2D::BuildShaders()
{
  vtkOpenGLRenderWindow *renWin = this->Context;

  std::string FSSource = vtkLineIntegralConvolution2D_VT;
  vtkShaderProgram::Substitute(FSSource,"//VTK::LICComponentSelection::Impl",
    "vec2 V = texture2D(texVectors, tcoordVC.st)" +
    GetComponentSelectionProgram(this->ComponentIds) + ";"
    );
  BuildAShader(renWin, &this->VTShader,
    FSSource.c_str());

  BuildAShader(renWin, &this->LIC0Shader,
    vtkLineIntegralConvolution2D_LIC0);

  FSSource = vtkLineIntegralConvolution2D_LICI;
  vtkShaderProgram::Substitute(FSSource,"//VTK::LICVectorLookup::Impl",
    GetVectorLookupProgram(this->NormalizeVectors)
    );
  BuildAShader(renWin, &this->LICIShader,
    FSSource.c_str());

  BuildAShader(renWin, &this->LICNShader,
    vtkLineIntegralConvolution2D_LICN);
  BuildAShader(renWin, &this->EEShader,
    vtkLineIntegralConvolution2D_EE);
  BuildAShader(renWin, &this->CEShader,
    vtkLineIntegralConvolution2D_CE);
  BuildAShader(renWin, &this->AAHShader,
    vtkLineIntegralConvolution2D_AAH);
  BuildAShader(renWin, &this->AAVShader,
    vtkLineIntegralConvolution2D_AAV);
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

  vtkOpenGLRenderWindow *renWin = this->Context;
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
  this->FBO->Bind(GL_FRAMEBUFFER);
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

    renWin->GetShaderCache()->ReadyShaderProgram(this->VTShader->Program);
    bufs.AttachImageVectorBuffer(this->FBO);

    float fTexSize[2];
    fTexSize[0] = inputTexSize[0];
    fTexSize[1] = inputTexSize[1];
    this->VTShader->Program->SetUniform2f("uTexSize", fTexSize);
    this->VTShader->Program->SetUniformi("texVectors", bufs.GetVectorTextureUnit());
    vtkOpenGLCheckErrorMacro("failed");
    // essential to initialize the entire buffer
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    vtkOpenGLCheckErrorMacro("failed");
    glClear(GL_COLOR_BUFFER_BIT);
    vtkOpenGLCheckErrorMacro("failed");
    size_t nVectorExtents = vectorExtents.size();
    for (size_t q=0; q<nVectorExtents; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->VTShader);
    vtkOpenGLCheckErrorMacro("failed");
    }
    bufs.DettachImageVectorBuffer(this->FBO);
    vtkOpenGLCheckErrorMacro("failed");

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
  bufs.AttachLICBuffers(this->FBO);

  renWin->GetShaderCache()->ReadyShaderProgram(this->LIC0Shader->Program);
  this->LIC0Shader->Program->SetUniformi("uStepNo", 0);
  this->LIC0Shader->Program->SetUniformi("uPassNo", 0);
  this->LIC0Shader->Program->SetUniformf("uMaskThreshold", this->MaskThreshold);
  this->LIC0Shader->Program->SetUniform2f("uNoiseBoundsPt1", noiseBoundsPt1);
    vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");
  this->LIC0Shader->Program->SetUniformi("texMaskVectors", bufs.GetMaskVectorTextureUnit());
    vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");
  this->LIC0Shader->Program->SetUniformi("texNoise", bufs.GetNoiseTextureUnit(0));
    vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");
  this->LIC0Shader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    vtkOpenGLStaticCheckErrorMacro("failed at RenderQuad");

  for (size_t q=0; q<nComputeExtents1; ++q)
  {
    bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
      this->LIC0Shader);
  }
  bufs.DettachLICBuffers(this->FBO);
  bufs.Swap();

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_lic0b_a.vtm","lic2d_lic0b_s.vtm",computeExtents1);
  #endif

  //
  // backward LIC
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->LICIShader->Program);
  this->LICIShader->Program->SetUniformi("uPassNo", 0);
  this->LICIShader->Program->SetUniformf("uStepSize", -this->StepSize);
  this->LICIShader->Program->SetUniform2f("uNoiseBoundsPt1", noiseBoundsPt1);
  this->LICIShader->Program->SetUniformi("texVectors",
    bufs.GetImageVectorTextureUnit());
  this->LICIShader->Program->SetUniformi("texNoise", bufs.GetNoiseTextureUnit(0));

  int stepNum = 0;
  for (int stepIdx=0; stepIdx<this->NumberOfSteps; ++stepIdx, ++stepNum)
  {
    bufs.AttachLICBuffers(this->FBO);
    this->LICIShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->LICIShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
    for (size_t q=0; q<nComputeExtents1; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->LICIShader);
    }
    bufs.DettachLICBuffers(this->FBO);
    bufs.Swap();
  }

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_licib_a.vtm", "lic2d_licib_s.vtm", computeExtents1);
  #endif

  // initialize seeds
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->LIC0Shader->Program);
  this->LIC0Shader->Program->SetUniformi("uStepNo", 1);

  bufs.AttachLICBuffers(this->FBO);
  for (size_t q=0; q<nComputeExtents1; ++q)
  {
    bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
      this->LIC0Shader);
  }
  bufs.DettachLICBuffers(this->FBO);
  bufs.Swap();

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_lic0f_a.vtm", "lic2d_lic0f_s.vtm", computeExtents1);
  #endif

  //
  // forward LIC
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->LICIShader->Program);
  this->LICIShader->Program->SetUniformf("uStepSize", this->StepSize);

  for (int stepIdx=0; stepIdx<this->NumberOfSteps; ++stepIdx, ++stepNum)
  {
    bufs.AttachLICBuffers(this->FBO);
    this->LICIShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->LICIShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
    for (size_t q=0; q<nComputeExtents1; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->LICIShader);
    }
    bufs.DettachLICBuffers(this->FBO);
    bufs.Swap();
  }

  #if (vtkLineIntegralConvolution2DDEBUG >= 2)
  bufs.WriteBuffers(rank,"lic2d_licif_a.vtm", "lic2d_licif_s.vtm", computeExtents1);
  #endif

  // finally done with Noise Texture 0
  bufs.DettachNoiseTexture(0);
  bufs.DettachVectorTextures();

  //
  // finalize LIC
  //
  renWin->GetShaderCache()->ReadyShaderProgram(this->LICNShader->Program);
  this->LICNShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());

  bufs.AttachLICBuffers(this->FBO);
  this->LICNShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
  this->LICNShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
  for (size_t q=0; q<nComputeExtents1; ++q)
  {
    bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
      this->LICNShader);
  }
  bufs.DettachBuffers(this->FBO);
  bufs.Swap();

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

      renWin->GetShaderCache()->ReadyShaderProgram(this->CEShader->Program);
      this->CEShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
      this->CEShader->Program->SetUniformf("uMin", grayMin );
      this->CEShader->Program->SetUniformf("uMaxMinDiff", grayMaxMinDiff);

      bufs.AttachLICBuffers(this->FBO);
      for (size_t q=0; q<nComputeExtents1; ++q)
      {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
          this->CEShader);
      }
      bufs.DettachLICBuffers(this->FBO);
      bufs.Swap();

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
    bufs.AttachEEBuffer(this->FBO);
    #endif

    renWin->GetShaderCache()->ReadyShaderProgram(this->EEShader->Program);
    this->EEShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->EEShader->Program->SetUniformf("uDx", dx);
    this->EEShader->Program->SetUniformf("uDy", dy);

    #ifndef INTEL_BUG
    bufs.AttachEEBuffer(this->FBO);
    #endif
    for (size_t q=0; q<nComputeExtents1; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->EEShader);
    }
    bufs.DettachEEBuffer(this->FBO);

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
    bufs.DettachBuffers(this->FBO);
    bufs.ClearBuffers(this->FBO, inputTexExtent, vectorExtents, /*clearEE=*/0);
    bufs.AttachVectorTextures();
    bufs.AttachNoiseTexture(1);

    //
    // initialize convolution and seeds
    //
    renWin->GetShaderCache()->ReadyShaderProgram(this->LIC0Shader->Program);
    this->LIC0Shader->Program->SetUniformi("uStepNo", 0);
    this->LIC0Shader->Program->SetUniformi("uPassNo", 1);

    bufs.AttachLICBuffers(this->FBO);
    this->LIC0Shader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->LIC0Shader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
    this->LIC0Shader->Program->SetUniformi("texNoise", bufs.GetNoiseTextureUnit(1));
    for (size_t q=0; q<nComputeExtents1; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->LIC0Shader);
    }
    bufs.DettachLICBuffers(this->FBO);
    bufs.Swap();

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteBuffers(rank,"lic2d_elic0b_a.vtm", "lic2d_elic0b_s.vtm", computeExtents1);
    #endif

    //
    // backward LIC
    //
    renWin->GetShaderCache()->ReadyShaderProgram(this->LICIShader->Program);
    this->LICIShader->Program->SetUniformi("uPassNo", 1);
    this->LICIShader->Program->SetUniformf("uStepSize", -this->StepSize);
    this->LICIShader->Program->SetUniformi("texNoise", bufs.GetNoiseTextureUnit(1));

    int nSteps = this->NumberOfSteps/2;
    stepNum = 0;
    for (int stepIdx=0; stepIdx<nSteps; ++stepIdx, ++stepNum)
    {
      bufs.AttachLICBuffers(this->FBO);
      this->LICIShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
      this->LICIShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
      for (size_t q=0; q<nComputeExtents1; ++q)
      {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
          this->LICIShader);
      }
      bufs.DettachLICBuffers(this->FBO);
      bufs.Swap();
    }

    #if (vtkLineIntegralConvolution2DDEBUG >=2 )
    bufs.WriteBuffers(rank,"lic2d_elicib_a.vtm", "lic2d_elicib_s.vtm",computeExtents1);
    #endif

    //
    // initialize seeds
    //
    renWin->GetShaderCache()->ReadyShaderProgram(this->LIC0Shader->Program);
    this->LIC0Shader->Program->SetUniformi("uStepNo", 1);

    bufs.AttachLICBuffers(this->FBO);
    this->LIC0Shader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->LIC0Shader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
    for (size_t q=0; q<nComputeExtents1; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->LIC0Shader);
    }
    bufs.DettachLICBuffers(this->FBO);
    bufs.Swap();

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteBuffers(rank,"lic2d_elic0f_a.vtm", "lic2d_elic0f_s.vtm",computeExtents1);
    #endif

    //
    // forward LIC
    //
    renWin->GetShaderCache()->ReadyShaderProgram(this->LICIShader->Program);
    this->LICIShader->Program->SetUniformf("uStepSize", this->StepSize);

    for (int stepIdx=0; stepIdx<nSteps; ++stepIdx, ++stepNum)
    {
      bufs.AttachLICBuffers(this->FBO);
      this->LICIShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
      this->LICIShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
      for (size_t q=0; q<nComputeExtents1; ++q)
      {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
          this->LICIShader);
      }
      bufs.DettachLICBuffers(this->FBO);
      bufs.Swap();
    }

    #if (vtkLineIntegralConvolution2DDEBUG >= 2)
    bufs.WriteBuffers(rank,"lic2d_elicif_a.vtm", "lic2d_elicif_s.vtm",computeExtents1);
    #endif

    // finally done with noise tyexture 1
    bufs.DettachNoiseTexture(1);
    bufs.DettachVectorTextures();

    //
    // finalize LIC
    //
    renWin->GetShaderCache()->ReadyShaderProgram(this->LICNShader->Program);
    this->LICNShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());

    bufs.AttachLICBuffers(this->FBO);
    this->LICNShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->LICNShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
    for (size_t q=0; q<nComputeExtents1; ++q)
    {
      bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
        this->LICNShader);
    }
    bufs.DettachLICBuffers(this->FBO);
    bufs.Swap();

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

    renWin->GetShaderCache()->ReadyShaderProgram(this->AAHShader->Program);
    this->AAHShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->AAHShader->Program->SetUniformf("uDx", dx);

    renWin->GetShaderCache()->ReadyShaderProgram(this->AAVShader->Program);
    this->AAVShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->AAVShader->Program->SetUniformf("uDy", dy);

    // it's ok to compute over just the valid extent
    // we don't care here if guard pixels are smoothed
    // however computing over the entire extent avoids
    // bleeding at the edges when multiple passes are
    // requested.
    for (int i=0; i<this->AntiAlias; ++i)
    {
      // horizontal pass
      renWin->GetShaderCache()->ReadyShaderProgram(this->AAHShader->Program);
      bufs.AttachLICBuffers(this->FBO);
      this->AAHShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
      this->AAHShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
      for (size_t q=0; q<nComputeExtents1; ++q)
      {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
          this->AAHShader);
      }
      bufs.DettachLICBuffers(this->FBO);
      bufs.Swap();

      // vertical pass
      renWin->GetShaderCache()->ReadyShaderProgram(this->AAVShader->Program);
      bufs.AttachLICBuffers(this->FBO);
      this->AAVShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
      this->AAVShader->Program->SetUniformi("texSeedPts", bufs.GetSeedTextureUnit());
      for (size_t q=0; q<nComputeExtents1; ++q)
      {
        bufs.RenderQuad(&computeBounds1[4*q], computeExtents1[q],
          this->AAVShader);
      }
      bufs.DettachLICBuffers(this->FBO);
      bufs.Swap();
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
    //this->GetGlobalMinMax(comm, grayMin, grayMax);

    // select M and m as a fraction of the range.
    grayMaxMinDiff = grayMax-grayMin;
    grayMin += grayMaxMinDiff*this->LowContrastEnhancementFactor;
    grayMax -= grayMaxMinDiff*this->HighContrastEnhancementFactor;
    grayMaxMinDiff = grayMax-grayMin;

    renWin->GetShaderCache()->ReadyShaderProgram(this->CEShader->Program);
    this->CEShader->Program->SetUniformi("texLIC", bufs.GetLICTextureUnit());
    this->CEShader->Program->SetUniformf("uMin", grayMin );
    this->CEShader->Program->SetUniformf("uMaxMinDiff", grayMaxMinDiff);

    bufs.AttachLICBuffers(this->FBO);
    for (size_t q=0; q<nComputeExtents2; ++q)
    {
      bufs.RenderQuad(&computeBounds2[4*q], computeExtents2[q],
        this->CEShader);
    }
    bufs.DettachLICBuffers(this->FBO);
    bufs.Swap();

    #if (vtkLineIntegralConvolution2DDEBUG >= 1)
    bufs.WriteBuffers(rank,"lic2d_2ce.vtm", NULL, computeExtents2);
    #endif

    #if defined(vtkLineIntegralConvolution2DTIME) || defined(vtkSurfaceLICPainterTIME)
    this->EndTimerEvent("vtkLineIntegralConvolution::ContrastEnhance2");
    #endif

    // --------------------------------------------- end contrast enahnce
  }

  bufs.DettachBuffers(this->FBO);
  this->FBO->UnBind(GL_FRAMEBUFFER);

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
//    << indent << "Comm=" << this->Comm << endl
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
