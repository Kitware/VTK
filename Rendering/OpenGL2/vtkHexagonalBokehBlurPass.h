// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2019-2021 Kitware SAS
// SPDX-FileCopyrightText: Copyright 2025 F3D-APP Foundation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHexagonalBokehBlurPass
 * @brief Screen space render pass that blur a given image using the hexagonal bokeh method.
 *
 * Screen space algorithm to blur an image. This blur is based on the algorithm described in
 * the link below. "Advances in Real-Time Rendering", Siggraph 2011
 * https://colinbarrebrisebois.com/2017/04/18/hexagonal-bokeh-blur-revisited-part-1-basic-3-pass-version/
 *
 * This blur is useful for "out of focus blur" look, for example for blurring a skybox.
 */

#ifndef vtkHexagonalBokehBlurPass_h
#define vtkHexagonalBokehBlurPass_h

#include "vtkImageProcessingPass.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkSmartPointer.h"

class vtkTextureObject;
class vtkOpenGLFrameBuffer;

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkHexagonalBokehBlurPass
  : public vtkImageProcessingPass
{
public:
  static vtkHexagonalBokehBlurPass* New();
  vtkTypeMacro(vtkHexagonalBokehBlurPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Entry point of the rendering execution for this render pass.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources held by the render pass.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Get/Set the size of the circle of confusion.
   * This radius defines how far from one pixel should the blur function go to compute the blur
   * color.
   * Default value is 20.0. This value should be positive.
   */
  vtkGetMacro(CircleOfConfusionRadius, float);
  vtkSetMacro(CircleOfConfusionRadius, float);
  ///@}

protected:
  vtkHexagonalBokehBlurPass() = default;
  ~vtkHexagonalBokehBlurPass() override = default;

private:
  vtkHexagonalBokehBlurPass(const vtkHexagonalBokehBlurPass&) = delete;
  void operator=(const vtkHexagonalBokehBlurPass&) = delete;

  /**
   * Setup graphics resources needed in this pass.
   */
  void InitializeGraphicsResources(vtkOpenGLRenderWindow* renderWindow, int width, int height);

  /**
   * Render delegate pass.
   */
  void RenderDelegate(const vtkRenderState* s, int width, int height);

  /**
   * First pass: blur vertically and diagonally the input to produce two outputs.
   */
  void RenderDirectionalBlur(vtkOpenGLRenderWindow* renWin, int width, int height, float coc);

  /**
   * Second pass: Combine and blur the two inputs to get the final result.
   */
  void RenderRhomboidBlur(vtkOpenGLRenderWindow* renWin, int width, int height, float coc);

  float CircleOfConfusionRadius = 20.0f;

  vtkSmartPointer<vtkOpenGLFramebufferObject> FrameBufferObject;
  vtkSmartPointer<vtkTextureObject> VerticalBlurTexture;
  vtkSmartPointer<vtkTextureObject> DiagonalBlurTexture;
  vtkSmartPointer<vtkTextureObject> BackgroundTexture;

  std::unique_ptr<vtkOpenGLQuadHelper> BlurQuadHelper = nullptr;
  std::unique_ptr<vtkOpenGLQuadHelper> RhomboidQuadHelper = nullptr;
};

VTK_ABI_NAMESPACE_END

#endif // vtkHexagonalBokehBlurPass_h
