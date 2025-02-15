// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeOcclusionCuller_h
#define vtkWebGPUComputeOcclusionCuller_h

#include "vtkCallbackCommand.h" // for the bounds recomputed callback
#include "vtkCuller.h"
#include "vtkNew.h"                   // for new macro
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkSmartPointer.h"          // for the pipeline smart pointer
#include "vtkWebGPUComputePass.h"     // for compute passes
#include "vtkWebGPUComputePipeline.h" // for the member compute pipeline
#include "vtkWebGPURenderWindow.h"    // for the render window weak pointer member

VTK_ABI_NAMESPACE_BEGIN

/**
 * This culler does both frustum culling and occlusion culling.
 *
 * Occlusion culling culls props that are occluded (behind) other props and that are not visible to
 * the camera because of that.
 *
 * This implementation uses the two-pass hierarchical z-buffer approach.
 *
 * This approach projects the bound of the actors onto the viewport and compares the depth of the
 * projected region with a prepass depth buffer. This "prepass" depth buffer is built from the
 * objects that were rendered last frame. These objects offer a good approximation of what objects
 * will be visible this frame (assuming no brutal camera movements). To make the depth comparison
 * between the quad of the actor (projection of its bounding box on the viewport) more efficient, a
 * mipmap chain of the depth buffer is used. Without this mipmap chain, we would have to compare the
 * depth of all the pixels (there could be dozens to hundreds of thousands depending on the
 * screen-space size of the actor) of the projected bounding box of the actor against the depth
 * buffer which would be way too expensive. Using a mipmap chain allows us to choose the right
 * mipmap so that we only have to check a few (~4 +/- 2) pixels for the depth.
 *
 * Resource for a general overview of the algorithm:
 * https://medium.com/@mil_kru/two-pass-occlusion-culling-4100edcad501
 *
 * Resource for non-power of two mipmap calculation:
 * https://miketuritzin.com/post/hierarchical-depth-buffers/
 *
 * To use this culler, simply instantiate it and set its RenderWindow (after Initialize() has been
 * called on the RenderWindow()):
 *
 * vtkNew<vtkWebGPUComputeOcclusionCuller> webgpuOcclusionCuller;
 *
 * renWin->Initialize();
 * webgpuOcclusionCuller->SetRenderWindow(renWin)
 *
 * Then add it to the cullers of your renderer:
 *
 * renderer->GetCullers()->AddItem(webgpuOcclusionCuller);
 *
 * You may also want to remove the default CPU coverage culler of your renderer by calling :
 *
 * renderer->GetCullers()->RemoveAllItems();
 *
 * before adding the webgpuOcclusionCuller.
 *
 * @warning: In its current state, the compute occlusion culler is expected to fail if the WebGPU
 * backend used is OpenGL. This is because OpenGL has its texture coordinate origin (0, 0) at the
 * bottom left corner of the texture whereas the shader of the occlusion culler expects the (0, 0)
 * of the texture to be in the top left corner. With OpenGL, this will cause reads into the depth
 * buffer to be reversed along the Y axis and incorrect depth values will be read --> invalid
 * culling.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeOcclusionCuller : public vtkCuller
{
public:
  vtkTypeMacro(vtkWebGPUComputeOcclusionCuller, vtkCuller);
  static vtkWebGPUComputeOcclusionCuller* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sets which render window this occlusion culler is going to work on
   */
  void SetRenderWindow(vtkWebGPURenderWindow* webGpuRenderWindow);

  /**
   * Culls props and returns the number of props that still need to be rendered after the culling
   */
  double Cull(
    vtkRenderer* renderer, vtkProp** propList, int& listLength, int& initialized) override;

protected:
  vtkWebGPUComputeOcclusionCuller();
  ~vtkWebGPUComputeOcclusionCuller() override;

