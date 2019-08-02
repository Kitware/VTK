/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLFluidMapper
 * @brief   Render fluid from position data (and color, if available)
 *
 * An OpenGL mapper that display fluid volume using screen space fluid rendering technique
 */

#ifndef vtkOpenGLFluidMapper_h
#define vtkOpenGLFluidMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkAbstractVolumeMapper.h"
#include "vtkShader.h"                 // for methods
#include "vtkOpenGLHelper.h"           // used for ivars
#include "vtkSmartPointer.h"
#include "vtkMatrix3x3.h"

#include <map> //for methods

class vtkMatrix4x4;
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLState;
class vtkOpenGLQuadHelper;
class vtkOpenGLVertexBufferObjectGroup;
class vtkPolyData;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLFluidMapper : public vtkAbstractVolumeMapper {
public:
    static vtkOpenGLFluidMapper* New();
    vtkTypeMacro(vtkOpenGLFluidMapper, vtkAbstractVolumeMapper)
    virtual void PrintSelf(ostream& os, vtkIndent indent) override;

    /**
     * Specify the particle position input data
     */
    void SetInputData(vtkPolyData* particles);

    /**
     * Specify the particle position and particle color input data
     * The color buffer must be an array of float of [r, g, b] values in range [0, 1] for each particle
     */
    void SetInputData(vtkPolyData* particles, vtkPolyData* colors);

    /**
     * Set/Get the particle radius, must be explicitly set by user
     * To fuse the gaps between particles and obtain a smooth surface,
     * this parameter need to be slightly larger than the actual particle radius,
     * (particle radius is the half distance between two consecutive particles in regular pattern sampling)
     */
    vtkSetMacro(ParticleRadius, float)
    vtkGetMacro(ParticleRadius, float)

    /**
     * Get/Set the number of filter iterations to filter the depth surface
     * This is an optional parameter, default value is 3
     * Usually set this to around 3-5
     * Too many filter iterations will over-smooth the surface
     */
    vtkSetMacro(SurfaceFilterIterations, uint32_t)
    vtkGetMacro(SurfaceFilterIterations, uint32_t)

    /**
     * Get/Set the number of filter iterations to filter the volume thickness and particle color
     * This is an optional parameter, default value is 3
     */
    vtkSetMacro(ThicknessAndVolumeColorFilterIterations, uint32_t)
    vtkGetMacro(ThicknessAndVolumeColorFilterIterations, uint32_t)

    /**
     * Get/Set the filter radius for smoothing the depth surface
     * This is an optional parameter, default value is 5
     * This is not exactly the radius in pixels,
     * instead it is just a parameter used for computing the actual filter radius in the screen space filtering
     */
    vtkSetMacro(SurfaceFilterRadius, uint32_t)
    vtkGetMacro(SurfaceFilterRadius, uint32_t)

    /**
     * Get/Set the filter radius to filter the volume thickness and particle color
     * This is an optional parameter, default value is 10 (pixels)
     */
    vtkSetMacro(ThicknessAndVolumeColorFilterRadius, float)
    vtkGetMacro(ThicknessAndVolumeColorFilterRadius, float)

    /**
     * Filter method to filter the depth buffer
     */
    enum FluidSurfaceFilterMethod {
        BilateralGaussian = 0,
        NarrowRange,
        // New filter method can be added here,
        NumFilterMethods
    };

    /**
     * Get/Set the filter method for filtering fluid surface
     */
    vtkSetMacro(SurfaceFilterMethod, vtkOpenGLFluidMapper::FluidSurfaceFilterMethod)
    vtkGetMacro(SurfaceFilterMethod, vtkOpenGLFluidMapper::FluidSurfaceFilterMethod)

    /**
     * Optional parameters, exclusively for narrow range filter
     * The first parameter is to control smoothing between surface depth values
     * The second parameter is to control curvature of the surface edges
     */
    void SetNarrowRangeFilterParameters(float lambda_, float mu_) { NRFilterLambda = lambda_; NRFilterMu = mu_; }

