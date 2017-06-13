/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFramebufferPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFramebufferPass
 * @brief   Render into a FO
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkFramebufferPass_h
#define vtkFramebufferPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDepthImageProcessingPass.h"

class vtkOpenGLFramebufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkFramebufferPass : public vtkDepthImageProcessingPass
{
public:
  static vtkFramebufferPass *New();
  vtkTypeMacro(vtkFramebufferPass,vtkDepthImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) VTK_OVERRIDE;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w) VTK_OVERRIDE;

  /**
   *  Set the format to use for the depth texture
   * e.g. vtkTextureObject::Float32
   */
  vtkSetMacro(DepthFormat, int);

  // Get the depth texture object
  vtkGetObjectMacro(DepthTexture, vtkTextureObject);

  // Get the Color texture object
  vtkGetObjectMacro(ColorTexture, vtkTextureObject);

 protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkFramebufferPass();

  /**
   * Destructor.
   */
  ~vtkFramebufferPass() VTK_OVERRIDE;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject *FrameBufferObject;
  vtkTextureObject *ColorTexture; // render target for the scene
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

 private:
  vtkFramebufferPass(const vtkFramebufferPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFramebufferPass&) VTK_DELETE_FUNCTION;
};

#endif