private:
  vtkWebGPUComputeOcclusionCuller(const vtkWebGPUComputeOcclusionCuller&) = delete;
  void operator=(const vtkWebGPUComputeOcclusionCuller&) = delete;
  /**
   * Sets up the first compute pass for copying the depth buffer of the render window to the first
   * mip level (level 0) of the hierarchical Z-buffer
   */
  void SetupDepthBufferCopyPass();

  /**
   * Sets up the compute pass for compute the max-mipmaps of the depth buffer
   */
  void SetupMipmapsPass();

  /**
   * Sets up the buffer used in the culling pass
   */
  void SetupCullingPass();

  /**
   * First render pass that renders the props that were visible last frame and that passed the
   * previous culling tests (if any). This pass is needed to fill the z-buffer.
   *
   * Return a list of the props that were not rendered for the first (filling the depth buffer) but
   * that need to be tested for culling (they are in the propList given to the Cull() call).
   */
  void FirstPassRender(vtkRenderer* renderer, vtkProp** propList, int listLength);

  /**
   * Copies the depth buffer filled by the rendering of the props of last frame into the mipmap
   * level 0 of the hierarchical z-buffer
   */
  void CopyDepthBuffer();

  /**
   * Computes the depth buffer max-mipmaps
   */
  void DepthMipmaps();

  /**
   * Culls the actors using the depth buffer mipmaps computed in the previous pass
   */
  void PropCulling(vtkRenderer* renderer, vtkProp** propList, int& listLength);

  /**
   * Reuploads the camera MVP matrix to its GPU buffer
   */
  void UpdateCameraMVPBuffer(vtkRenderer* renderer);

  /**
   * Resizes the various bounds buffers (inputBounds, outputBoundsIndices) and updates their data
   */
  void UpdateBoundsBuffers(vtkProp** propList, int listLength);

  /**
   * Adds the occlusion culling pipeline to the passed renderer so that the pipeline can reuse the
   * textures from the render window of the renderer
   */
  void AddOcclusionCullingPipelineToRenderer(vtkRenderer* renderer);

  /**
   * Sets-up the hierarchical z-buffer mipmapped texture
   */
  void CreateHierarchicalZBuffer();

  /**
   * Computes the number of mip levels for the given width and height and returns that number.
   *
   * The widths and heights of all the mip levels are also stored in MipmapWidths and MipmapHeights
   */
  int ComputeMipLevelsSizes(int width, int height);

  /**
   * Resizes the hi zbuffer texture to the given new width and height. The level 0 of the new
   * texture isn't initialized and the mipmaps are not immediately recomputed.
   */
  void ResizeHierarchicalZBuffer(uint32_t newWidth, uint32_t newHeight);

  /**
   * Recomputes the number of mipmaps of the hi-z buffer for the given newWidth and newHeight and
   * recreates all the texture views on the mipmap levels of the hi-z buffer
   */
  void ResizeHierarchicalZBufferMipmapsChain();

  /**
   * Creates the texture view of the hierarchical z buffer for copying the depth buffer of the
   * render window into it
   */
  void FinishSetupDepthCopyPass();

  /**
   * Creates the texture views for all the mipmap levels that are going to be needed to downsample
   * the depth buffer
   */
  void FinishSetupMipmapsPass();

  /**
   * Adds the texture view of the hierarchical z buffer to the culling pass
   */
  void FinishSetupCullingPass();

  /**
   * Callback to read the number of props that passed the culling test
   */
  static void ReadIndicesCountCallback(const void* mappedData, void* indicesCount);

  /**
   * Callback for reading the props that passed the culling test and store them in the propList of
   * the renderer (passed through the userdata of the callback).
   *
   * For a prop to be written to the propList that will be rendered, it needs to have passed the
   * culling test but also not having been rendered in the first pass (because rendering it twice is
   * useless so we're not adding a prop that was already rendered to the list of props that need to
   * be rendered).
   */
  static void FillObjectsToDrawCallback(const void* mappedData, void* data);

  /**
   * Callback that reads the indices of the props that were culled by the occlusion culling. These
   * props are then removed from the list of "props rendered last frame"  of the wgpu renderer
   */
  static void OutputIndicesCulledCallback(const void* mappedData, void* data);

  /**
   * Callback called when the render window of this occlusion culler is resized. This callback
   * resizes the hierarchical z-buffer
   */
  static void WindowResizedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  // Occlusion culling pipeline
  vtkSmartPointer<vtkWebGPUComputePipeline> OcclusionCullingPipeline;

  // Pass that copies the depth buffer of the render window into the mip level 0 of the hierarchical
  // z-buffer
  vtkSmartPointer<vtkWebGPUComputePass> DepthBufferCopyPass;

  // Index of the hierarchical z buffer in the depth buffer copy compute pass
  int HierarchicalZBufferTextureIndexCopyPass = -1;
  // Index of the hierarchical z buffer in the depth buffer mipmaps pass
  int HierarchicalZBufferTextureIndexMipmapsPass = -1;
  // Index of the hierarchical z buffer in the culling pass
  int HierarchicalZBufferTextureIndexCullingPass = -1;
  // All the views necessary for the computation of the depth buffer mipmaps
  std::vector<vtkSmartPointer<vtkWebGPUComputeTextureView>> HierarchicalZBufferMipmapViews;
  // Texture view indices within the DepthMipmapsPass compute pass
  std::vector<int> HierarchicalZBufferMipmapViewsIndices;
  // Total number of mipmaps of the hierarchical z buffer
  int HierarchicalZBufferMipmapCount = -1;
  // Widths of the successive mipmaps of the hierarchical z buffer
  std::vector<int> MipmapWidths;
  // Heights of the successive mipmaps of the hierarchical z buffer
  std::vector<int> MipmapHeights;
  // We need to keep the uniform buffer here because we can only create it once the RenderWindow has
  // set its Device to the pipeline of this occlusion culler. We will add the buffer to the pipeline
  // on the first frame, when we're sure that the Device has been set

  // Pass that downsamples the mipmap level 0 of the depth buffer into as many mipmap levels as
  // possible
  vtkSmartPointer<vtkWebGPUComputePass> DepthMipmapsPass;
  // Pass that does the culling of the actors against the hierarchial z-buffer
  vtkSmartPointer<vtkWebGPUComputePass> CullingPass;

  // Index of the hierarchical z buffer texture view in the culling pass
  int CullingPassHierarchicalZBufferView = -1;
  // Index of the bounds buffer in the culling pass
  int CullingPassBoundsBufferIndex = -1;
  // Index of the buffer that contains the indices of the props that passed the culling test in the
  // culling pass
  int CullingPassOutputIndicesBufferIndex = -1;
  // How many props that were sent to the culling shader passed the culling test
  int CullingPassOutputIndicesCountBufferIndex = -1;
  // Index of the buffer that contains the indices of the props that were culled. Needed to update
  // the visibility of the props in the PropsRendered array of the WGPURenderer
  int CullingPassOutputIndicesCulledBufferIndex = -1;
  // How many props were culled by the culling pass
  int CullingPassOutputIndicesCulledCountBufferIndex = -1;
  // Index of the buffer that contains the number of bounds to cull in the culling pass
  int CullingPassBoundsCountBufferIndex = -1;
  // Index of the buffer that contains the view projection matrix in the culling pass
  int CullingPassMVPMatrixBufferIndex = -1;

  // Structure passed to the callbacks for reading the results of the culling pass
  struct FillObjectsToDrawCallbackMapData
  {
    // How many props passed the culling test.
    // This is a pointer to the 'listLength' parameter of the Cull() method
    int* listLength = nullptr;
    // Prop list of the renderer that needs to be updated
    vtkProp** propList = nullptr;

    // WebGPU renderer
    // Used for accessing the 'rendered last frame' list
    vtkWebGPURenderer* renderer = nullptr;
  };

  // Structure passed to the callbacks for reading the results of the culling pass
  struct OutputIndicesCulledMapData
  {
    // Point to the renderer for removing the props that were culled from the list of "props
    // rendered last frame"
    vtkWebGPURenderer* renderer = nullptr;

    // Prop list of the renderer
    vtkProp** propList = nullptr;

    // How many props were culled
    int culledCount = -1;
  };

  // If this is the first frame, every object is going to be rendered b the FirstPassRender to fill
  // the z-buffer
  bool FirstFrame = true;
  // Whether or not we're done initializing the compute passes of this culler
  bool Initialized = false;
  // Render window whose depth buffer we're going to use for the culling
  vtkWeakPointer<vtkWebGPURenderWindow> WebGPURenderWindow = nullptr;

  // Callback command for when the render window that this occlusion culler is attached to is
  // resized
  vtkSmartPointer<vtkCallbackCommand> WindowResizedCallbackCommand;
};

VTK_ABI_NAMESPACE_END

#endif
