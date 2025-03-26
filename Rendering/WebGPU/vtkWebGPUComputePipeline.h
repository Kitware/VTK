// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePipeline_h
#define vtkWebGPUComputePipeline_h

#include "vtkObject.h"
#include "vtkWGPUContext.h"               // for requesting device / adapter
#include "vtkWebGPUComputeBuffer.h"       // for compute buffers used by the pipeline
#include "vtkWebGPUComputeRenderBuffer.h" // for compute render buffers used by the pipeline

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderWindow;
class vtkWebGPURenderer;

/**
 * This class is an abstraction for offloading computation from the CPU onto the GPU using WebGPU
 * compute shaders.
 *
 * The basic usage of a pipeline outside a rendering pipeline is:
 *  - Create a pipeline
 *  - Set its shader source code
 *  - Set its shader entry point
 *  - Create the vtkWebGPUComputeBuffers that contain the data manipulated by the compute shader
 *  - Add the buffers to the pipeline
 *  - Set the number of workgroups
 *  - Dispatch the compute shader
 *  - Update()
 *  - ReadBufferFromGPU() to make results from the GPU available to the CPU
 *
 * Integrated into a rendering pipeline, the only difference in the usage of the class is going to
 * be the creation of the buffers. You will not create the vtkWebGPUComputeBuffer yourself but
 * rather acquire one (or many) by calling AcquirePointAttributeComputeRenderBuffer() on a
 * vtkWebGPURenderer. The returned buffers can then be added to the pipeline with AddRenderBuffer().
 * Other steps are identical.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputePipeline : public vtkObject
{
public:
  vtkTypeMacro(vtkWebGPUComputePipeline, vtkObject);
  static vtkWebGPUComputePipeline* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /*
   * Callback called when the asynchronous mapping of a buffer is done
   * and data ready to be copied.
   * This callback takes two parameters:
   * - A first void pointer to the data mapped from the GPU ready to be copied
   * - A second void pointer pointing to user data, which can essentially be anything
   *      needed by the callback to copy the data to the CPU
   */
  using MapAsyncCallback = std::function<void(const void*, void*)>;

  ///@{
  /**
   * Set/get the WGSL source of the shader
   */
  vtkGetMacro(ShaderSource, std::string);
  vtkSetMacro(ShaderSource, std::string);
  ///@}

  /**
   * Sets the WGSL shader source that this compute pipeline will use by reading it from a file
   */
  void SetShaderSourceFromPath(const std::string& shaderFilePath);

  ///@{
  /**
   * Set/get the entry (name of the function) of the WGSL compute shader
   */
  vtkGetMacro(ShaderEntryPoint, std::string);
  vtkSetMacro(ShaderEntryPoint, std::string);
  ///@}

  ///@{
  /**
   * Set/get the label of the compute pipeline.
   * This label will be printed along with error/warning logs
   * to help with debugging
   */
  vtkGetMacro(Label, std::string);
  void SetLabel(const std::string& label);
  ///@}

  /**
   * Adds a buffer to the pipeline and uploads its data to the device.
   *
   * Returns the index of the buffer that can for example be used as input to the
   * ReadBufferFromGPU function
   */
  int AddBuffer(vtkWebGPUComputeBuffer* buffer);

  /**
   * Adds a render buffer to the pipeline. A render buffer can be obtained from
   * vtkWebGPUPolyDataMapper::AcquirePointXXXXRenderBuffer() or
   * vtkWebGPUPolyDataMapper::AcquireCellXXXXRenderBuffer()
   */
  void AddRenderBuffer(vtkWebGPUComputeRenderBuffer* renderBuffer);

  /*
   * This function maps the buffer, making it accessible to the CPU. This is
   * an asynchronous operation, meaning that the given callback will be called
   * when the mapping is done.
   *
   * The buffer data can then be read from the callback and stored
   * in a buffer passed in via the userdata pointer for example
   */
  void ReadBufferFromGPU(
    int bufferIndex, vtkWebGPUComputePipeline::MapAsyncCallback callback, void* userdata);

  /**
   * Makes some various (and obvious) checks to ensure that the buffer is ready to be created.
   *
   * Returns true if the buffer is correct.
   * If the buffer is incorrect, returns false
   */
  bool IsBufferValid(vtkWebGPUComputeBuffer* buffer, const char* bufferLabel);

  ///@{
  /*
   * Set/get the number of workgroups in each dimension that are used by each Dispatch() call.
   */
  void SetWorkgroups(int groupsX, int groupsY, int groupsZ);
  ///@}

  /**
   * Dispatch the compute shader with (X, Y, Z) = (groupX, groupsY, groupZ) groups
   */
  void Dispatch();

  /**
   * Executes WebGPU commands and callbacks. This method needs to be called at some point to allow
   * for the execution of WebGPU commands that have been submitted so far. A call to Dispatch() or
   * ReadBufferFromGPU() without a call to Update() will have no effect. Calling Dispatch() and then
   * ReadBufferFromGPU() and then Update() is valid. You do not need to call Update() after every
   * pipeline call. It can be called only once "at the end".
   */
  void Update() { vtkWGPUContext::WaitABit(); }

protected:
  /**
   * Constructor that initializes the device and adapter
   */
  vtkWebGPUComputePipeline();
  ~vtkWebGPUComputePipeline() = default;

private:
  friend class vtkWebGPURenderer;
  friend class vtkWebGPURenderWindow;

  vtkWebGPUComputePipeline(const vtkWebGPUComputePipeline&) = delete;
  void operator=(const vtkWebGPUComputePipeline&) = delete;

  /*
   * Sets the adapter. Useful to reuse an already existing adapter.
   */
  void SetAdapter(wgpu::Adapter adapter);

  /*
   * Sets the device. Useful to reuse an already existing device.
   */
  void SetDevice(wgpu::Device device);

  /**
   * Binds the buffer to the pipeline at the WebGPU level.
   * To use once the buffer has been properly set up.
   */
  void SetupRenderBuffer(vtkWebGPUComputeRenderBuffer* renderBuffer);

  // WGSL source code of the shader
  std::string ShaderSource;
  // Name of the main function of the compute shader
  std::string ShaderEntryPoint;
  // Label used for debugging
  std::string Label = "VTK Compute pipeline";

  struct ComputePipelineInternals;
  std::unique_ptr<ComputePipelineInternals> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