    /**
     * Optional parameters, exclusively for bilateral gaussian filter
     * The parameter is for controlling smoothing between surface depth values
     */
    void SetBilateralGaussianFilterParameter(float sigmaDepth_) { BiGaussFilterSigmaDepth = sigmaDepth_; }

    /**
     * Display mode for the fluid, default value is TransparentFluidVolume
     */
    enum FluidDisplayMode {
        UnfilteredOpaqueSurface = 0,
        FilteredOpaqueSurface,
        UnfilteredSurfaceNormal,
        FilteredSurfaceNormal,
        TransparentFluidVolume,
        NumDisplayModes
    };

    /**
     * Get/Set the display mode
     */
    vtkSetMacro(DisplayMode, vtkOpenGLFluidMapper::FluidDisplayMode)
    vtkGetMacro(DisplayMode, vtkOpenGLFluidMapper::FluidDisplayMode)

    /**
     * Get/Set the fluid attennuation color
     * (color that will be absorpted exponentially when going through the fluid volume)
     */
    vtkSetVector3Macro(AttennuationColor, float)
    vtkGetVector3Macro(AttennuationColor, float)

    /**
     * Get/Set the fluid surface color if rendered in opaque surface mode without particle color
     */
    vtkSetVector3Macro(OpaqueColor, float)
    vtkGetVector3Macro(OpaqueColor, float)

    /**
     * Get/Set the power value for particle color if input data has particle color
     * Default value is 0.1, and can be set to any non-negative number
     * The particle color is then recomputed as newColor = pow(oldColor, power) * scale
     */
    vtkSetMacro(ParticleColorPower, float)
    vtkGetMacro(ParticleColorPower, float)

    /**
     * Get/Set the scale value for particle color if input data has particle color
     * Default value is 1.0, and can be set to any non-negative number
     * The particle color is then recomputed as newColor = pow(oldColor, power) * scale
     */
    vtkSetMacro(ParticleColorScale, float)
    vtkGetMacro(ParticleColorScale, float)

    /**
     * Get/Set the fluid volume attennuation scale, which will be multiple with attennuation color
     * Default value is 1.0, and can be set to any non-negative number
     * The larger attennuation scale, the darker fluid color
     */
    vtkSetMacro(AttennuationScale, float)
    vtkGetMacro(AttennuationScale, float)

    /**
     * Get/Set the fluid surface additional reflection scale
     * This value is in [0, 1], which 0 means using the reflection color computed from fresnel equation,
     * and 1 means using reflection color as [1, 1, 1]
     * Default value is 0
     */
    vtkSetMacro(AdditionalReflection, float)
    vtkGetMacro(AdditionalReflection, float)

    /**
     * Get/Set the scale value for refraction
     * This is needed for tweak refraction of volumes with different size scales
     * For example, fluid volume having diameter of 100.0 will refract light much more than volume with diameter 1.0
     * This value is in [0, 1], default value is 1.0
     */
    vtkSetMacro(RefractionScale, float)
    vtkGetMacro(RefractionScale, float)

    /**
     * Get/Set the fluid refraction index
     * Default value is 1.33 (water)
     */
    vtkSetMacro(RefractiveIndex, float)
    vtkGetMacro(RefractiveIndex, float)

    /**
     * This calls RenderPiece
     */
    virtual void Render(vtkRenderer* ren, vtkVolume* vol) override;

protected:
    vtkOpenGLFluidMapper();
    virtual ~vtkOpenGLFluidMapper() override;

    /**
     * Perform string replacements on the shader templates
     */
    void UpdateDepthThicknessColorShaders(vtkOpenGLHelper& glHelper, vtkRenderer* renderer, vtkVolume* vol);

    /**
     * Set the shader parameters related to the actor/mapper/camera
     */
    void SetDepthThicknessColorShaderParameters(vtkOpenGLHelper& glHelper, vtkRenderer* renderer, vtkVolume* vol);

