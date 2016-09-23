/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSobelGradientMagnitudePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSobelGradientMagnitudePass
 * @brief   Implement a post-processing edge
 * detection with a Sobel gradient magnitude render pass.
 *
 * Detect the edges of the image renderered by its delegate. Edge-detection
 * uses a Sobel high-pass filter (3x3 kernel).
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
 * This pass requires a OpenGL context that supports texture objects (TO),
 * framebuffer objects (FBO) and GLSL. If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @par Implementation:
 * To compute the gradient magnitude, the x and y components of the gradient
 * (Gx and Gy) have to be computed first. Each computation of Gx and Gy uses
 * a separable filter.
 * The first pass takes the image from the delegate as the single input
 * texture.
 * The first pass has two outputs, one for the first part of Gx, Gx1, result of
 * a convolution with (-1 0 1), one for the first part of Gy, Gy1, result of a
 * convolution with (1 2 1).
 * The second pass has two inputs, Gx1 and Gy1. Kernel (1 2 1)^T is applied
 * to Gx1 and kernel (-1 0 1)^T is applied to Gx2. It gives the values for
 * Gx and Gy. Thoses values are then used to compute the magnitude of the
 * gradient which is stored in the render target.
 * The gradient computation happens per component (R,G,B). A is arbitrarly set
 * to 1 (full opacity).
 *
 * @par Implementation:
 * \image html vtkSobelGradientMagnitudePassFigure.png
 * \image latex vtkSobelGradientMagnitudePassFigure.eps
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkSobelGradientMagnitudePass_h
#define vtkSobelGradientMagnitudePass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkImageProcessingPass.h"

class vtkDepthPeelingPassLayerList; // Pimpl
class vtkFrameBufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;


class VTKRENDERINGOPENGL2_EXPORT vtkSobelGradientMagnitudePass : public vtkImageProcessingPass
{
public:
  static vtkSobelGradientMagnitudePass *New();
  vtkTypeMacro(vtkSobelGradientMagnitudePass,vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  virtual void Render(const vtkRenderState *s);

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w);

 protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkSobelGradientMagnitudePass();

  /**
   * Destructor.
   */
  virtual ~vtkSobelGradientMagnitudePass();

  /**
   * Graphics resources.
   */
  vtkFrameBufferObject *FrameBufferObject;
  vtkTextureObject *Pass1; // render target for the scene
  vtkTextureObject *Gx1; // render target 0 for the first shader
  vtkTextureObject *Gy1; // render target 1 for the first shader

  // Structures for the various cell types we render.
  vtkOpenGLHelper *Program1; // shader to compute Gx1 and Gy1
  vtkOpenGLHelper *Program2; // shader to compute |G| from Gx1 and Gy1

 private:
  vtkSobelGradientMagnitudePass(const vtkSobelGradientMagnitudePass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSobelGradientMagnitudePass&) VTK_DELETE_FUNCTION;
};

#endif
