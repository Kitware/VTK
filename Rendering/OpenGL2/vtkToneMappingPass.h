/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToneMappingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkToneMappingPass
 * @brief   Implement a post-processing Tone Mapping.
 *
 * Tone mapping is the process of mapping HDR colors to [0;1] range.
 * This render pass supports three different modes:
 * - Clamp: clamps the color to [0;1] range
 * - Reinhard: maps the color using formula: x/(x+1)
 * - Exponential: maps the colors using a coefficient and the formula: 1-exp(-a*x)
 *
 * Advanced tone maping like Reinhard or Exponential can be useful when several lights
 * are added to the renderer.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkToneMappingPass_h
#define vtkToneMappingPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkToneMappingPass : public vtkImageProcessingPass
{
public:
  static vtkToneMappingPass* New();
  vtkTypeMacro(vtkToneMappingPass, vtkImageProcessingPass);

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  /**
   * Enumeration of tone mapping algorithms
   */
  enum
  {
    Clamp = 0,
    Reinhard = 1,
    Exponential = 2
  };

  //@{
  /**
   * Get/Set the tone mapping type
   * Default is Clamp
   */
  vtkSetClampMacro(ToneMappingType, int, 0, 2);
  vtkGetMacro(ToneMappingType, int);
  //@}

  //@{
  /**
   * Get/Set Exposure coefficient used for exponential tone mapping.
   * Default is 1.0
   */
  vtkGetMacro(Exposure, float);
  vtkSetMacro(Exposure, float);
  //@}

protected:
  vtkToneMappingPass() = default;
  ~vtkToneMappingPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject = nullptr;
  vtkTextureObject* ColorTexture = nullptr;
  vtkOpenGLQuadHelper* QuadHelper = nullptr;

  int ToneMappingType = Clamp;
  float Exposure = 1.0;

private:
  vtkToneMappingPass(const vtkToneMappingPass&) = delete;
  void operator=(const vtkToneMappingPass&) = delete;
};

#endif