    /**
     * Setup the texture buffers
     */
    void SetupBuffers(vtkOpenGLRenderWindow* const renderWindow);

    /**
     * Render the fluid particles
     */
    void RenderParticles(vtkRenderer* renderer, vtkVolume* vol, bool bDepthShader);

    // Public parameters, their usage are stated at their Get/Set functions ======>>>>>
    float ParticleRadius = 0.0f;

    FluidSurfaceFilterMethod SurfaceFilterMethod     = FluidSurfaceFilterMethod::NarrowRange;
    uint32_t                 SurfaceFilterIterations = 3u;
    uint32_t                 SurfaceFilterRadius     = 5u;
    float                    NRFilterLambda          = 10.0f;
    float                    NRFilterMu = 1.0f;
    float                    BiGaussFilterSigmaDepth = 10.0f;

    uint32_t ThicknessAndVolumeColorFilterIterations = 3u;
    uint32_t ThicknessAndVolumeColorFilterRadius     = 10u;

    FluidDisplayMode DisplayMode = FluidDisplayMode::TransparentFluidVolume;

    float OpaqueColor[3] { 0.0f, 0.0f, 0.95f };
    float AttennuationColor[3] { 0.5f, 0.2f, 0.05f };
    float ParticleColorPower   = 0.1f;
    float ParticleColorScale   = 1.0f;
    float AttennuationScale    = 1.0f;
    float AdditionalReflection = 0.0f;
    float RefractionScale      = 1.0f;
    float RefractiveIndex      = 1.33f;

    // Private parameters ======>>>>>

    // Indicate that the input data has a color buffer
    bool m_bHasVertexColor = false;

    // Cache viewport dimensions
    int m_ViewportX;
    int m_ViewportY;
    int m_ViewportWidth;
    int m_ViewportHeight;

    // Cache camera parameters
    vtkMatrix4x4* m_CamWCVC;
    vtkMatrix3x3* m_CamNorms;
    vtkMatrix4x4* m_CamVCDC;
    vtkMatrix4x4* m_CamWCDC;
    bool          m_CamParallelProjection;

    // Frame buffers
    vtkSmartPointer<vtkOpenGLFramebufferObject> m_FBFluidEyeZ       = nullptr;
    vtkSmartPointer<vtkOpenGLFramebufferObject> m_FBThickness       = nullptr;
    vtkSmartPointer<vtkOpenGLFramebufferObject> m_FBFilterThickness = nullptr;
    vtkSmartPointer<vtkOpenGLFramebufferObject> m_FBCompNormal      = nullptr;
    vtkSmartPointer<vtkOpenGLFramebufferObject> m_FBFilterDepth     = nullptr;

    // Screen quad render
    vtkOpenGLQuadHelper* m_QuadFluidDepthFilter[NumFilterMethods] { nullptr, nullptr };
    vtkOpenGLQuadHelper* m_QuadThicknessFilter = nullptr;
    vtkOpenGLQuadHelper* m_QuadFluidNormal     = nullptr;
    vtkOpenGLQuadHelper* m_QuadFinalBlend      = nullptr;

    // The VBO and its layout for rendering particles
    vtkSmartPointer<vtkOpenGLVertexBufferObjectGroup> m_VBOs;
    vtkTimeStamp    m_VBOBuildTime; // When was the OpenGL VBO updated?
    vtkOpenGLHelper m_GLHelperDepthThickness;

    // Texture buffers
    enum TextureBuffers {
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
    enum OptionalTextureBuffers {
        Color = 0,
        SmoothedColor,
        NumOptionalTexBuffers
    };

    vtkTextureObject* m_TexBuffer[NumTexBuffers];
    vtkTextureObject* m_OptionalTexBuffer[NumOptionalTexBuffers];
    vtkMatrix4x4*     m_TempMatrix4;

private:
    vtkOpenGLFluidMapper(const vtkOpenGLFluidMapper&) = delete;
    void operator=(const vtkOpenGLFluidMapper&)       = delete;
};

#endif
