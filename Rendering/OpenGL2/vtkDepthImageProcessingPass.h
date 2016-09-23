/*=========================================================================

   Program: ParaView
   Module:    vtkDepthImageProcessingPass.h

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

class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkFrameBufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkDepthImageProcessingPass : public vtkImageProcessingPass
{
public:
  vtkTypeMacro(vtkDepthImageProcessingPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w);

  //@{
  /**
   * Delegate for rendering the image to be processed.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkCameraPass or to a post-processing pass.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(DelegatePass,vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass *delegatePass);
  //@}

 protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkDepthImageProcessingPass();

  /**
   * Destructor.
   */
  virtual ~vtkDepthImageProcessingPass();

  /**
   * Render delegate with a image of different dimensions than the
   * original one.
   * \pre s_exists: s!=0
   * \pre fbo_exists: fbo!=0
   * \pre fbo_has_context: fbo->GetContext()!=0
   * \pre colortarget_exists: colortarget!=0
   * \pre colortarget_has_context: colortarget->GetContext()!=0
   */
  virtual void RenderDelegate(const vtkRenderState *s,
                      int width,
                      int height,
                      int newWidth,
                      int newHeight,
                      vtkFrameBufferObject *fbo,
                      vtkTextureObject *colortarget,
                      vtkTextureObject *depthtarget);

  /**
   * Read parent size - for sake of code clarity
   * This function is generic, can be useful in multiple image-based rendering classes
   * \pre s_exists: s!=0
   */
  void ReadWindowSize(const vtkRenderState* s);

  vtkRenderPass *DelegatePass;
  int    Width;       // parent window width
  int    Height;      // parent window height
  int    W;           // this width
  int    H;           // this height
  int    ExtraPixels; // w(h) = width(height) + 2*extrapixels

 private:
  vtkDepthImageProcessingPass(const vtkDepthImageProcessingPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDepthImageProcessingPass&) VTK_DELETE_FUNCTION;
};

#endif
