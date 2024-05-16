// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLRenderer
 * @brief   OpenGL renderer
 *
 * vtkOpenGLRenderer is a concrete implementation of the abstract class
 * vtkRenderer. vtkOpenGLRenderer interfaces to the OpenGL graphics library.
 */

#ifndef vtkOpenGLRenderer_h
#define vtkOpenGLRenderer_h

#include "vtkRenderer.h"

#include "vtkOpenGLQuadHelper.h"       // for ivar
#include "vtkPBRIrradianceTexture.h"   // for ivar
#include "vtkPBRLUTTexture.h"          // for ivar
#include "vtkPBRPrefilterTexture.h"    // for ivar
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // For vtkSmartPointer
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO
#include <memory>                      // for unique_ptr
#include <string>                      // Ivars
#include <vector>                      // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;
class vtkOpenGLFXAAFilter;
class vtkRenderPass;
class vtkOpenGLState;
class vtkOpenGLTexture;
class vtkOrderIndependentTranslucentPass;
class vtkTextureObject;
class vtkDepthPeelingPass;
class vtkShaderProgram;
class vtkShadowMapPass;
class vtkSSAOPass;
class vtkPolyData;
class vtkTexturedActor2D;
class vtkPolyDataMapper2D;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLRenderer : public vtkRenderer
{
public:
  static vtkOpenGLRenderer* New();
  vtkTypeMacro(vtkOpenGLRenderer, vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Concrete open gl render method.
   */
  void DeviceRender() override;

  /**
   * Overridden to support hidden line removal.
   */
  void DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* fbo = nullptr) override;

  /**
   * Render translucent polygonal geometry. Default implementation just call
   * UpdateTranslucentPolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with depth peeling must
   * override this method.
   */
  void DeviceRenderTranslucentPolygonalGeometry(vtkFrameBufferObjectBase* fbo = nullptr) override;

  void Clear() override;

  /**
   * Ask lights to load themselves into graphics pipeline.
   */
  int UpdateLights() override;

  /**
   * Is rendering at translucent geometry stage using depth peeling and
   * rendering a layer other than the first one? (Boolean value)
   * If so, the uniform variables UseTexture and Texture can be set.
   * (Used by vtkOpenGLProperty or vtkOpenGLTexture)
   */
  int GetDepthPeelingHigherLayer();

  /**
   * Indicate if this system is subject to the apple/NVIDIA bug that causes
   * crashes in the driver when too many query objects are allocated.
   */
  static bool HaveAppleQueryAllocationBug();

  /**
   * Dual depth peeling may be disabled for certain runtime configurations.
   * This method returns true if vtkDualDepthPeelingPass will be used in place
   * of vtkDepthPeelingPass.
   */
  bool IsDualDepthPeelingSupported();

  // Get the state object used to keep track of
  // OpenGL state
  vtkOpenGLState* GetState();

  // get the standard lighting uniform declarations
  // for the current set of lights
  const char* GetLightingUniforms();

  // update the lighting uniforms for this shader if they
  // are out of date
  void UpdateLightingUniforms(vtkShaderProgram* prog);

  // get the complexity of the current lights as a int
  // 0 = no lighting
  // 1 = headlight
  // 2 = directional lights
  // 3 = positional lights
  enum LightingComplexityEnum
  {
    NoLighting = 0,
    Headlight = 1,
    Directional = 2,
    Positional = 3
  };
  vtkGetMacro(LightingComplexity, int);

  // get the number of lights turned on
  vtkGetMacro(LightingCount, int);

  ///@{
  /**
   * Set the user light transform applied after the camera transform.
   * Can be null to disable it.
   */
  void SetUserLightTransform(vtkTransform* transform);
  vtkTransform* GetUserLightTransform();
  ///@}

  ///@{
  /**
   * Get environment textures used for image based lighting.
   */
  vtkSetSmartPointerMacro(EnvMapLookupTable, vtkPBRLUTTexture);
  vtkPBRLUTTexture* GetEnvMapLookupTable();
  vtkSetSmartPointerMacro(EnvMapIrradiance, vtkPBRIrradianceTexture);
  vtkPBRIrradianceTexture* GetEnvMapIrradiance();
  vtkSetSmartPointerMacro(EnvMapPrefiltered, vtkPBRPrefilterTexture);
  vtkPBRPrefilterTexture* GetEnvMapPrefiltered();
  ///@}

