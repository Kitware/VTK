// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSSAOPass
 * @brief   Implement a screen-space ambient occlusion pass.
 *
 * SSAO darkens some pixels to improve depth perception simulating ambient occlusion
 * in screen space.
 * For each fragment, random samples inside a hemisphere at the fragment position oriented with
 * the normal are tested against other fragments to compute an average occlusion.
 * The number of samples and the radius of the hemisphere are configurables.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkSSAOPass_h
#define vtkSSAOPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include "vtkTextureObject.h" // For texture format enum

#include <vector> // For vector

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix4x4;
class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkSSAOPass : public vtkImageProcessingPass
{
public:
  static vtkSSAOPass* New();
  vtkTypeMacro(vtkSSAOPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  /**
   * Pre replace shader values
   */
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  /**
   * Post replace shader values
   */
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  /**
   * Set shader parameters. Set the draw buffers depending on the mapper.
   */
  bool SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper, vtkProp* prop,
    vtkOpenGLVertexArrayObject* VAO = nullptr) override;

  ///@{
  /**
   * Get/Set the SSAO hemisphere radius.
   * Default is 0.5
   */
  vtkGetMacro(Radius, double);
  vtkSetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Get/Set the number of samples.
   * Default is 32
   */
  vtkGetMacro(KernelSize, unsigned int);
  vtkSetClampMacro(KernelSize, unsigned int, 1, 1000);
  ///@}

  ///@{
  /**
   * Get/Set the bias when comparing samples.
   * Default is 0.01
   */
  vtkGetMacro(Bias, double);
  vtkSetMacro(Bias, double);
  ///@}

  ///@{
  /**
   * Get/Set blurring of the ambient occlusion.
   * Blurring can help to improve the result if samples number is low.
   * Default is false
   */
  vtkGetMacro(Blur, bool);
  vtkSetMacro(Blur, bool);
  vtkBooleanMacro(Blur, bool);
  ///@}

  /**
   *  Set the format to use for the depth texture
   *  vtkTextureObject::Float32 and vtkTextureObject::Fixed32 are supported.
   */
  vtkSetMacro(DepthFormat, int);

  ///@{
  /**
   * Get/Set the opacity threshold value used to write depth information for volumes.
   * When the opacity of the current raycast sample reaches this value, the fragment depth is
   * written to the depth buffer which results in SSAO being applied at this location.
   * Default is 0.9
   */
  vtkGetMacro(VolumeOpacityThreshold, double);
  vtkSetClampMacro(VolumeOpacityThreshold, double, 0.0, 1.0);
  ///@}

  ///@{
  /**
   * Control intensity of darkening.
   * Default is 1.0. Larger value causes stronger darkening. 0 means no darkening at all.
   */
  vtkGetMacro(IntensityScale, double);
  vtkSetMacro(IntensityScale, double);
  ///@}

  ///@{
  /**
   * Control intensity of darkening.
   * Range is between 0.0 and 1.0. Default is 0.0. Larger value prevents darkening
   * lightly occluded regions, which can be particularly noticeable when IntensityScale is set to a
   * higher value.
   */
  vtkGetMacro(IntensityShift, double);
  vtkSetClampMacro(IntensityShift, double, 0.0, 1.0);
  ///@}

protected:
  vtkSSAOPass() = default;
  ~vtkSSAOPass() override = default;

  /**
   * Called in PreRender to add the GLDepthMaskOverride information key to volumes,
   * which allows them to write to the depth texture by overriding the value of glDepthMask.
   */
  void PreRenderProp(vtkProp* prop) override;
  /**
   * Called in PostRender to clean the GLDepthMaskOverride information key on volumes.
   */
  void PostRenderProp(vtkProp* prop) override;

  void ComputeKernel();
  void InitializeGraphicsResources(vtkOpenGLRenderWindow* renWin, int w, int h);

  void RenderDelegate(const vtkRenderState* s, int w, int h);
  void RenderSSAO(vtkOpenGLRenderWindow* renWin, vtkMatrix4x4* projection, int w, int h);
  void RenderCombine(vtkOpenGLRenderWindow* renWin);

  vtkTextureObject* ColorTexture = nullptr;
  vtkTextureObject* PositionTexture = nullptr;
  vtkTextureObject* NormalTexture = nullptr;
  vtkTextureObject* SSAOTexture = nullptr;
  vtkTextureObject* DepthTexture = nullptr;

  int DepthFormat = vtkTextureObject::Float32;

  vtkOpenGLFramebufferObject* FrameBufferObject = nullptr;

  vtkOpenGLQuadHelper* SSAOQuadHelper = nullptr;
  vtkOpenGLQuadHelper* CombineQuadHelper = nullptr;

  std::vector<float> Kernel;
  unsigned int KernelSize = 32;
  double Radius = 0.5;
  double Bias = 0.01;
  bool Blur = false;

  double VolumeOpacityThreshold = 0.9;

  double IntensityScale = 1.0;
  double IntensityShift = 0.0;

private:
  vtkSSAOPass(const vtkSSAOPass&) = delete;
  void operator=(const vtkSSAOPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
