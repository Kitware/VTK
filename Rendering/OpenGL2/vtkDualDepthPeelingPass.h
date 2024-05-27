// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkDualDepthPeelingPass
 * @brief   Implements the dual depth peeling algorithm.
 *
 *
 * Dual depth peeling is an augmentatation of the standard depth peeling
 * algorithm that peels two layers (front and back) for each render pass. The
 * technique is described in "Order independent transparency with dual depth
 * peeling" (February 2008) by L. Bavoil, K. Myers.
 *
 * This algorithm has been extended to also peel volumetric data along with
 * translucent geometry. To use this feature, set VolumetricPass to an
 * appropriate RenderPass (usually vtkVolumetricPass).
 *
 * The pass occurs in several stages:
 *
 * 1. Copy the current (opaque geometry) depth buffer into a texture.
 * 2. Initialize the min-max depth buffer from the opaque depth texture and the
 *    translucent geometry.
 * 3. Peel the nearest and farthest fragments:
 * 3a. Blend fragments that match the nearest depth of the min-max depth buffer
 *     into the front buffer.
 * 3b. Write the far depth fragments into a temporary buffer.
 * 3c. Extract the next set of min/max depth values for the next peel.
 * 3d. Blend the temporary far fragment texture (3b) into an accumulation
 *     texture.
 * 3e. Go back to 3a and repeat until the maximum number of peels is met, or
 *     the desired occlusion ratio is satisfied.
 * 4. If the occlusion ratio != 0 (i.e. we hit the maximum number of peels
 *    before finishing), alpha blend the remaining fragments in-between the
 *    near and far accumulation textures.
 * 5. Blend all accumulation buffers over the opaque color buffer to produce the
 *    final image.
 */

#ifndef vtkDualDepthPeelingPass_h
#define vtkDualDepthPeelingPass_h

#include "vtkDepthPeelingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

#include <array> // For std::array!

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkOpenGLVertexArrayObject;
class vtkRenderTimerLog;
class vtkShaderProgram;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkDualDepthPeelingPass
  : public vtkDepthPeelingPass
{
public:
  static vtkDualDepthPeelingPass* New();
  vtkTypeMacro(vtkDualDepthPeelingPass, vtkDepthPeelingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* s) override;
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Delegate for rendering the volumetric geometry, if needed.
   * It is usually set to a vtkVolumetricPass.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(VolumetricPass, vtkRenderPass)
  virtual void SetVolumetricPass(vtkRenderPass* volumetricPass);
  ///@}

  // vtkOpenGLRenderPass virtuals:
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;
  bool SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper, vtkProp* prop,
    vtkOpenGLVertexArrayObject* VAO = nullptr) override;
  vtkMTimeType GetShaderStageMTime() override;

