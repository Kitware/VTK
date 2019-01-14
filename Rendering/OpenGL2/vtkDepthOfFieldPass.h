/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthOfFieldPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDepthOfFieldPass
 * @brief   Implement a post-processing DOF blur pass.
 *
 * Currently only does behind the focal plane
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * The delegate is used once.
 *
 * Its delegate is usually set to a vtkCameraPass or to a post-processing pass.
 *
 * @par Implementation:
 * As the filter is separable, it first blurs the image horizontally and then
 * vertically. This reduces the number of texture samples
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkDepthOfFieldPass_h
#define vtkDepthOfFieldPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDepthImageProcessingPass.h"

class vtkDepthPeelingPassLayerList; // Pimpl
class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkDepthOfFieldPass : public vtkDepthImageProcessingPass
{
public:
  static vtkDepthOfFieldPass *New();
  vtkTypeMacro(vtkDepthOfFieldPass,vtkDepthImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Use automatic focal distance calculation, this is on by default
   * When on the center of the viewport will always be in focus
   * regardless of where the focal point is.
   */
  vtkSetMacro(AutomaticFocalDistance,bool);
  vtkGetMacro(AutomaticFocalDistance,bool);
  vtkBooleanMacro(AutomaticFocalDistance,bool);
  //@}

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
  vtkDepthOfFieldPass();

  /**
   * Destructor.
   */
  ~vtkDepthOfFieldPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject *FrameBufferObject;
  vtkTextureObject *Pass1; // render target for the scene
  vtkTextureObject *Pass1Depth; // render target for the depth

  // Structures for the various cell types we render.
  vtkOpenGLHelper *BlurProgram;

  bool AutomaticFocalDistance;

 private:
  vtkDepthOfFieldPass(const vtkDepthOfFieldPass&) = delete;
  void operator=(const vtkDepthOfFieldPass&) = delete;
};

#endif
