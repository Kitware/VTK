/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFXAAPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLFXAAPass
 * @brief   Render pass calling the FXAA filter
 *
 * vtkOpenGLFXAAPass is an image post processing render pass. It is a fast anti aliasing
 * filter.
 *
 * This pass usually takes the camera pass as its delegate pass.
 *
 * @note Currently, this pass wraps the existing FXAA implementation. It copies the pixels
 * from the framebuffer to a texture. A better approach would be to use the usual render pass
 * workflow to create a framebuffer drawing directly on the texture.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass
 */

#ifndef vtkOpenGLFXAAPass_h
#define vtkOpenGLFXAAPass_h

#include "vtkImageProcessingPass.h"

#include "vtkNew.h"                    // For vtkNew
#include "vtkOpenGLFXAAFilter.h"       // For vtkOpenGLFXAAFilter
#include "vtkRenderingOpenGL2Module.h" // For export macro

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLFXAAPass : public vtkImageProcessingPass
{
public:
  static vtkOpenGLFXAAPass* New();
  vtkTypeMacro(vtkOpenGLFXAAPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

protected:
  vtkOpenGLFXAAPass() = default;
  ~vtkOpenGLFXAAPass() override = default;

  /**
   * Graphics resources.
   */
  vtkNew<vtkOpenGLFXAAFilter> FXAAFilter;

private:
  vtkOpenGLFXAAPass(const vtkOpenGLFXAAPass&) = delete;
  void operator=(const vtkOpenGLFXAAPass&) = delete;
};

#endif
