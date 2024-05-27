// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc
// SPDX-License-Identifier: BSD-3-Clause
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
 * Shade the image rendered by its delegate. Two image resolutions are used
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farthest z-value and
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
#include "vtkOpenGLHelper.h"           // used for ivars
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkOpenGLFramebufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkEDLShading : public vtkDepthImageProcessingPass
{
public:
  static vtkEDLShading* New();
  vtkTypeMacro(vtkEDLShading, vtkDepthImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

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
  void EDLInitializeFramebuffers(vtkRenderState& s);

  /**
   * Initialization of required GLSL shaders
   */
  void EDLInitializeShaders(vtkOpenGLRenderWindow*);

  /**
   * Render EDL in full resolution buffer
   */
  bool EDLShadeHigh(vtkRenderState& s, vtkOpenGLRenderWindow*);

  /**
   * Render EDL in middle resolution buffer
   */
  bool EDLShadeLow(vtkRenderState& s, vtkOpenGLRenderWindow*);

  /**
   * Render EDL in middle resolution buffer
   */
  bool EDLBlurLow(vtkRenderState& s, vtkOpenGLRenderWindow*);

  /**
   * Compose color and shaded images
   */
  bool EDLCompose(const vtkRenderState* s, vtkOpenGLRenderWindow*);

  ///@{
  /**
   * Framebuffer object and textures for initial projection
   */
  vtkOpenGLFramebufferObject* ProjectionFBO;
  // used to record scene data
  vtkTextureObject* ProjectionColorTexture;
  // color render target for projection pass
  vtkTextureObject* ProjectionDepthTexture;
  // depth render target for projection pass
  ///@}

  // Framebuffer objects and textures for EDL
  vtkOpenGLFramebufferObject* EDLHighFBO;
  // for EDL full res shading
  vtkTextureObject* EDLHighShadeTexture;
  // color render target for EDL full res pass
  vtkOpenGLFramebufferObject* EDLLowFBO;
  // for EDL low res shading (image size/4)
  vtkTextureObject* EDLLowShadeTexture;
  // color render target for EDL low res pass
  vtkTextureObject* EDLLowBlurTexture;
  // color render target for EDL low res
  // bilateral filter pass

  // Shader prohrams
  vtkOpenGLHelper EDLShadeProgram;
  vtkOpenGLHelper EDLComposeProgram;
  vtkOpenGLHelper BilateralProgram;

  float EDLNeighbours[8][4];
  bool EDLIsFiltered;
  int EDLLowResFactor; // basically 4

  float Zn; // near clipping plane
  float Zf; // far clipping plane

private:
  vtkEDLShading(const vtkEDLShading&) = delete;
  void operator=(const vtkEDLShading&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
