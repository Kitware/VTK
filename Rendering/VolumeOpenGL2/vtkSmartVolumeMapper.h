// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSmartVolumeMapper
 * @brief   Adaptive volume mapper
 *
 * vtkSmartVolumeMapper is a volume mapper that will delegate to a specific
 * volume mapper based on rendering parameters and available hardware. Use the
 * SetRequestedRenderMode() method to control the behavior of the selection.
 * The following options are available:
 *
 * @par vtkSmartVolumeMapper::DefaultRenderMode:
 *          Allow the vtkSmartVolumeMapper to select the best mapper based on
 *          rendering parameters and hardware support. If GPU ray casting is
 *          supported, the vtkGPUVolumeRayCastMapper mapper will be used for
 *          all rendering. If not, then the vtkFixedPointVolumeRayCastMapper
 *          will be used exclusively. This is the default requested render
 *          mode, and is generally the best option. When you use this option,
 *          your volume will always be rendered, but the method used to render
 *          it may vary based on parameters and platform.
 *
 * @par vtkSmartVolumeMapper::RayCastRenderMode:
 *          Use the vtkFixedPointVolumeRayCastMapper for both interactive and
 *          still rendering. When you use this option your volume will always
 *          be rendered with the vtkFixedPointVolumeRayCastMapper.
 *
 * @par vtkSmartVolumeMapper::GPURenderMode:
 *          Use the vtkGPUVolumeRayCastMapper, if supported, for both
 *          interactive and still rendering. If the GPU ray caster is not
 *          supported (due to hardware limitations or rendering parameters)
 *          then no image will be rendered. Use this option only if you have
 *          already checked for supported based on the current hardware,
 *          number of scalar components, and rendering parameters in the
 *          vtkVolumeProperty.
 *
 * @par vtkSmartVolumeMapper::GPURenderMode:
 *  You can adjust the contrast and brightness in the rendered image using the
 *  FinalColorWindow and FinalColorLevel ivars. By default the
 *  FinalColorWindow is set to 1.0, and the FinalColorLevel is set to 0.5,
 *  which applies no correction to the computed image. To apply the window /
 *  level operation to the computer image color, first a Scale and Bias
 *  value are computed:
 *  <pre>
 *  scale = 1.0 / this->FinalColorWindow
 *  bias  = 0.5 - this->FinalColorLevel / this->FinalColorWindow
 *  </pre>
 *  To compute a new color (R', G', B', A') from an existing color (R,G,B,A)
 *  for a pixel, the following equation is used:
 *  <pre>
 *  R' = R*scale + bias*A
 *  G' = G*scale + bias*A
 *  B' = B*scale + bias*A
 *  A' = A
 *  </pre>
 * Note that bias is multiplied by the alpha component before adding because
 * the red, green, and blue component of the color are already pre-multiplied
 * by alpha. Also note that the window / level operation leaves the alpha
 * component unchanged - it only adjusts the RGB values.
 */

#ifndef vtkSmartVolumeMapper_h
#define vtkSmartVolumeMapper_h

