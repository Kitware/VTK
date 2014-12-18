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
// This pass requires a OpenGL context that supports texture objects (TO),
// framebuffer objects (FBO) and GLSL. If not, it will emit an error message
// and will render its delegate and return.
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

#ifndef vtkGaussianBlurPass_h
#define vtkGaussianBlurPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkImageProcessingPass.h"

class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkFrameBufferObject;
class vtkTextureObject;
namespace vtkgl
{
class CellBO;
}

class VTKRENDERINGOPENGL2_EXPORT vtkGaussianBlurPass : public vtkImageProcessingPass
{
public:
  static vtkGaussianBlurPass *New();
  vtkTypeMacro(vtkGaussianBlurPass,vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX

  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);

 protected:
  // Description:
  // Default constructor. DelegatePass is set to NULL.
  vtkGaussianBlurPass();

  // Description:
  // Destructor.
  virtual ~vtkGaussianBlurPass();

  // Description:
  // Graphics resources.
  vtkFrameBufferObject *FrameBufferObject;
  vtkTextureObject *Pass1; // render target for the scene
  vtkTextureObject *Pass2; // render target for the horizontal pass

  // Structures for the various cell types we render.
  vtkgl::CellBO *BlurProgram;

  bool Supported;
  bool SupportProbed;

 private:
  vtkGaussianBlurPass(const vtkGaussianBlurPass&);  // Not implemented.
  void operator=(const vtkGaussianBlurPass&);  // Not implemented.
};

#endif
