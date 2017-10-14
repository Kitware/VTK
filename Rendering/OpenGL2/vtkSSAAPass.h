/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSSAAPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSSAAPass
 * @brief   Implement Screen Space Anti Aliasing pass.
 *
 * Render to a larger image and then sample down
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
 * vertically. This reduces the number of texture samples taken.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkSSAAPass_h
#define vtkSSAAPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkSSAAPass : public vtkRenderPass
{
public:
  static vtkSSAAPass *New();
  vtkTypeMacro(vtkSSAAPass,vtkRenderPass);
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
  vtkSSAAPass();

  /**
   * Destructor.
   */
  ~vtkSSAAPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject *FrameBufferObject;
  vtkTextureObject *Pass1; // render target for the scene
  vtkTextureObject *Pass2; // render target for the horizontal pass

  // Structures for the various cell types we render.
  vtkOpenGLHelper *SSAAProgram;

  vtkRenderPass *DelegatePass;

 private:
  vtkSSAAPass(const vtkSSAAPass&) = delete;
  void operator=(const vtkSSAAPass&) = delete;
};

#endif
