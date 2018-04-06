/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleMotionBlurPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimpleMotionBlurPass
 * @brief   Avergae frames to simulate motion blur.
 *
 * A slow and simple approach that simply renders multiple frames
 * and accumulates them before displaying them. As such it causes the
 * render process to be SubFrames times slower than normal but handles
 * all types of motion correctly as it is actually rendering all the
 * sub frames.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkSimpleMotionBlurPass_h
#define vtkSimpleMotionBlurPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDepthImageProcessingPass.h"

class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkSimpleMotionBlurPass : public vtkDepthImageProcessingPass
{
public:
  static vtkSimpleMotionBlurPass *New();
  vtkTypeMacro(vtkSimpleMotionBlurPass,vtkDepthImageProcessingPass);
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

  //@{
  /**
   * Set the number of sub frames for doing motion blur.
   * Once this is set greater than one, you will no longer see a new frame
   * for every Render().  If you set this to five, you will need to do
   * five Render() invocations before seeing the result. This isn't
   * very impressive unless something is changing between the Renders.
   * Changing this value may reset the current subframe count.
   */
  vtkGetMacro(SubFrames, int);
  virtual void SetSubFrames(int subFrames);
  //@}

  /**
   *  Set the format to use for the depth texture
   * e.g. vtkTextureObject::Float32
   */
  vtkSetMacro(DepthFormat, int);

  /**
   *  Set the format to use for the color texture
   *  vtkTextureObject::Float16 vtkTextureObject::Float32
   *  and vtkTextureObject::Fixed8 are supported. Fixed8
   *  is the default.
   */
  vtkSetMacro(ColorFormat, int);

  // Get the depth texture object
  vtkGetObjectMacro(DepthTexture, vtkTextureObject);

  // Get the Color texture object
  vtkGetObjectMacro(ColorTexture, vtkTextureObject);

 protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkSimpleMotionBlurPass();

  /**
   * Destructor.
   */
  ~vtkSimpleMotionBlurPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject *FrameBufferObject;
  vtkTextureObject *ColorTexture; // render target for the scene
  vtkTextureObject *AccumulationTexture[2]; // where we add the colors
  vtkTextureObject *DepthTexture; // render target for the depth

  //@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  int DepthFormat;
  int ColorFormat;

  int SubFrames;               // number of sub frames
  int CurrentSubFrame;         // what one are we on
  int ActiveAccumulationTexture;
  vtkOpenGLHelper *BlendProgram;

 private:
  vtkSimpleMotionBlurPass(const vtkSimpleMotionBlurPass&) = delete;
  void operator=(const vtkSimpleMotionBlurPass&) = delete;
};

#endif
