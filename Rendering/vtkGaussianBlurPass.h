/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianBlurPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGaussianBlurPass - Implement a post-processing Gaussian blur
// render pass.
// .SECTION Description
// Blur the image renderered by its delegate. Blurring uses a Gaussian low-pass
// filter with a 5x5 kernel.
//
// This pass expects an initialized depth buffer and color buffer.
// Initialized buffers means they have been cleared with farest z-value and
// background color/gradient/transparent color.
// An opaque pass may have been performed right after the initialization.
//
// The delegate is used once.
//
// Its delegate is usually set to a vtkCameraPass or to a post-processing pass.
// 
// .SECTION Implementation
// As the filter is separable, it first blurs the image horizontally and then
// vertically. This reduces the number of texture sampling to 5 per pass.
// In addition, as texture sampling can already blend texel values in linear
// mode, by adjusting the texture coordinate accordingly, only 3 texture
// sampling are actually necessary.
// Reference: OpenGL Bloom Toturial by Philip Rideout, section
// Exploit Hardware Filtering  http://prideout.net/bloom/index.php#Sneaky

// .SECTION See Also
// vtkRenderPass

#ifndef __vtkGaussianBlurPass_h
#define __vtkGaussianBlurPass_h

#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkShaderProgram2;
class vtkShader2;
class vtkFrameBufferObject;
class vtkTextureObject;

class VTK_RENDERING_EXPORT vtkGaussianBlurPass : public vtkRenderPass
{
public:
  static vtkGaussianBlurPass *New();
  vtkTypeRevisionMacro(vtkGaussianBlurPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  
  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);
  
  // Description:
  // Delegate for rendering the image to be blurred.
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // It is usually set to a vtkCameraPass or to a post-processing pass.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(DelegatePass,vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass *delegatePass);
  
 protected:
  // Description:
  // Default constructor. DelegatePass is set to NULL.
  vtkGaussianBlurPass();

  // Description:
  // Destructor.
  virtual ~vtkGaussianBlurPass();
  
  vtkRenderPass *DelegatePass;
 
  // Description:
  // Graphics resources.
  vtkFrameBufferObject *FrameBufferObject;
  vtkTextureObject *Pass1; // render target for the scene
  vtkTextureObject *Pass2; // render target for the horizontal pass
  vtkShaderProgram2 *BlurProgram; // blur shader
  
 private:
  vtkGaussianBlurPass(const vtkGaussianBlurPass&);  // Not implemented.
  void operator=(const vtkGaussianBlurPass&);  // Not implemented.
};

#endif
