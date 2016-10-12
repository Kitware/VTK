/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDualDepthPeelingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDepthPeelingPass.h"

class vtkFrameBufferObject2;
class vtkOpenGLBufferObject;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkDualDepthPeelingPass:
    public vtkDepthPeelingPass
{
public:
  static vtkDualDepthPeelingPass* New();
  vtkTypeMacro(vtkDualDepthPeelingPass, vtkDepthPeelingPass)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void Render(const vtkRenderState *s);
  virtual void ReleaseGraphicsResources(vtkWindow *w);

  // vtkOpenGLRenderPass virtuals:
  virtual bool ReplaceShaderValues(std::string &vertexShader,
                                   std::string &geometryShader,
                                   std::string &fragmentShader,
                                   vtkAbstractMapper *mapper,
                                   vtkProp *prop);
  virtual bool SetShaderParameters(vtkShaderProgram *program,
                                   vtkAbstractMapper *mapper, vtkProp *prop);
  virtual vtkMTimeType GetShaderStageMTime();

protected:

  // Name the textures used by this render pass. These are indexes into
  // this->Textures
  enum TextureName
  {
    BackTemp = 0, // RGBA8 back-to-front peeling buffer
    Back, // RGBA8 back-to-front accumulation buffer
    FrontA, // RGBA8 front-to-back accumulation buffer
    FrontB, // RGBA8 front-to-back accumulation buffer
    DepthA, // RG32F min-max depth buffer
    DepthB, // RG32F min-max depth buffer
    OpaqueDepth, // Stores the depth map from the opaque passes

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

  vtkDualDepthPeelingPass();
  ~vtkDualDepthPeelingPass();

  void SetCurrentStage(ShaderStage stage);

  /**
   * Release all FBOs and textures.
   */
  void FreeGLObjects();

  /**
   * Render the translucent pass geometry, counting number of render calls.
   */
  void RenderTranslucentPass();

  /**
   * Allocate and configure FBOs and textures.
   */
  void Initialize(const vtkRenderState *s);

  //@{
  /**
   * Initialize helpers.
   */
  void InitColorTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitDepthTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitOpaqueDepthTexture(vtkTextureObject *tex, const vtkRenderState *s);
  void InitFramebuffer(const vtkRenderState *s);
  //@}

  //@{
  /**
   * Fill textures with initial values, bind the framebuffer.
   */
  void Prepare();
  void InitializeOcclusionQuery();
  void CopyOpaqueDepthBuffer();
  void InitializeDepth();
  //@}

  bool PeelingDone();

  /**
   * Render the scene to produce the next set of peels.
   */
  void Peel();

  void InitializeTargets();

  void PeelRender();

  void BlendBackBuffer();
  void StartOcclusionQuery();
  void EndOcclusionQuery();

  /**
   * Swap the src/dest render targets:
   */
  void SwapTargets();

  void Finalize();

  void AlphaBlendRender();

  void BlendFinalImage();
  void DeleteOcclusionQueryId();

  const vtkRenderState *RenderState;

  vtkShaderProgram *CopyDepthProgram;
  vtkOpenGLVertexArrayObject *CopyDepthVAO;
  vtkOpenGLBufferObject *CopyDepthVBO;

  vtkShaderProgram *BackBlendProgram;
  vtkOpenGLVertexArrayObject *BackBlendVAO;
  vtkOpenGLBufferObject *BackBlendVBO;

  vtkShaderProgram *BlendProgram;
  vtkOpenGLVertexArrayObject *BlendVAO;
  vtkOpenGLBufferObject *BlendVBO;

  vtkFrameBufferObject2 *Framebuffer;
  vtkTextureObject *Textures[NumberOfTextures];

  TextureName FrontSource; // The current front source buffer
  TextureName FrontDestination; // The current front destination buffer
  TextureName DepthSource; // The current depth source buffer
  TextureName DepthDestination; // The current depth destination buffer

  ShaderStage CurrentStage;
  vtkTimeStamp CurrentStageTimeStamp;

  int CurrentPeel;
  unsigned int OcclusionQueryId;
  unsigned int WrittenPixels;
  unsigned int OcclusionThreshold;

  int RenderCount; // Debug info, counts number of geometry passes.

private:
  vtkDualDepthPeelingPass(const vtkDualDepthPeelingPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDualDepthPeelingPass&) VTK_DELETE_FUNCTION;
};

#endif // vtkDualDepthPeelingPass_h
