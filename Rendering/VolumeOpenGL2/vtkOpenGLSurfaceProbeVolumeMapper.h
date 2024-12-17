// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLSurfaceProbeVolumeMapper
 * @brief   PolyDataMapper colored with probed volume data.
 *
 * PolyDataMapper that probes volume data at the points positions specified in its input data.
 * The rendered surface is colored using the scalar values that were probed in the source volume.
 * The mapper accepts three inputs: the Input, the Source and an optional ProbeInput.
 * The Source data defines the vtkImageData from which scalar values are interpolated.
 * The Input data defines the rendered surface.
 * The ProbeInput defines the geometry used to interpolate the source data.
 * If the ProbeInput is not specified, the Input is used both for probing and rendering.
 *
 * Projecting the scalar values from the ProbeInput to the Input is done thanks to texture
 * coordinates. Both inputs must provide texture coordinates in the [0, 1] range.
 *
 * The sampled scalar values can be computed with different blending strategy that use surface
 * normals to perform thick probing of the Source data.
 *
 * @note The following features are not supported yet but should be considered in the future.
 *
 * - The volume texture is always uploaded using linear interpolation.
 *   The public API could provide a setter to use nearest neighbor interpolation instead.
 *
 * - If the source is rendered by a volume mapper, any transform applied to the volume
 *   is ignored as there is no interface to pass this information.
 *
 * - Only the first scalar component is used for rendering and rescaled with Window/Level.
 *   Consider supporting RGB volumes without W/L mapping, and independent component.
 *   Consider supporting Color and opacity transfer function to replace W/L mapping.
 *
 * Passing a vtkVolumeProperty to this mapper should be considered to address the above points.
 *
 * - A background value of (0, 0, 0, 0) is used when probing outside the volume, but this
 *   parameters could be exposed in the public API
 *
 * - A step value corresponding to the half of the minimum spacing value of the source is used for
 *   blend modes, but it could be configured from the public API.
 */

#ifndef vtkOpenGLSurfaceProbeVolumeMapper_h
#define vtkOpenGLSurfaceProbeVolumeMapper_h

#include "vtkNew.h" // For vtkNew
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkTextureObject;
class vtkVolumeTexture;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLSurfaceProbeVolumeMapper
  : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLSurfaceProbeVolumeMapper* New();
  vtkTypeMacro(vtkOpenGLSurfaceProbeVolumeMapper, vtkOpenGLPolyDataMapper);

  ///@{
  /**
   * Specify the input data used for probing (optional).
   * If no probe data is specified, the input is used.
   */
  void SetProbeInputData(vtkPolyData* in);
  vtkPolyData* GetProbeInput();
  void SetProbeInputConnection(vtkAlgorithmOutput* algOutput);
  ///@}

  ///@{
  /**
   * Specify the input data to be probed.
   */
  void SetSourceData(vtkImageData* in);
  vtkImageData* GetSource();
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);
  ///@}

  ///@{
  /**
   * Set/Get the current window and level values used for scalar coloring.
   */
  vtkGetMacro(Window, double);
  vtkSetMacro(Window, double);

  vtkGetMacro(Level, double);
  vtkSetMacro(Level, double);
  ///@}

  ///@{
  /**
   * Set/Get the blend mode.
   * Default is none.
   *
   * \sa vtkVolumeMapper::GetBlendMode()
   */
  enum class BlendModes : unsigned int
  {
    NONE = 0,
    MAX,
    MIN,
    AVERAGE
  };

  vtkGetEnumMacro(BlendMode, BlendModes);
  vtkSetEnumMacro(BlendMode, BlendModes);
  void SetBlendModeToNone() { this->SetBlendMode(BlendModes::NONE); }
  void SetBlendModeToMaximumIntensity() { this->SetBlendMode(BlendModes::MAX); }
  void SetBlendModeToMinimumIntensity() { this->SetBlendMode(BlendModes::MIN); }
  void SetBlendModeToAverageIntensity() { this->SetBlendMode(BlendModes::AVERAGE); }
  ///@}

  ///@{
  /**
   * Set/Get the blend width in world coordinates.
   */
  vtkGetMacro(BlendWidth, double);
  vtkSetMacro(BlendWidth, double);
  ///@}

  void RenderPiece(vtkRenderer* ren, vtkActor* act) override;

  void UpdateShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

protected:
  int FillInputPortInformation(int port, vtkInformation* info) override;

  virtual void ReplaceShaderPositionPass(vtkActor* act);
  virtual void ReplaceShaderProbePass(vtkActor* act);
  virtual void UpdateShadersProbePass(vtkOpenGLHelper& cellBO, vtkRenderer* ren);

private:
  vtkOpenGLSurfaceProbeVolumeMapper();
  ~vtkOpenGLSurfaceProbeVolumeMapper() override = default;

  void CreateTexture(vtkTextureObject*, vtkOpenGLRenderWindow*);
  void ReplaceActiveFBO(vtkRenderer*);
  void RestoreActiveFBO(vtkRenderer*);

  vtkNew<vtkOpenGLFramebufferObject> FBO;

  vtkNew<vtkTextureObject> PositionsTextureObject;
  vtkNew<vtkTextureObject> NormalsTextureObject;
  vtkNew<vtkVolumeTexture> VolumeTexture;

  vtkNew<vtkImageData> TransformedSource;

  // Internal pass type used for shader updates
  enum class PassTypes : unsigned int
  {
    DEFAULT = 0,
    POSITION_TEXTURE,
    PROBE
  };
  PassTypes CurrentPass = PassTypes::DEFAULT;

  // Window / level
  double Window = 1.0;
  double Level = 0.0;

  // Blend mode
  BlendModes BlendMode = BlendModes::NONE;
  double BlendWidth = 1.0;

  // Saved state
  bool SavedScissorTestState = false;
  bool SavedBlendState = false;
  int SavedViewport[4] = {};
};

VTK_ABI_NAMESPACE_END
#endif
