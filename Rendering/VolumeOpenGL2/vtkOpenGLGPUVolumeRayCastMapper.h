// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLGPUVolumeRayCastMapper
 * @brief OpenGL implementation of volume rendering through ray-casting.
 *
 * @section multi Multiple Inputs

 * When multiple inputs are rendered simultaneously, it is possible to
 * composite overlapping areas correctly. Inputs are connected directly to
 * the mapper and their parameters (transfer functions, transformations, etc.)
 * are specified through standard vtkVolume instances. These vtkVolume
 * instances are to be registered in a special vtkProp3D, vtkMultiVolume.
 *
 * Structures related to a particular active input are stored in a helper
 * class (vtkVolumeInputHelper) and helper structures are kept in a
 * port-referenced map (VolumeInputMap). The order of the inputs in the
 * map is important as it defines the order in which parameters are
 * bound to uniform variables (transformation matrices, bias, scale and every
 * other required rendering parameter).
 *
 * A separate code path is used when rendering multiple-inputs in order to
 * facilitate the co-existance of these two modes (single/multiple), due to
 * current feature incompatibilities with multiple inputs (e.g. texture-streaming,
 * cropping, etc.).
 *
 * @note A limited set of the mapper features are currently supported for
 * multiple inputs:
 *
 * - Blending
 *   - Composite (front-to-back)
 *
 * - Transfer functions (defined separately for per input)
 *   - 1D color
 *   - 1D scalar opacity
 *   - 1D gradient magnitude opacity
 *   - 2D scalar-gradient magnitude
 *
 * - Point and cell data
 *   - With the limitation that all of the inputs are assumed to share the same
 *     name/id.
 *
 * - Inputs
 *   - 1-component inputs with vtkVolumeProperty::IndependentComponentsOn()
 *   - 4-component inputs with vtkVolumeProperty::IndependentComponentsOff()
 *
 * @sa vtkGPUVolumeRayCastMapper vtkVolumeInputHelper vtkVolumeTexture
 * vtkMultiVolume
 *
 */

#ifndef vtkOpenGLGPUVolumeRayCastMapper_h
#define vtkOpenGLGPUVolumeRayCastMapper_h
#include <map> // For methods

#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkNew.h"                          // For vtkNew
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkShader.h"                       // For methods
#include "vtkSmartPointer.h"                 // For smartptr
#include "vtkWrappingHints.h"                // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericOpenGLResourceFreeCallback;
class vtkImplicitFunction;
class vtkOpenGLCamera;
class vtkOpenGLTransferFunctions2D;
class vtkOpenGLVolumeGradientOpacityTables;
class vtkOpenGLVolumeOpacityTables;
class vtkOpenGLVolumeRGBTables;
class vtkShaderProgram;
class vtkTextureObject;
class vtkVolume;
class vtkVolumeInputHelper;
class vtkVolumeTexture;
class vtkOpenGLShaderProperty;

class VTKRENDERINGVOLUMEOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLGPUVolumeRayCastMapper
  : public vtkGPUVolumeRayCastMapper
{
public:
  static vtkOpenGLGPUVolumeRayCastMapper* New();

  enum Passes
  {
    RenderPass,
    DepthPass = 1
  };

  vtkTypeMacro(vtkOpenGLGPUVolumeRayCastMapper, vtkGPUVolumeRayCastMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Low level API to enable access to depth texture in
  // RenderToTexture mode. It will return either nullptr if
  // RenderToImage was never turned on or texture captured
  // the last time RenderToImage was on.
  vtkTextureObject* GetDepthTexture();

  // Description:
  // Low level API to enable access to color texture in
  // RenderToTexture mode. It will return either nullptr if
  // RenderToImage was never turned on or texture captured
  // the last time RenderToImage was on.
  vtkTextureObject* GetColorTexture();

  // Description:
  // Low level API to export the depth texture as vtkImageData in
  // RenderToImage mode.
  void GetDepthImage(vtkImageData* im) override;

  // Description:
  // Low level API to export the color texture as vtkImageData in
  // RenderToImage mode.
  void GetColorImage(vtkImageData* im) override;

  // Description:
  // Mapper can have multiple passes and internally it will set
  // the state. The state can not be set externally explicitly
  // but can be set indirectly depending on the options set by
  // the user.
  vtkGetMacro(CurrentPass, int);

  // Sets a depth texture for this mapper to use
  // This allows many mappers to use the same
  // texture reducing GPU usage. If this is set
  // the standard depth texture code is skipped
  // The depth texture should be activated
  // and deactivated outside of this class
  void SetSharedDepthTexture(vtkTextureObject* nt);

  /**
   * Set a fixed number of partitions in which to split the volume
   * during rendering. This will force by-block rendering without
   * trying to compute an optimum number of partitions.
   */
  void SetPartitions(unsigned short x, unsigned short y, unsigned short z);

  /**
   *  Load the volume texture into GPU memory.  Actual loading occurs
   *  in vtkVolumeTexture::LoadVolume.  The mapper by default loads data
   *  lazily (at render time), so it is most commonly not necessary to call
   *  this function.  This method is only exposed in order to support on-site
   *  loading which is useful in cases where the user needs to know a-priori
   *  whether loading will succeed or not.
   */
  bool PreLoadData(vtkRenderer* ren, vtkVolume* vol);

  // Description:
  // Delete OpenGL objects.
  // \post done: this->OpenGLObjectsCreated==0
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkOpenGLGPUVolumeRayCastMapper();
  ~vtkOpenGLGPUVolumeRayCastMapper() override;

  vtkGenericOpenGLResourceFreeCallback* ResourceCallback;

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildDepthPassShader(
    vtkRenderer* ren, vtkVolume* vol, int noOfComponents, int independentComponents);

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildShader(vtkRenderer* ren);

  // TODO Take these out as these are no longer needed
  // Methods called by the AMR Volume Mapper.
  void PreRender(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol),
    double vtkNotUsed(datasetBounds)[6], double vtkNotUsed(scalarRange)[2],
    int vtkNotUsed(noOfComponents), unsigned int vtkNotUsed(numberOfLevels)) override
  {
  }

  // \pre input is up-to-date
  void RenderBlock(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol),
    unsigned int vtkNotUsed(level)) override
  {
  }

  void PostRender(vtkRenderer* vtkNotUsed(ren), int vtkNotUsed(noOfComponents)) override {}

  // Description:
  // Rendering volume on GPU
  void GPURender(vtkRenderer* ren, vtkVolume* vol) override;

  // Description:
  // Method that performs the actual rendering given a volume and a shader
  void DoGPURender(vtkRenderer* ren, vtkOpenGLCamera* cam, vtkShaderProgram* shaderProgram,
    vtkOpenGLShaderProperty* shaderProperty);

  // Description:
  // Update the reduction factor of the render viewport (this->ReductionFactor)
  // according to the time spent in seconds to render the previous frame
  // (this->TimeToDraw) and a time in seconds allocated to render the next
  // frame (allocatedTime).
  // \pre valid_current_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  // \pre positive_TimeToDraw: this->TimeToDraw>=0.0
  // \pre positive_time: allocatedTime>0
  // \post valid_new_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  void ComputeReductionFactor(double allocatedTime);

  // Description:
  // Returns a reduction ratio for each dimension
  // This ratio is computed from MaxMemoryInBytes and MaxMemoryFraction so that the total
  // memory usage of the resampled image, by the returned ratio, does not exceed
  // `MaxMemoryInBytes * MaxMemoryFraction`
  // \pre input is up-to-date
  // \post Aspect ratio of image is always kept
  // - for a 1D image `ratio[1] == ratio[2] == 1`
  // - for a 2D image `ratio[0] == ratio[1]` and `ratio[2] == 1`
  // - for a 3D image `ratio[0] == ratio[1] == ratio[2]`
  void GetReductionRatio(double* ratio) override;

  // Description:
  // Empty implementation.
  int IsRenderSupported(
    vtkRenderWindow* vtkNotUsed(window), vtkVolumeProperty* vtkNotUsed(property)) override
  {
    return 1;
  }

  ///@{
  /**
   *  \brief vtkOpenGLRenderPass API
   */
  vtkMTimeType GetRenderPassStageMTime(vtkVolume* vol);

  /**
   * Create the basic shader template strings before substitutions
   */
  void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkOpenGLShaderProperty* p);

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);

  /**
   *  RenderPass string replacements on shader templates called from
   *  ReplaceShaderValues.
   */
  void ReplaceShaderCustomUniforms(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkOpenGLShaderProperty* p);
  void ReplaceShaderBase(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderTermination(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderShading(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderCompute(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderCropping(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderClipping(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderMasking(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderRTT(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderRenderPass(
    std::map<vtkShader::Type, vtkShader*>& shaders, vtkVolume* vol, bool prePass);

  /**
   *  Update parameters from RenderPass
   */
  void SetShaderParametersRenderPass();

  /**
   *  Caches the vtkOpenGLRenderPass::RenderPasses() information.
   *  Note: Do not dereference the pointers held by this object. There is no
   *  guarantee that they are still valid!
   */
  vtkNew<vtkInformation> LastRenderPassInfo;
  ///@}

  double ReductionFactor;
  int CurrentPass;

public:
  using VolumeInput = vtkVolumeInputHelper;
  using VolumeInputMap = std::map<int, vtkVolumeInputHelper>;
  VolumeInputMap AssembledInputs;

private:
  class vtkInternal;
  vtkInternal* Impl;

  friend class vtkVolumeTexture;

  vtkOpenGLGPUVolumeRayCastMapper(const vtkOpenGLGPUVolumeRayCastMapper&) = delete;
  void operator=(const vtkOpenGLGPUVolumeRayCastMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLGPUVolumeRayCastMapper_h
