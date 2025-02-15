// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkShadowMapBakerPass
 * @brief   Implement a builder of shadow map pass.
 *
 * Bake a list of shadow maps, once per spot light.
 * It work in conjunction with the vtkShadowMapPass, which uses the
 * shadow maps for rendering the opaque geometry (a technique to render hard
 * shadows in hardware).
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farthest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 *
 *
 * Its delegate is usually set to a vtkOpaquePass.
 *
 * @par Implementation:
 * The first pass of the algorithm is to generate a shadow map per light
 * (depth map from the light point of view) by rendering the opaque objects
 *
 * @sa
 * vtkRenderPass, vtkOpaquePass, vtkShadowMapPass
 */

#ifndef vtkShadowMapBakerPass_h
#define vtkShadowMapBakerPass_h

#include "vtkOpenGLRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // for ivars
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO
#include <vector>                      // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkInformationIntegerKey;
class vtkCamera;
class vtkLight;
class vtkOpenGLFramebufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkShadowMapBakerPass : public vtkOpenGLRenderPass
{
public:
  static vtkShadowMapBakerPass* New();
  vtkTypeMacro(vtkShadowMapBakerPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Delegate for rendering the camera, lights, and opaque geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It defaults to a vtkCameraPass with a sequence of
   * vtkLightPass/vtkOpaquePass.
   */
  vtkGetObjectMacro(OpaqueSequence, vtkRenderPass);
  virtual void SetOpaqueSequence(vtkRenderPass* opaqueSequence);
  ///@}

  ///@{
  /**
   * Delegate for compositing of the shadow maps across processors.
   * If it is NULL, there is no z compositing.
   * It is usually set to a vtkCompositeZPass (Parallel package).
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(CompositeZPass, vtkRenderPass);
  virtual void SetCompositeZPass(vtkRenderPass* compositeZPass);
  ///@}

  ///@{
  /**
   * Set/Get the number of pixels in each dimension of the shadow maps
   * (shadow maps are square). Initial value is 256. The greater the better.
   * Resolution does not have to be a power-of-two value.
   */
  vtkSetMacro(Resolution, unsigned int);
  vtkGetMacro(Resolution, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the exponential constant for the Exponential Shadow Maps. The
   * default value differs from the value recommended by the authors of the Exponential
   * Shadow Map paper, VTK uses 11.f instead of 80.f. Empirically this improves rendering
   * performance with minimal tradeoff in shadow resolution.
   *
   * The author's recommended value of 80.f represents the maximum practical value for
   * 32-bit floating point precision in the shadow map. Values that are too
   * small will lead to "light leaking" (where shadows get attenuated away from
   * the light source). Larger values will cause shadows near the light to
   * disappear.
   */
  vtkSetMacro(ExponentialConstant, float);
  vtkGetMacro(ExponentialConstant, float);
  ///@}

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Tell if there is at least one shadow.
   * Initial value is false.
   */
  bool GetHasShadows();

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Tell if the light `l' can create shadows.
   * The light has to not be a head light and to be directional or
   * positional with an angle less than 180 degrees.
   * \pre l_exists: l!=0
   */
  bool LightCreatesShadow(vtkLight* l);

  /**
   * INTERNAL USE ONLY
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Give access to the baked shadow maps.
   */
  std::vector<vtkSmartPointer<vtkTextureObject>>* GetShadowMaps();

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Give access the cameras builds from the lights.
   */
  std::vector<vtkSmartPointer<vtkCamera>>* GetLightCameras();

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Do the shadows need to be updated?
   * Value changed by vtkShadowMapBakerPass and used by vtkShadowMapPass.
   * Initial value is true.
   */
  bool GetNeedUpdate();

  // // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Set NeedUpate to false. Called by vtkShadowMapPass.
  void SetUpToDate();

protected:
  /**
   * Default constructor. DelegatetPass is set to NULL.
   */
  vtkShadowMapBakerPass();

  /**
   * Destructor.
   */
  ~vtkShadowMapBakerPass() override;

  // vtkOpenGLRenderPass virtuals:
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;
  bool SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper, vtkProp* prop,
    vtkOpenGLVertexArrayObject* VAO = nullptr) override;

  /**
   * Helper method to compute the mNearest point in a given direction.
   * To be called several times, with initialized = false the first time.
   * v: point
   * pt: origin of the direction
   * dir: direction
   */
  void PointNearFar(
    double* v, double* pt, double* dir, double& mNear, double& mFar, bool initialized);

  /**
   * Compute the min/max of the projection of a box in a given direction.
   * bb: bounding box
   * pt: origin of the direction
   * dir: direction
   */
  void BoxNearFar(double* bb, double* pt, double* dir, double& mNear, double& mFar);

  /**
   * Build a camera from spot light parameters.
   * \pre light_exists: light!=0
   * \pre lcamera_exists: lcamera!=0
   */
  void BuildCameraLight(vtkLight* light, double* boundingBox, vtkCamera* lcamera);

  /**
   * Check if shadow mapping is supported by the current OpenGL context.
   * \pre w_exists: w!=0
   */
  void CheckSupport(vtkOpenGLRenderWindow* w);

  vtkRenderPass* OpaqueSequence;

  vtkRenderPass* CompositeZPass;

  unsigned int Resolution;
  float ExponentialConstant{ 11.0f };

  bool HasShadows;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject* FrameBufferObject;

  std::vector<vtkSmartPointer<vtkTextureObject>>* ShadowMaps;
  std::vector<vtkSmartPointer<vtkCamera>>* LightCameras;

  vtkTimeStamp LastRenderTime;
  bool NeedUpdate;
  size_t CurrentLightIndex;

private:
  vtkShadowMapBakerPass(const vtkShadowMapBakerPass&) = delete;
  void operator=(const vtkShadowMapBakerPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