#include "vtkImageReslice.h"                 // for VTK_RESLICE_NEAREST, VTK_RESLICE_CUBIC
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkVolumeMapper.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkFixedPointVolumeRayCastMapper;
class vtkGPUVolumeRayCastMapper;
class vtkImageResample;
class vtkMultiBlockVolumeMapper;
class vtkOSPRayVolumeInterface;
class vtkAnariVolumeInterface;
class vtkRenderWindow;
class vtkVolume;
class vtkVolumeProperty;
class vtkImageMagnitude;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT VTK_MARSHALAUTO vtkSmartVolumeMapper : public vtkVolumeMapper
{
public:
  static vtkSmartVolumeMapper* New();
  vtkTypeMacro(vtkSmartVolumeMapper, vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the final color window. This controls the contrast of
   * the image. The default value is 1.0. The Window can be
   * negative (this causes a "negative" effect on the image)
   * Although Window can be set to 0.0, any value less than
   * 0.00001 and greater than or equal to 0.0 will be set to
   * 0.00001, and any value greater than -0.00001 but less
   * than or equal to 0.0 will be set to -0.00001.
   * Initial value is 1.0.
   */
  vtkSetMacro(FinalColorWindow, float);
  ///@}

  ///@{
  /**
   * Get the final color window. Initial value is 1.0.
   */
  vtkGetMacro(FinalColorWindow, float);
  ///@}

  ///@{
  /**
   * Set the final color level. The level controls the
   * brightness of the image. The final color window will
   * be centered at the final color level, and together
   * represent a linear remapping of color values. The
   * default value for the level is 0.5.
   */
  vtkSetMacro(FinalColorLevel, float);
  ///@}

  ///@{
  /**
   * Get the final color level.
   */
  vtkGetMacro(FinalColorLevel, float);
  ///@}

  // The possible values for the default and current render mode ivars
  enum
  {
    DefaultRenderMode = 0,
    RayCastRenderMode = 1,
    GPURenderMode = 2,
    OSPRayRenderMode = 3,
    AnariRenderMode = 4,
    UndefinedRenderMode = 5,
    InvalidRenderMode = 6
  };

  /**
   * Set the requested render mode. The default is
   * vtkSmartVolumeMapper::DefaultRenderMode.
   */
  void SetRequestedRenderMode(int mode);

  /**
   * Set the requested render mode to vtkSmartVolumeMapper::DefaultRenderMode.
   * This is the best option for an application that must adapt to different
   * data types, hardware, and rendering parameters.
   */
  void SetRequestedRenderModeToDefault();

  /**
   * Set the requested render mode to vtkSmartVolumeMapper::RayCastRenderMode.
   * This option will use software rendering exclusively. This is a good option
   * if you know there is no hardware acceleration.
   */
  void SetRequestedRenderModeToRayCast();

  /**
   * Set the requested render mode to vtkSmartVolumeMapper::GPURenderMode.
   * This option will use hardware accelerated rendering exclusively. This is a
   * good option if you know there is hardware acceleration.
   */
  void SetRequestedRenderModeToGPU();

  /**
   * Set the requested render mode to vtkSmartVolumeMapper::OSPRayRenderMode.
   * This option will use intel OSPRay to do software rendering exclusively.
   */
  void SetRequestedRenderModeToOSPRay();

  /**
   * Set the requested render mode to vtkSmartVolumeMapper::AnariRenderMode.
   * This option will use ANARI to do rendering exclusively.
   */
  void SetRequestedRenderModeToAnari();

  ///@{
  /**
   * Get the requested render mode.
   */
  vtkGetMacro(RequestedRenderMode, int);
  ///@}

  /**
   * This will return the render mode used during the previous call to
   * Render().
   */
  int GetLastUsedRenderMode();

  ///@{
  /**
   * Value passed to the GPU mapper. Ignored by other mappers.
   * Maximum size of the 3D texture in GPU memory.
   * Will default to the size computed from the graphics
   * card. Can be adjusted by the user.
   * Useful if the automatic detection is defective or missing.
   */
  vtkSetMacro(MaxMemoryInBytes, vtkIdType);
  vtkGetMacro(MaxMemoryInBytes, vtkIdType);
  ///@}

  ///@{
  /**
   * Value passed to the GPU mapper. Ignored by other mappers.
   * Maximum fraction of the MaxMemoryInBytes that should
   * be used to hold the texture. Valid values are 0.1 to
   * 1.0.
   */
  vtkSetClampMacro(MaxMemoryFraction, float, 0.1f, 1.0f);
  vtkGetMacro(MaxMemoryFraction, float);
  ///@}

  ///@{
  /**
   * Set interpolation mode for downsampling (lowres GPU)
   * (initial value: cubic).
   */
  vtkSetClampMacro(InterpolationMode, int, VTK_RESLICE_NEAREST, VTK_RESLICE_CUBIC);
  vtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor();
  void SetInterpolationModeToLinear();
  void SetInterpolationModeToCubic();
  ///@}

  /**
   * This method can be used to render a representative view of the input data
   * into the supplied image given the supplied blending mode, view direction,
   * and view up vector.
   */
  void CreateCanonicalView(vtkRenderer* ren, vtkVolume* volume, vtkVolume* volume2,
    vtkImageData* image, int blend_mode, double viewDirection[3], double viewUp[3]);

  ///@{
  /**
   * If UseJittering is on, each ray traversal direction will be
   * perturbed slightly using a noise-texture to get rid of wood-grain
   * effect. This is only used by the GPU mapper.
   */
  vtkSetClampMacro(UseJittering, vtkTypeBool, 0, 1);
  vtkGetMacro(UseJittering, vtkTypeBool);
  vtkBooleanMacro(UseJittering, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If the DesiredUpdateRate of the vtkRenderWindow that caused the Render
   * falls at or above this rate, the render is considered interactive and
   * the mapper may be adjusted (depending on the render mode).
   * Initial value is 1.0.
   */
  vtkSetClampMacro(InteractiveUpdateRate, double, 1.0e-10, 1.0e10);
  ///@}

  ///@{
  /**
   * Get the update rate at or above which this is considered an
   * interactive render.
   * Initial value is 1.0.
   */
  vtkGetMacro(InteractiveUpdateRate, double);
  ///@}

  ///@{
  /**
   * If the InteractiveAdjustSampleDistances flag is enabled,
   * vtkSmartVolumeMapper interactively sets and resets the
   * AutoAdjustSampleDistances flag on the internal volume mapper. This flag
   * along with InteractiveUpdateRate is useful to adjust volume mapper sample
   * distance based on whether the render is interactive or still.
   * By default, InteractiveAdjustSampleDistances is enabled.
   */
  vtkSetClampMacro(InteractiveAdjustSampleDistances, vtkTypeBool, 0, 1);
  vtkGetMacro(InteractiveAdjustSampleDistances, vtkTypeBool);
  vtkBooleanMacro(InteractiveAdjustSampleDistances, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If AutoAdjustSampleDistances is on, the ImageSampleDistance
   * will be varied to achieve the allocated render time of this
   * prop (controlled by the desired update rate and any culling in
   * use).
   * Note that, this flag is ignored when InteractiveAdjustSampleDistances is
   * enabled. To explicitly set and use this flag, one must disable
   * InteractiveAdjustSampleDistances.
   */
  vtkSetClampMacro(AutoAdjustSampleDistances, vtkTypeBool, 0, 1);
  vtkGetMacro(AutoAdjustSampleDistances, vtkTypeBool);
  vtkBooleanMacro(AutoAdjustSampleDistances, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the distance between samples used for rendering
   * when AutoAdjustSampleDistances is off, or when this mapper
   * has more than 1 second allocated to it for rendering.
   * If SampleDistance is negative, it will be computed based on the dataset
   * spacing. Initial value is -1.0.
   */
  vtkSetMacro(SampleDistance, float);
  vtkGetMacro(SampleDistance, float);
  ///@}

  ///@{
  /**
   * @copydoc vtkGPUVolumeRayCastMapper::SetGlobalIlluminationReach(float)
   *
   * This parameter is only used when the underlying mapper
   * is a vtkGPUVolumeRayCastMapper.
   */
  vtkSetClampMacro(GlobalIlluminationReach, float, 0.0f, 1.0f);
  vtkGetMacro(GlobalIlluminationReach, float);
  ///@}

  ///@{
  /**
   * @copydoc vtkGPUVolumeRayCastMapper::SetVolumetricScatteringBlending(float)
   *
   * This parameter is only used when the underlying mapper
   * is a vtkGPUVolumeRayCastMapper.
   */
  vtkSetClampMacro(VolumetricScatteringBlending, float, 0.0f, 2.0f);
  vtkGetMacro(VolumetricScatteringBlending, float);
  ///@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Initialize rendering for this volume.
   */
  void Render(vtkRenderer*, vtkVolume*) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  ///@{
  /**
   * VectorMode is a special rendering mode for 3-component vectors which makes
   * use of GPURayCastMapper's independent-component capabilities. In this mode,
   * a single component in the vector can be selected for rendering. In addition,
   * the mapper can compute a scalar field representing the magnitude of this vector
   * using a vtkImageMagnitude object (MAGNITUDE mode).
   */
  enum VectorModeType
  {
    DISABLED = -1,
    MAGNITUDE = 0,
    COMPONENT = 1,
  };

  void SetVectorMode(int mode);
  vtkGetMacro(VectorMode, int);

  vtkSetClampMacro(VectorComponent, int, 0, 3);
  vtkGetMacro(VectorComponent, int);
  ///@}

  ///@{
  /**
   * Set/Get the transfer 2D Y axis array
   */
  vtkSetStringMacro(Transfer2DYAxisArray);
  vtkGetStringMacro(Transfer2DYAxisArray);
  ///@}

  ///@{
  /**
   * LowResDisable disables low res mode (default)
   * LowResResample enable low res mode by automatically resampling the volume,
   * this enable large volume to be displayed at higher frame rate at the cost of
   * rendering quality
   * Actual resample factor will be determined using MaxMemoryInBytes and MaxMemoryFraction
   */
  enum LowResModeType
  {
    LowResModeDisabled = 0,
    LowResModeResample = 1,
  };

  vtkSetMacro(LowResMode, int);
  vtkGetMacro(LowResMode, int)
  ///@}

protected:
  vtkSmartVolumeMapper();
  ~vtkSmartVolumeMapper() override;

  /**
   * Connect input of the vtkSmartVolumeMapper to the input of the
   * internal volume mapper by doing a shallow to avoid memory leaks.
   * \pre m_exists: m!=0
   */
  void ConnectMapperInput(vtkVolumeMapper* m);

  /**
   * Connect input of the vtkSmartVolumeMapper to the input of the
   * internal resample filter by doing a shallow to avoid memory leaks.
   * \pre m_exists: f!=0
   */
  void ConnectFilterInput(vtkImageResample* f);

  ///@{
  /**
   * Window / level ivars
   */
  float FinalColorWindow;
  float FinalColorLevel;
  ///@}

  ///@{
  /**
   * GPU mapper-specific memory ivars.
   */
  vtkIdType MaxMemoryInBytes;
  float MaxMemoryFraction;
  ///@}

  /**
   * Used for downsampling.
   */
  int InterpolationMode;

  ///@{
  /**
   * The requested render mode is used to compute the current render mode. Note
   * that the current render mode can be invalid if the requested mode is not
   * supported.
   */
  int RequestedRenderMode;
  int CurrentRenderMode;
  ///@}

  ///@{
  /**
   * Initialization variables.
   */
  int Initialized;
  vtkTimeStamp SupportStatusCheckTime;
  int GPUSupported;
  int RayCastSupported;
  int LowResGPUNecessary;
  ///@}

  /**
   * This is the resample filter that may be used if we need to
   * create a low resolution version of the volume for GPU rendering
   */
  vtkImageResample* GPUResampleFilter;

  ///@{
  /**
   * This filter is used to compute the magnitude of 3-component data. MAGNITUDE
   * is one of the supported modes when rendering separately a single independent
   * component.
   *
   * \note
   * This feature was added specifically for ParaView so it might eventually be
   * moved into a derived mapper in ParaView.
   */
  vtkImageMagnitude* ImageMagnitude;
  vtkImageData* InputDataMagnitude;
  ///@}

  /**
   * The initialize method. Called from ComputeRenderMode whenever something
   * relevant has changed.
   */
  void Initialize(vtkRenderer* ren, vtkVolume* vol);

  /**
   * The method that computes the render mode from the requested render mode
   * based on the support status for each render method.
   */
  void ComputeRenderMode(vtkRenderer* ren, vtkVolume* vol);

  /**
   * Expose GPU mapper for additional customization.
   */
  friend class vtkMultiBlockVolumeMapper;
  vtkGetObjectMacro(GPUMapper, vtkGPUVolumeRayCastMapper);

  ///@{
  /**
   * The three potential mappers
   */
  vtkGPUVolumeRayCastMapper* GPULowResMapper;
  vtkGPUVolumeRayCastMapper* GPUMapper;
  vtkFixedPointVolumeRayCastMapper* RayCastMapper;
  ///@}

  /**
   * We need to keep track of the blend mode we had when we initialized
   * because we need to reinitialize (and recheck hardware support) if
   * it changes
   */
  int InitializedBlendMode;

  /**
   * Enable / disable stochastic jittering
   */
  vtkTypeBool UseJittering;

  /**
   * The distance between sample points along the ray
   */
  float SampleDistance;

  /**
   * Secondary rays ambient/global adjustment coefficient
   */
  float GlobalIlluminationReach = 0.0;

  /**
   * Blending coefficient between surfacic and volumetric models in GPU Mapper
   */
  float VolumetricScatteringBlending = 0.0;

  /**
   * Set whether or not the sample distance should be automatically calculated
   * within the internal volume mapper
   */
  vtkTypeBool AutoAdjustSampleDistances;

  /**
   * If the DesiredUpdateRate of the vtkRenderWindow causing the Render is at
   * or above this value, the render is considered interactive. Otherwise it is
   * considered still.
   */
  double InteractiveUpdateRate;

  /**
   * If the InteractiveAdjustSampleDistances flag is enabled,
   * vtkSmartVolumeMapper interactively sets and resets the
   * AutoAdjustSampleDistances flag on the internal volume mapper. This flag
   * along with InteractiveUpdateRate is useful to adjust volume mapper sample
   * distance based on whether the render is interactive or still.
   */
  vtkTypeBool InteractiveAdjustSampleDistances;

  ///@{
  /**
   * VectorMode is a special rendering mode for 3-component vectors which makes
   * use of GPURayCastMapper's independent-component capabilities. In this mode,
   * a single component in the vector can be selected for rendering. In addition,
   * the mapper can compute a scalar field representing the magnitude of this vector
   * using a vtkImageMagnitude object (MAGNITUDE mode).
   */
  int VectorMode;
  int VectorComponent;
  vtkTimeStamp MagnitudeUploadTime;
  ///@}

  ///@{
  /**
   * Keep a cache of the last input to the mapper so that input data changes can be propagated to
   * the resample filter and internal mappers.
   */
  vtkDataSet* LastInput;
  vtkDataSet* LastFilterInput;
  ///@}

  /**
   * Define the array used for the Y axis of transfer 2D.
   * This is used when the transfer function  mode is set to 2D. If unset, the
   * default is to use the gradient of the scalar.
   */
  char* Transfer2DYAxisArray;

  int LowResMode = LowResModeDisabled;

private:
  ///@{
  /**
   * Adjust the GPUMapper's parameters (ColorTable, Weights, etc.) to render
   * a single component of a dataset.
   */
  void SetupVectorMode(vtkVolume* vol);
  /**
   * vtkImageMagnitude is used to compute the norm of the input multi-component
   * array. vtkImageMagnitude can only process point data, so in the case of cell
   * data it is first transformed to points.
   */
  void ComputeMagnitudeCellData(vtkDataSet* input, vtkDataArray* arr);
  void ComputeMagnitudePointData(vtkDataSet* input, vtkDataArray* arr);
  ///@}

  vtkSmartVolumeMapper(const vtkSmartVolumeMapper&) = delete;
  void operator=(const vtkSmartVolumeMapper&) = delete;

  vtkOSPRayVolumeInterface* OSPRayMapper;
  vtkAnariVolumeInterface* AnariMapper;
};

VTK_ABI_NAMESPACE_END
#endif