  /**
   * Get spherical harmonics coefficients used for irradiance
   */
  vtkFloatArray* GetSphericalHarmonics();

  ///@{
  /**
   * Use spherical harmonics instead of irradiance texture
   */
  vtkSetMacro(UseSphericalHarmonics, bool);
  vtkGetMacro(UseSphericalHarmonics, bool);
  vtkBooleanMacro(UseSphericalHarmonics, bool);
  ///@}

  /**
   * Set/Get the environment texture used for image based lighting.
   * This texture is supposed to represent the scene background.
   * If it is not a cubemap, the texture is supposed to represent an equirectangular projection.
   * If used with raytracing backends, the texture must be an equirectangular projection and must be
   * constructed with a valid vtkImageData.
   * Warning, this texture must be expressed in linear color space.
   * If the texture is in sRGB color space, set the color flag on the texture or
   * set the argument isSRGB to true.
   * Note that this texture can be omitted if LUT, SpecularColorMap and SphericalHarmonics
   * are used and provided
   *
   * @sa vtkTexture::UseSRGBColorSpaceOn
   */
  void SetEnvironmentTexture(vtkTexture* texture, bool isSRGB = false) override;

  // Method to release graphics resources
  void ReleaseGraphicsResources(vtkWindow* w) override;

protected:
  vtkOpenGLRenderer();
  ~vtkOpenGLRenderer() override;

  /**
   * Check the compilation status of some fragment shader source.
   */
  void CheckCompilation(unsigned int fragmentShader);

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both vtkActors and vtkVolumes
   * Returns the number of props that rendered geometry.
   */
  int UpdateGeometry(vtkFrameBufferObjectBase* fbo = nullptr) override;

  /**
   * Check and return the textured background for the current state
   * If monocular or stereo left eye, check BackgroundTexture
   * If stereo right eye, check RightBackgroundTexture
   */
  vtkTexture* GetCurrentTexturedBackground();

  friend class vtkOpenGLProperty;
  friend class vtkOpenGLTexture;
  friend class vtkOpenGLImageSliceMapper;
  friend class vtkOpenGLImageResliceMapper;

  /**
   * FXAA is delegated to an instance of vtkOpenGLFXAAFilter
   */
  vtkOpenGLFXAAFilter* FXAAFilter;

  /**
   * Depth peeling is delegated to an instance of vtkDepthPeelingPass
   */
  vtkDepthPeelingPass* DepthPeelingPass;

  /**
   * Fallback for transparency
   */
  vtkOrderIndependentTranslucentPass* TranslucentPass;

  /**
   * Shadows are delegated to an instance of vtkShadowMapPass
   */
  vtkShadowMapPass* ShadowMapPass;

  /**
   * SSAO is delegated to an instance of vtkSSAOPass
   */
  vtkSSAOPass* SSAOPass;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  friend class vtkRenderPass;

  std::string LightingDeclaration;
  int LightingComplexity;
  int LightingCount;
  vtkMTimeType LightingUpdateTime;

  /**
   * Optional user transform for lights
   */
  vtkSmartPointer<vtkTransform> UserLightTransform;

  vtkSmartPointer<vtkPBRLUTTexture> EnvMapLookupTable;
  vtkSmartPointer<vtkPBRIrradianceTexture> EnvMapIrradiance;
  vtkSmartPointer<vtkPBRPrefilterTexture> EnvMapPrefiltered;
  vtkSmartPointer<vtkFloatArray> SphericalHarmonics;
  bool UseSphericalHarmonics;

  vtkSmartPointer<vtkTexturedActor2D> BackgroundTextureActor;
  vtkSmartPointer<vtkTexturedActor2D> BackgroundGradientActor;
  vtkSmartPointer<vtkPolyDataMapper2D> BackgroundMapper;
  vtkSmartPointer<vtkPolyData> BackgroundQuad;

private:
  vtkOpenGLRenderer(const vtkOpenGLRenderer&) = delete;
  void operator=(const vtkOpenGLRenderer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
