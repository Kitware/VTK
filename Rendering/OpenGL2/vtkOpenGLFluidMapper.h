// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLFluidMapper
 * @brief   Render fluid from position data (and color, if available)
 *
 * An OpenGL mapper that display fluid volume using a screen space
 * fluid rendering technique. Thanks to Nghia Truong for the algorithm
 * and initial implementation.
 */

#ifndef vtkOpenGLFluidMapper_h
#define vtkOpenGLFluidMapper_h

#include "vtkAbstractVolumeMapper.h"

#include "vtkOpenGLHelper.h"           // used for ivars
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkShader.h"                 // for methods
#include "vtkSmartPointer.h"           // for ivars
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <map> //for methods

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLState;
class vtkOpenGLQuadHelper;
class vtkOpenGLVertexBufferObjectGroup;
class vtkPolyData;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLFluidMapper
  : public vtkAbstractVolumeMapper
{
public:
  static vtkOpenGLFluidMapper* New();
  vtkTypeMacro(vtkOpenGLFluidMapper, vtkAbstractVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkPolyData* in);
  vtkPolyData* GetInput();
  ///@}

  ///@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  vtkSetMacro(ScalarVisibility, bool);
  vtkGetMacro(ScalarVisibility, bool);
  vtkBooleanMacro(ScalarVisibility, bool);
  ///@}

  ///@{
  /**
   * Set/Get the particle radius, must be explicitly set by user
   * To fuse the gaps between particles and obtain a smooth surface,
   * this parameter need to be slightly larger than the actual particle radius,
   * (particle radius is the half distance between two consecutive particles in
   * regular pattern sampling)
   */
  vtkSetMacro(ParticleRadius, float);
  vtkGetMacro(ParticleRadius, float);
  ///@}

  ///@{
  /**
   * Get/Set the number of filter iterations to filter the depth surface
   * This is an optional parameter, default value is 3
   * Usually set this to around 3-5
   * Too many filter iterations will over-smooth the surface
   */
  vtkSetMacro(SurfaceFilterIterations, vtkTypeUInt32);
  vtkGetMacro(SurfaceFilterIterations, vtkTypeUInt32);
  ///@}

  ///@{
  /**
   * Get/Set the number of filter iterations to filter the volume thickness
   * and particle color This is an optional parameter, default value is 3
   */
  vtkSetMacro(ThicknessAndVolumeColorFilterIterations, vtkTypeUInt32);
  vtkGetMacro(ThicknessAndVolumeColorFilterIterations, vtkTypeUInt32);
  ///@}

  ///@{
  /**
   * Get/Set the filter radius for smoothing the depth surface
   * This is an optional parameter, default value is 5
   * This is not exactly the radius in pixels,
   * instead it is just a parameter used for computing the actual filter
   * radius in the screen space filtering
   */
  vtkSetMacro(SurfaceFilterRadius, vtkTypeUInt32);
  vtkGetMacro(SurfaceFilterRadius, vtkTypeUInt32);
  ///@}

  ///@{
  /**
   * Get/Set the filter radius to filter the volume thickness and particle
   * color This is an optional parameter, default value is 10 (pixels)
   */
  vtkSetMacro(ThicknessAndVolumeColorFilterRadius, float);
  vtkGetMacro(ThicknessAndVolumeColorFilterRadius, float);
  ///@}

  /**
   * Filter method to filter the depth buffer
   */
  enum FluidSurfaceFilterMethod
  {
    BilateralGaussian = 0,
    NarrowRange,
    // New filter method can be added here,
    NumFilterMethods
  };

  ///@{
  /**
   * Get/Set the filter method for filtering fluid surface
   */
  vtkSetMacro(SurfaceFilterMethod, vtkOpenGLFluidMapper::FluidSurfaceFilterMethod);
  vtkGetMacro(SurfaceFilterMethod, vtkOpenGLFluidMapper::FluidSurfaceFilterMethod);
  ///@}

  /**
   * Optional parameters, exclusively for narrow range filter
   * The first parameter is to control smoothing between surface depth values
   * The second parameter is to control curvature of the surface edges
   */
  void SetNarrowRangeFilterParameters(float lambda, float mu)
  {
    this->NRFilterLambda = lambda;
    this->NRFilterMu = mu;
  }

  /**
   * Optional parameters, exclusively for bilateral gaussian filter
   * The parameter is for controlling smoothing between surface depth values
   */
  void SetBilateralGaussianFilterParameter(float sigmaDepth)
  {
    this->BiGaussFilterSigmaDepth = sigmaDepth;
  }

  /**
   * Display mode for the fluid, default value is TransparentFluidVolume
   */
  enum FluidDisplayMode
  {
    UnfilteredOpaqueSurface = 0,
    FilteredOpaqueSurface,
    UnfilteredSurfaceNormal,
    FilteredSurfaceNormal,
    TransparentFluidVolume,
    NumDisplayModes
  };

  ///@{
  /**
   * Get/Set the display mode
   */
  vtkSetMacro(DisplayMode, vtkOpenGLFluidMapper::FluidDisplayMode);
  vtkGetMacro(DisplayMode, vtkOpenGLFluidMapper::FluidDisplayMode);
  ///@}

  ///@{
  /**
   * Get/Set the fluid attenuation color
   * (color that will be absorpted exponentially when going through the fluid
   * volume)
   */
  vtkSetVector3Macro(AttenuationColor, float);
  vtkGetVector3Macro(AttenuationColor, float);
  ///@}

  ///@{
  /**
   * Get/Set the fluid surface color if rendered in opaque surface mode
   * without particle color
   */
  vtkSetVector3Macro(OpaqueColor, float);
  vtkGetVector3Macro(OpaqueColor, float);
  ///@}

  ///@{
  /**
   * Get/Set the power value for particle color if input data has particle
   * color Default value is 0.1, and can be set to any non-negative number The
   * particle color is then recomputed as newColor = pow(oldColor, power) *
   * scale
   */
  vtkSetMacro(ParticleColorPower, float);
  vtkGetMacro(ParticleColorPower, float);
  ///@}

  ///@{
  /**
   * Get/Set the scale value for particle color if input data has particle
   * color Default value is 1.0, and can be set to any non-negative number The
   * particle color is then recomputed as newColor = pow(oldColor, power) *
   * scale
   */
  vtkSetMacro(ParticleColorScale, float);
  vtkGetMacro(ParticleColorScale, float);
  ///@}

  ///@{
  /**
   * Get/Set the fluid volume attenuation scale, which will be multiplied
   * with attenuation color Default value is 1.0, and can be set to any
   * non-negative number The larger attenuation scale, the darker fluid
   * color
   */
  vtkSetMacro(AttenuationScale, float);
  vtkGetMacro(AttenuationScale, float);
  ///@}

  ///@{
  /**
   * Get/Set the fluid surface additional reflection scale This value is in
   * [0, 1], which 0 means using the reflection color computed from fresnel
   * equation, and 1 means using reflection color as [1, 1, 1] Default value
   * is 0
   */
  vtkSetMacro(AdditionalReflection, float);
  vtkGetMacro(AdditionalReflection, float);
  ///@}

  ///@{
  /**
   * Get/Set the scale value for refraction This is needed for tweak
   * refraction of volumes with different size scales For example, fluid
   * volume having diameter of 100.0 will refract light much more than
   * volume with diameter 1.0 This value is in [0, 1], default value is 1.0
   */
  vtkSetMacro(RefractionScale, float);
  vtkGetMacro(RefractionScale, float);
  ///@}

  ///@{
  /**
   * Get/Set the fluid refraction index. The default value is 1.33 (water)
   */
  vtkSetMacro(RefractiveIndex, float);
  vtkGetMacro(RefractiveIndex, float);
  ///@}

  /**
   * This calls RenderPiece
   */
  void Render(vtkRenderer* ren, vtkVolume* vol) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

protected:
  vtkOpenGLFluidMapper();
  ~vtkOpenGLFluidMapper() override;

  /**
   * Perform string replacements on the shader templates
   */
  void UpdateDepthThicknessColorShaders(
    vtkOpenGLHelper& glHelper, vtkRenderer* renderer, vtkVolume* vol);

  /**
   * Set the shader parameters related to the actor/mapper/camera
   */
  void SetDepthThicknessColorShaderParameters(
    vtkOpenGLHelper& glHelper, vtkRenderer* renderer, vtkVolume* vol);

  /**
   * Setup the texture buffers
   */
  void SetupBuffers(vtkOpenGLRenderWindow* renderWindow);

  /**
   * Render the fluid particles
   */
  void RenderParticles(vtkRenderer* renderer, vtkVolume* vol);

  // Public parameters, their usage are stated at their Get/Set functions
  // ======>>>>>
  float ParticleRadius = 1.0f;

  FluidSurfaceFilterMethod SurfaceFilterMethod = FluidSurfaceFilterMethod::NarrowRange;
  vtkTypeUInt32 SurfaceFilterIterations = 3u;
  vtkTypeUInt32 SurfaceFilterRadius = 5u;
  float NRFilterLambda = 10.0f;
  float NRFilterMu = 1.0f;
  float BiGaussFilterSigmaDepth = 10.0f;

  vtkTypeUInt32 ThicknessAndVolumeColorFilterIterations = 3u;
  vtkTypeUInt32 ThicknessAndVolumeColorFilterRadius = 10u;

  FluidDisplayMode DisplayMode = FluidDisplayMode::TransparentFluidVolume;

  float OpaqueColor[3]{ 0.0f, 0.0f, 0.95f };
  float AttenuationColor[3]{ 0.5f, 0.2f, 0.05f };
  float ParticleColorPower = 0.1f;
  float ParticleColorScale = 1.0f;
  float AttenuationScale = 1.0f;
  float AdditionalReflection = 0.0f;
  float RefractionScale = 1.0f;
  float RefractiveIndex = 1.33f;

  bool ScalarVisibility = false;
  bool InDepthPass = true;

  // Private parameters ======>>>>>

  // Indicate that the input data has a color buffer
  bool HasVertexColor = false;

  // Cache viewport dimensions
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;

  // Cache camera parameters
  vtkMatrix4x4* CamWCVC;
  vtkMatrix3x3* CamInvertedNorms;
  vtkMatrix4x4* CamVCDC;
  vtkMatrix4x4* CamWCDC;
  vtkMatrix4x4* CamDCVC;
  vtkTypeBool CamParallelProjection;

  // Frame buffers
  vtkSmartPointer<vtkOpenGLFramebufferObject> FBFluidEyeZ;
  vtkSmartPointer<vtkOpenGLFramebufferObject> FBThickness;
  vtkSmartPointer<vtkOpenGLFramebufferObject> FBFilterThickness;
  vtkSmartPointer<vtkOpenGLFramebufferObject> FBCompNormal;
  vtkSmartPointer<vtkOpenGLFramebufferObject> FBFilterDepth;

  // Screen quad render
  vtkOpenGLQuadHelper* QuadFluidDepthFilter[NumFilterMethods]{ nullptr, nullptr };
  vtkOpenGLQuadHelper* QuadThicknessFilter = nullptr;
  vtkOpenGLQuadHelper* QuadFluidNormal = nullptr;
  vtkOpenGLQuadHelper* QuadFinalBlend = nullptr;

  // The VBO and its layout for rendering particles
  vtkSmartPointer<vtkOpenGLVertexBufferObjectGroup> VBOs;
  vtkTimeStamp VBOBuildTime; // When was the OpenGL VBO updated?
  vtkOpenGLHelper GLHelperDepthThickness;

  // Texture buffers
  enum TextureBuffers
  {
    OpaqueZ = 0,
    OpaqueRGBA,
    FluidZ,
    FluidEyeZ,
    SmoothedFluidEyeZ,
    FluidThickness,
    SmoothedFluidThickness,
    FluidNormal,
    NumTexBuffers
  };

  // These are optional texture buffers
  enum OptionalTextureBuffers
  {
    Color = 0,
    SmoothedColor,
    NumOptionalTexBuffers
  };

  vtkTextureObject* TexBuffer[NumTexBuffers];
  vtkTextureObject* OptionalTexBuffer[NumOptionalTexBuffers];
  vtkMatrix4x4* TempMatrix4;

private:
  vtkOpenGLFluidMapper(const vtkOpenGLFluidMapper&) = delete;
  void operator=(const vtkOpenGLFluidMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
