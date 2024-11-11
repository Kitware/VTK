// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePointCloudMapper_h
#define vtkWebGPUComputePointCloudMapper_h

#include "vtkCallbackCommand.h" // for listening on camera & polydata changes
#include "vtkPolyData.h"        // for the poyldata that is going to be rendered
#include "vtkPolyDataMapper.h"
#include "vtkRenderingWebGPUModule.h" // for the module export macro
#include "vtkSmartPointer.h"          // for smart pointers

class vtkWebGPURenderer;
class vtkWebGPURenderWindow;
class vtkWebGPUComputePipeline;
class vtkWebGPUComputePass;
class vtkWebGPUComputeBuffer;
class vtkWebGPUPointCloudMapperInternals;

VTK_ABI_NAMESPACE_BEGIN

/**
 * The point cloud renderer uses WebGPU compute shaders to render the point cells of a polydata onto
 * the framebuffer of a given WebGPURenderer.
 *
 * The implementation is based on the paper from Shütz et. al:
 * https://www.cg.tuwien.ac.at/research/publications/2021/SCHUETZ-2021-PCC/
 *
 * Only the basic version presented in the paper has been implemented (described as 'atomicMin' in
 * the paper), WebGPU not having the required features (warp-level intrinsics most notably) at the
 * time of implementing (august 2024). Writing to a depth buffer from a compute shader (depth buffer
 * texture storage) is also a very ill-supported feature (~1% of devices). One option could be to
 * copy the point depth buffer to the depth buffer of the render window with a CopyBufferToTexture
 * operation but copying to the depth aspect of a depth buffer isn't supported by Dawn yet.
 * Our solution is thus to employ a fragment shader pass with the point depth buffer bound to it.
 * The shader can then write to the depth buffer.
 *
 * The renderer support point colors through point scalars.
 *
 * This implementation will run into issues if WebGPU uses the OpenGL backend. This is because
 * OpenGL has its texture coordinate origin (0, 0) at the bottom left corner of the texture whereas
 * the shader of the point cloud mapper expects the (0, 0) of the texture to be in the top left
 * corner. With OpenGL, this will cause reads into the depth buffer to be reversed along the Y axis
 * and incorrect depth values will be read --> invalid depth handling --> issues will arise with
 * multiple point clouds per renderer or a point cloud mixed with regular triangle based geometry.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputePointCloudMapper : public vtkPolyDataMapper
{
public:
  vtkTypeMacro(vtkWebGPUComputePointCloudMapper, vtkPolyDataMapper);
  static vtkWebGPUComputePointCloudMapper* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Renders the given actor with the given renderer.
   *
   * @warning: In its current state, the vtkWebGPUComputePointCloudMapper does not support rendering
   * the actors of two different renderers. This means that calling RenderPiece() once with a first
   * vtkRenderer and then calling RenderPiece() again with another vtkRenderer will yield incorrect
   * results. Two mappers must be used in that case
   */
  void RenderPiece(vtkRenderer* renderer, vtkActor* act) override;

protected:
  /**
   * Creates the compute passes and sets up the observers
   */
  vtkWebGPUComputePointCloudMapper();
  ~vtkWebGPUComputePointCloudMapper() override;

private:
  vtkWebGPUComputePointCloudMapper(const vtkWebGPUComputePointCloudMapper&) = delete;
  void operator=(const vtkWebGPUComputePointCloudMapper&) = delete;
  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  void ComputeBounds() override;

  vtkSmartPointer<vtkWebGPUPointCloudMapperInternals> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
