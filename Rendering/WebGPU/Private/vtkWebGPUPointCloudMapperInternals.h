// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUPointCloudMapperInternals_h
#define vtkWebGPUPointCloudMapperInternals_h

#include "vtkObject.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtkSmartPointer.h"
#include "vtkWebGPUComputePointCloudMapper.h"
#include "vtk_wgpu.h"

class vtkPolyData;
class vtkRenderer;
class vtkWebGPUComputeBuffer;
class vtkWebGPUComputePass;
class vtkWebGPUComputePipeline;
class vtkWebGPUComputePointCloudMapper;
class vtkWebGPURenderWindow;

VTK_ABI_NAMESPACE_BEGIN

/**
 * Internal implementation details of vtkWebGPUPointCloudMapper
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUPointCloudMapperInternals : public vtkObject
{
public:
  static vtkWebGPUPointCloudMapperInternals* New();
  vtkTypeMacro(vtkWebGPUPointCloudMapperInternals, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetMapper(vtkWebGPUComputePointCloudMapper* mapper);

  /**
   * Structure that contains the wgpu objects necessary for the use of the render pipeline that
   * copies the depth buffer of a point cloud mapper to the depth buffer of a render window (using a
   * simple fragment shader that reads the depth from the point depth buffer and writes it to the
   * depth buffer of the render window)
   */
  struct CopyDepthBufferRenderPipeline
  {
    wgpu::BindGroup BindGroup = nullptr;
    wgpu::RenderPipeline Pipeline = nullptr;
    wgpu::Buffer FramebufferWidthUniformBuffer = nullptr;
  };

protected:
  vtkWebGPUPointCloudMapperInternals();
  ~vtkWebGPUPointCloudMapperInternals() override;

private:
  vtkWebGPUPointCloudMapperInternals(const vtkWebGPUPointCloudMapperInternals&) = delete;
  void operator=(const vtkWebGPUPointCloudMapperInternals&) = delete;

  friend class vtkWebGPUComputePointCloudMapper;

  /**
   * Returns the WebGPU Render window of the given renderer. Nullptr if something goes wrong
   */
  vtkWebGPURenderWindow* GetRendererRenderWindow(vtkRenderer* renderer);

  /**
   * Creates the compute pipeline and sets up the compute passes for rendering point clouds
   */
  void Initialize(vtkRenderer* renderer);

  /**
   * Copies the depth buffer that contains the depth of the points back to the depth buffer of the
   * render window.
   *
   * This is necessary because there is currently (august 2024) no way for WebGPU to copy from a
   * custom depth buffer (as 'pointDepthBuffer' is in the point cloud mapper) to the depth buffer of
   * a render window framebuffer. The solution is to use a fragment shader that reads from the
   * buffer and writes to the depth buffer (using the rasterizer pipeline)
   */
  void UpdateRenderWindowDepthBuffer(vtkRenderer* renderer);

  /**
   * Creates the render pipeline for copying the point depth buffer to the render window's depth
   * buffer using a fragment shader
   */
  void CreateCopyDepthBufferRenderPipeline(vtkWebGPURenderWindow* wgpuRenderWindow);

  /**
   * Dispatches the render pass that copies the point depth buffer to the depth buffer of the render
   * window
   */
  void CopyDepthBufferToRenderWindow(vtkWebGPURenderWindow* wgpuRenderWindow);

  /**
   * Udates various attributes of this mapper if necessary.
   *
   * One example is the size of the point depth buffer used: if the RenderWindow of the given
   * renderer isn't the same size as the size of the current point depth buffer, the point depth
   * buffer will be resized
   */
  void Update(vtkRenderer* renderer);

  /**
   * Sets the device of the render window of the given renderer on the compute pipeline
   */
  void UseRenderWindowDevice(vtkRenderer* renderer);

  /**
   * Resizes the textures used by the point cloud mapper to the size of the render window
   */
  void ResizeToRenderWindow(vtkRenderer* renderer);

  /**
   * Sets up the compute pass that copies the depth buffer of the render window
   */
  void InitializeDepthCopyPass(vtkRenderer* renderer);

  /**
   * Sets up the compute pass that renders the points
   */
  void InitializePointRenderPass(vtkRenderer* renderer);

  /**
   * Resizes and uploads the points data to be rendered from the current CachedInput.
   *
   * This function also reconfigures the render compute pass so it uses enough workgroups to cover
   * all the points
   */
  void UploadPointsToGPU();

  /**
   * Resizes and uploads the point colors from the current CachedInput.
   *
   * The only color format supported are point scalars with an unsigned char format and 4
   * components. If the point scalars of the given polydata do not respect that format, no colors
   * will be uploaded
   */
  void UploadColorsToGPU();

  /**
   * Updates the view projection matrix buffer with the view projection matrix data of the matrix of
   * the WebGPURenderer this compute point cloud renderer is rendering to
   */
  void UploadCameraVPMatrix(vtkRenderer* renderer);

  // Whether or not the compute pipeline has been initialized
  bool Initialized = false;

  // Compute pipeline for the point cloud rendering
  vtkSmartPointer<vtkWebGPUComputePipeline> ComputePipeline;

  // Compute pass that copies the depth buffer of the render window into the custom depth
  // buffer for rendering the points
  vtkSmartPointer<vtkWebGPUComputePass> CopyDepthPass;
  // Compute pass that renders the points to the framebuffer of the render window of the
  // WebGPURenderer
  vtkSmartPointer<vtkWebGPUComputePass> RenderPointsPass;

  // Custom depth buffer for the render of the points
  vtkSmartPointer<vtkWebGPUComputeBuffer> PointDepthBuffer;
  // Index of the buffer that contains the point data in the render point pass
  int PointBufferIndex = -1;
  // Index of the buffer that holds the colors of the points in float format in the point render
  // pass
  int PointColorBufferIndex = -1;

  // Custom depth buffer that contains the depth of the points after they've been rendered
  int PointDepthBufferIndex = -1;
  // Index of the view-projection matrix buffer in the render point pass
  int CameraVPBufferIndex = -1;
  // Index of the framebuffer in the render point pass
  int FrameBufferRenderTextureIndex = -1;

  // The renderer culling pass always calls GetBounds() on the mappers. We use this opportunity to
  // cache the polyData input so that we can reuse it later without having to call on the expensive
  // GetInput() function
  vtkPolyData* CachedInput = nullptr;
  // MTime of the last points we uploaded to the GPU
  vtkMTimeType LastPointsMTime = 0;
  // MTime of the last point data (for point colors) we uploaded to the GPU
  vtkMTimeType LastPointDataMTime = 0;

  // Contains the wgpu objects for refereing to the render pipeline that copies the point depth
  // buffer to the depth buffer of the render window
  CopyDepthBufferRenderPipeline CopyDepthBufferPipeline;

  // vtkWebGPUComputePointCloudMapper whose internals this instance is
  vtkWebGPUComputePointCloudMapper* ParentMapper = nullptr;
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBindGroupInternals.h
