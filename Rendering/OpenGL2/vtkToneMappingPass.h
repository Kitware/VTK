// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkToneMappingPass
 * @brief   Implement a post-processing Tone Mapping.
 *
 * Tone mapping is the process of mapping HDR colors to [0;1] range.
 * This render pass supports four different modes:
 * - Clamp: clamps the color to [0;1] range
 * - Reinhard: maps the color using formula: x/(x+1)
 * - Exponential: maps the colors using a coefficient and the formula: 1-exp(-a*x)
 * - GenericFilmic: maps the color using five parameters that allow to shape the
 *                  tonemapping curve : Contrast adjust the toe (left part) of the curve;
 *                  Shoulder adjusts the right part; MidIn and MidOut adjusts the middle gray
 *                  level in percent of the curve for input and output (ie. the halfway point
 *                  between white and black); and HdrMax is the maximum HDR input that is not
 *                  clipped. A boolean UseACES allows to use the Academy Color Encoding System.
 *
 * Advanced tone mapping like GenericFilmic, Reinhard or Exponential can be useful when several
 * lights are added to the renderer.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkToneMappingPass_h
#define vtkToneMappingPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkToneMappingPass : public vtkImageProcessingPass
{
public:
  static vtkToneMappingPass* New();
  vtkTypeMacro(vtkToneMappingPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Set function to set uncharted 2 presets, and default presets
   */
  void SetGenericFilmicDefaultPresets();
  void SetGenericFilmicUncharted2Presets();
  ///@}

  /**
   * Enumeration of tone mapping algorithms
   */
  enum
  {
    Clamp = 0,
    Reinhard = 1,
    Exponential = 2,
    GenericFilmic = 3,
    NeutralPBR = 4
  };

  ///@{
  /**
   * Get/Set the tone mapping type.
   * Default is GenericFilmic
   */
  vtkSetClampMacro(ToneMappingType, int, 0, 4);
  vtkGetMacro(ToneMappingType, int);
  ///@}

  ///@{
  /**
   * Get/Set Exposure coefficient used for exponential and Generic Filmic tone mapping.
   * Default is 1.0
   */
  vtkGetMacro(Exposure, float);
  vtkSetMacro(Exposure, float);
  ///@}

  ///@{
  /**
   * Contrast adjust the toe of the curve. Typically in [1-2].
   * Default is 1.6773
   */
  vtkSetClampMacro(Contrast, float, 0.0001f, VTK_FLOAT_MAX);
  vtkGetMacro(Contrast, float);
  ///@}

  ///@{
  /**
   * Shoulder limit the output in the shoulder region of the curve.
   * Typically in [0.9-1].
   * Default is 0.9714
   */
  vtkSetClampMacro(Shoulder, float, 0.0001, 1.f);
  vtkGetMacro(Shoulder, float);
  ///@}

  ///@{
  /**
   * Mid level anchor input.
   * Default is 0.18 (in percent gray)
   */
  vtkSetClampMacro(MidIn, float, 0.0001, 1.f);
  vtkGetMacro(MidIn, float);
  ///@}

  ///@{
  /**
   * Mid level anchor output.
   * Default is 0.18 (in percent gray)
   */
  vtkSetClampMacro(MidOut, float, 0.0001, 1.f);
  vtkGetMacro(MidOut, float);
  ///@}

  ///@{
  /**
   * Maximum HDR input that is not clipped.
   * Default is 11.0785
   */
  vtkSetClampMacro(HdrMax, float, 1.f, VTK_FLOAT_MAX);
  vtkGetMacro(HdrMax, float);
  ///@}

  ///@{
  /**
   * Apply or not the Academy Color Encoding System (ACES).
   * Default is true
   */
  vtkSetMacro(UseACES, bool);
  vtkGetMacro(UseACES, bool);
  ///@}

protected:
  vtkToneMappingPass() = default;
  ~vtkToneMappingPass() override;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject = nullptr;
  vtkTextureObject* ColorTexture = nullptr;
  vtkOpenGLQuadHelper* QuadHelper = nullptr;

  vtkMTimeType PreComputeMTime = 0;

  int ToneMappingType = GenericFilmic;
  float Exposure = 1.0;

  /**
   * Parameters for Generic Filmic Tonemapping
   */
  float Contrast = 1.6773;
  float Shoulder = 0.9714;
  float MidIn = 0.18;
  float MidOut = 0.18;
  float HdrMax = 11.0785;
  bool UseACES = true;

  /**
   * Used to recompile the shader if UseACES is modified
   */
  bool UseACESChangeValue = true;

  /**
   * Computed from previous parameters.
   * Allow to anchor the curve.
   * Default value are for GenericFilmic default preset
   */
  float ClippingPoint = 1.117427;
  float ToeSpeed = 0.244676;

  /**
   * Pre compute ClippingPoint and ToeSpeed.
   */
  void PreComputeAnchorCurveGenericFilmic();

private:
  vtkToneMappingPass(const vtkToneMappingPass&) = delete;
  void operator=(const vtkToneMappingPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
