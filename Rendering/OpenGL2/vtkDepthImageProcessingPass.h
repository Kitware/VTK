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
 * @class   vtkDepthImageProcessingPass
 * @brief   Convenient class for post-processing
 * passes. Based on vtkImageProcessingPass, but writes depth as well
 * in a texture
 *
 * Abstract class with some convenient methods frequently used in subclasses.
 *
 *
 * @sa
 * vtkRenderPass vtkDepthImageProcessingPass vtkEDLShading
 */

#ifndef vtkDepthImageProcessingPass_h
#define vtkDepthImageProcessingPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkOpenGLFramebufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkDepthImageProcessingPass
  : public vtkImageProcessingPass
{
public:
  vtkTypeMacro(vtkDepthImageProcessingPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkDepthImageProcessingPass();

  /**
   * Destructor.
   */
  ~vtkDepthImageProcessingPass() override;

  /**
   * Render delegate with a image of different dimensions than the
   * original one.
   * \pre s_exists: s!=0
   * \pre fbo_exists: fbo!=0
   * \pre fbo_has_context: fbo->GetContext()!=0
   * \pre colortarget_exists: colortarget!=0
   * \pre colortarget_has_context: colortarget->GetContext()!=0
   */
  virtual void RenderDelegate(const vtkRenderState* s, int width, int height, int newWidth,
    int newHeight, vtkOpenGLFramebufferObject* fbo, vtkTextureObject* colortarget,
    vtkTextureObject* depthtarget);

  /**
   * Read parent size - for sake of code clarity
   * This function is generic, can be useful in multiple image-based rendering classes
   * \pre s_exists: s!=0
   */
  void ReadWindowSize(const vtkRenderState* s);

  int Origin[2];   // Viewport origin
  int Width;       // parent window width
  int Height;      // parent window height
  int W;           // this width
  int H;           // this height
  int ExtraPixels; // w(h) = width(height) + 2*extrapixels

private:
  vtkDepthImageProcessingPass(const vtkDepthImageProcessingPass&) = delete;
  void operator=(const vtkDepthImageProcessingPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
