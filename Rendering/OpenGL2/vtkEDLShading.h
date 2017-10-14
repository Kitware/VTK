/*=========================================================================

   Program: VTK
   Module:  vtkEDLShading.h

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
/**
 * @class   vtkEDLShading
 *
 *
 * Implement an EDL offscreen shading.
 * Shade the image renderered by its delegate. Two image resolutions are used
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 * The delegate is used once.
 *
 * Its delegate is usually set to a vtkCameraPass or to a post-processing pass.
 *
*/

#ifndef vtkEDLShading_h
#define vtkEDLShading_h

#define EDL_HIGH_RESOLUTION_ON 1
#define EDL_LOW_RESOLUTION_ON 1

#include "vtkDepthImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h" // needed for vtkSmartPointer
#include "vtkOpenGLHelper.h" // used for ivars

class vtkOpenGLRenderWindow;
class vtkOpenGLFramebufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkEDLShading : public vtkDepthImageProcessingPass
{
public:
  static vtkEDLShading *New();
  vtkTypeMacro(vtkEDLShading,vtkDepthImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w) override;

 protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkEDLShading();

  /**
   * Destructor.
   */
  ~vtkEDLShading() override;

  /**
   * Initialization of required framebuffer objects
   */
  void EDLInitializeFramebuffers(vtkRenderState &s);

  /**
   * Initialization of required GLSL shaders
   */
  void EDLInitializeShaders(vtkOpenGLRenderWindow *);

  /**
   * Render EDL in full resolution buffer
   */
  bool EDLShadeHigh(vtkRenderState &s, vtkOpenGLRenderWindow *);

  /**
   * Render EDL in middle resolution buffer
   */
  bool EDLShadeLow(vtkRenderState &s, vtkOpenGLRenderWindow *);

  /**
   * Render EDL in middle resolution buffer
   */
  bool EDLBlurLow(vtkRenderState &s, vtkOpenGLRenderWindow *);

  /**
   * Compose color and shaded images
   */
  bool EDLCompose(const vtkRenderState *s, vtkOpenGLRenderWindow *);

  //@{
  /**
   * Framebuffer object and textures for initial projection
   */
  vtkOpenGLFramebufferObject  *ProjectionFBO;
                        // used to record scene data
  vtkTextureObject      *ProjectionColorTexture;
                        // color render target for projection pass
  vtkTextureObject      *ProjectionDepthTexture;
                        // depth render target for projection pass
  //@}

  // Framebuffer objects and textures for EDL
  vtkOpenGLFramebufferObject *EDLHighFBO;
                       // for EDL full res shading
  vtkTextureObject     *EDLHighShadeTexture;
                       // color render target for EDL full res pass
  vtkOpenGLFramebufferObject *EDLLowFBO;
                       // for EDL low res shading (image size/4)
  vtkTextureObject     *EDLLowShadeTexture;
                       // color render target for EDL low res pass
  vtkTextureObject     *EDLLowBlurTexture;
                       // color render target for EDL low res
                       // bilateral filter pass

  // Shader prohrams
  vtkOpenGLHelper EDLShadeProgram;
  vtkOpenGLHelper EDLComposeProgram;
  vtkOpenGLHelper BilateralProgram;

  float EDLNeighbours[8][4];
  bool  EDLIsFiltered;
  int   EDLLowResFactor; // basically 4

  float Zn;  // near clipping plane
  float Zf;  // far clipping plane

 private:
  vtkEDLShading(const vtkEDLShading&) = delete;
  void operator=(const vtkEDLShading&) = delete;
};

#endif