protected:
  bool PostReplaceTranslucentShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop);
  bool PreReplaceVolumetricShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop);
  bool SetTranslucentShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper,
    vtkProp* prop, vtkOpenGLVertexArrayObject* VAO);
  bool SetVolumetricShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper,
    vtkProp* prop, vtkOpenGLVertexArrayObject* VAO);

  // Name the textures used by this render pass. These are indexes into
  // this->Textures
  enum TextureName
  {
    BackTemp = 0, // RGBA8 back-to-front peeling buffer
    Back,         // RGBA8 back-to-front accumulation buffer
    FrontA,       // RGBA8 front-to-back accumulation buffer
    FrontB,       // RGBA8 front-to-back accumulation buffer
    DepthA,       // RG32F min-max depth buffer
    DepthB,       // RG32F min-max depth buffer
    OpaqueDepth,  // Stores the depth map from the opaque passes

    NumberOfTextures
  };

  // The stages of this multipass render pass:
  enum ShaderStage
  {
    InitializingDepth,
    Peeling,
    AlphaBlending,

    NumberOfPasses,
    Inactive = -1,
  };

  enum PeelType
  {
    TranslucentPeel,
    VolumetricPeel
  };

  vtkDualDepthPeelingPass();
  ~vtkDualDepthPeelingPass() override;

  void SetCurrentStage(ShaderStage stage);
  vtkSetMacro(CurrentPeelType, PeelType);

  /**
   * Release all FBOs and textures.
   */
  void FreeGLObjects();

  /**
   * Render the translucent pass geometry, counting number of render calls.
   */
  void RenderTranslucentPass();

  /**
   * Render any volumetric geometry.
   */
  void RenderVolumetricPass();

  bool IsRenderingVolumes();

  /**
   * Allocate and configure FBOs and textures.
   */
  void Initialize(const vtkRenderState* state);

  ///@{
  /**
   * Initialize helpers.
   */
  void InitColorTexture(vtkTextureObject* tex, const vtkRenderState* s);
  void InitDepthTexture(vtkTextureObject* tex, const vtkRenderState* s);
  void InitOpaqueDepthTexture(vtkTextureObject* tex, const vtkRenderState* s);
  void InitFramebuffer(const vtkRenderState* s);
  ///@}

  /**
   * Bind and activate draw buffers.
   * @{
   */
  void ActivateDrawBuffer(TextureName id) { this->ActivateDrawBuffers(&id, 1); }
  template <size_t NumTextures>
  void ActivateDrawBuffers(const std::array<TextureName, NumTextures>& a)
  {
    this->ActivateDrawBuffers(a.data(), a.size());
  }
  void ActivateDrawBuffers(const TextureName* ids, size_t numTextures);
  /**@}*/

  /**
   * Fill textures with initial values, bind the framebuffer.
   */
  void Prepare();

  void InitializeOcclusionQuery();
  void CopyOpaqueDepthBuffer();
  void InitializeDepth();

  void PeelVolumesOutsideTranslucentRange();

  bool PeelingDone();

  /**
   * Render the scene to produce the next set of peels.
   */
  void Peel();

  // Depending on whether we're handling volumes or not, we'll initialize the
  // front destination buffer by either clearing it or copying the last peel's
  // output into it.
  void PrepareFrontDestination();
  void ClearFrontDestination();
  void CopyFrontSourceToFrontDestination();

  void InitializeTargetsForTranslucentPass();
  void InitializeTargetsForVolumetricPass();

  void PeelTranslucentGeometry();
  void PeelVolumetricGeometry();

  void BlendBackBuffer();

  void StartTranslucentOcclusionQuery();
  void EndTranslucentOcclusionQuery();

  void StartVolumetricOcclusionQuery();
  void EndVolumetricOcclusionQuery();

  /**
   * Swap the src/dest render targets:
   */
  void SwapFrontBufferSourceDest();
  void SwapDepthBufferSourceDest();

  void Finalize();

  void AlphaBlendRender();

  void BlendFinalImage();
  void DeleteOcclusionQueryIds();

  vtkRenderTimerLog* Timer;
  vtkRenderPass* VolumetricPass;
  const vtkRenderState* RenderState;

  vtkOpenGLQuadHelper* CopyColorHelper;
  vtkOpenGLQuadHelper* CopyDepthHelper;
  vtkOpenGLQuadHelper* BackBlendHelper;
  vtkOpenGLQuadHelper* BlendHelper;

  vtkTextureObject* Textures[NumberOfTextures];

  TextureName FrontSource;      // The current front source buffer
  TextureName FrontDestination; // The current front destination buffer
  TextureName DepthSource;      // The current depth source buffer
  TextureName DepthDestination; // The current depth destination buffer

  ShaderStage CurrentStage;
  PeelType CurrentPeelType;
  vtkTimeStamp CurrentStageTimeStamp;

  bool LastPeelHadVolumes;
  int CurrentPeel;
  unsigned int TranslucentOcclusionQueryId;
  unsigned int TranslucentWrittenPixels;
  unsigned int VolumetricOcclusionQueryId;
  unsigned int VolumetricWrittenPixels;
  unsigned int OcclusionThreshold;

  int TranslucentRenderCount; // Debug info, counts number of geometry passes.
  int VolumetricRenderCount;  // Debug info, counts number of volumetric passes.

  // Cached state:
  bool SaveScissorTestState;
  int CullFaceMode;
  bool CullFaceEnabled;
  bool DepthTestEnabled;

private:
  vtkDualDepthPeelingPass(const vtkDualDepthPeelingPass&) = delete;
  void operator=(const vtkDualDepthPeelingPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDualDepthPeelingPass_h
