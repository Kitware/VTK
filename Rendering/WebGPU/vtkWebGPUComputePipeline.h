// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePipeline_h
#define vtkWebGPUComputePipeline_h

#include "vtkObject.h"
#include "vtkWebGPUComputePass.h"   // for the list of compute passes held by this pipeline
#include "vtkWebGPUConfiguration.h" // for requesting device / adapter
#include "vtk_wgpu.h"               // for webgpu

#include <unordered_map> // for the registered buffers / textures

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderWindow;
class vtkWebGPURenderer;

/**
 * A compute pipeline is the orchestrator of a collection of compute passes.
 *
 * Computes passes are configured and added to the pipeline.
 *
 * After commands have been given to compute passes (Dispatch(), ReadBufferFromGPU(), ...), the
 * Update() function of the compute pipeline can be called which will in turn execute all the
 * commands that have been given so far to all the compute passes of this pipeline. The commands
 * execute in the order they were given to the compute passes but is independent of the order of the
 * compute passes themselves. This means that the Dispatch() command of the compute pass 2 can
 * execute before the Dispatch() command of the compute pass 1 if you called Pass2->Dispatch()
 * before Pass1->Dispatch().
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputePipeline : public vtkObject
{
public:
  vtkTypeMacro(vtkWebGPUComputePipeline, vtkObject);
  static vtkWebGPUComputePipeline* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the label of the compute pipeline.
   * This label will be printed along with error/warning logs
   * to help with debugging
   */
  vtkGetMacro(Label, std::string&);
  vtkSetMacro(Label, std::string);
  ///@}

  void SetWGPUConfiguration(vtkWebGPUConfiguration* config);
  vtkGetSmartPointerMacro(WGPUConfiguration, vtkWebGPUConfiguration);

  /**
   * Adds a compute pass to this pipeline
   */
  vtkSmartPointer<vtkWebGPUComputePass> CreateComputePass();

  /**
   * Returns the list of the compute passes that have been added to this compute pipeline
   */
  const std::vector<vtkSmartPointer<vtkWebGPUComputePass>>& GetComputePasses() const;

  /**
   * Dispatch all the compute passes of this compute pipeline in the order they were added
   */
  void DispatchAllPasses();

  /**
   * Executes WebGPU commands and callbacks. This method needs to be called at some point to allow
   * for the execution of WebGPU commands that have been submitted so far. A call to Dispatch() or
   * ReadBufferFromGPU() without a call to Update() will have no effect. Calling Dispatch() and then
   * ReadBufferFromGPU() and then Update() is valid. You do not need to call Update() after every
   * pipeline call. It can be called only once "at the end".
   */
  void Update();

  /**
   * Releases the resources used by this pipeline: all compute passes & registered buffers/textures.
   *
   * The WebGPU Configuration of this pipeline will be reset by this function and a call to
   * SetWGPUConfiguration() will be necessary to create new compute passes on this pipeline.
   */
  void ReleaseResources();

protected:
  vtkWebGPUComputePipeline();
  ~vtkWebGPUComputePipeline() override;

private:
  friend class vtkWebGPUComputePassInternals;
  friend class vtkWebGPUCellToPrimitiveConverter;
  friend class vtkWebGPURenderWindow;
  friend class vtkWebGPURenderer;

  vtkWebGPUComputePipeline(const vtkWebGPUComputePipeline&) = delete;
  void operator=(const vtkWebGPUComputePipeline&) = delete;

  /**
   * Registers a new buffer created for a compute pass in this pipeline so that it can be reused by
   * other compute passes
   */
  void RegisterBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer);

  /**
   * Registers a new texture created for a compute pass in this pipeline so that it can be reused by
   * other compute passes
   */
  void RegisterTexture(vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture wgpuTexture);

  ///@{
  /**
   * Checks whether the given compute buffer/texture/texture view has already been registered in
   * this pipeline. If so, returns true and sets the parameter to the WebGPU object registered in
   * this pipeline.
   *
   * Returns false and leaves the parameter unchanged otherwise.
   */
  bool GetRegisteredBuffer(
    vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer& wgpuBuffer);

  bool GetRegisteredTexture(
    vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture& wgpuTexture);
  ///@}

  /**
   * Makes sure that the WGPUConfiguration of this pipeline is initialized. If it is not, this
   * method does the initialization.
   */
  void EnsureConfigured();

  /**
   * WebGPU adapter and device used by this pipeline (and all the compute passes contained in this
   * pipeline)
   */
  vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration = nullptr;

  /**
   * List of all the passes contained in this pipeline
   */
  std::vector<vtkSmartPointer<vtkWebGPUComputePass>> ComputePasses;

  /**
   * A map of the WebGPU buffers/textures/texture views used by all the compute passes of this
   * pipeline.
   *
   * Used when the same vtkWebGPUComputeBuffer is added to two different vtkWebGPUComputePasses of
   * the same pipeline. This is a case where the two passes want to share the buffer so it has to be
   * created on the device only once. The buffer will be registered when added to the first compute
   * pass. When added to the second compute pass, the list of registered buffers will be checked and
   * because the buffer that we're trying to add is found as registered, it will not be created on
   * the device again and will be reused for the second pass.
   */
  std::unordered_map<vtkSmartPointer<vtkWebGPUComputeBuffer>, wgpu::Buffer> RegisteredBuffers;
  std::unordered_map<vtkSmartPointer<vtkWebGPUComputeTexture>, wgpu::Texture> RegisteredTextures;

  // Label used for debugging
  std::string Label = "WebGPU Compute Pipeline";
};

VTK_ABI_NAMESPACE_END

#endif
