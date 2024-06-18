// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePipeline_h
#define vtkWebGPUComputePipeline_h

#include "vtkObject.h"
#include "vtkWGPUContext.h"                    // for requesting device / adapter
#include "vtkWebGPUComputeBuffer.h"            // for compute buffers used by the pipeline
#include "vtkWebGPUComputeRenderBuffer.h"      // for compute render buffers used by the pipeline
#include "vtkWebGPUInternalsComputePipeline.h" // for "pimpl"

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
  int AddBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer);

  /**
   * Adds a render buffer to the pipeline. A render buffer can be obtained from
   * vtkWebGPUPolyDataMapper::AcquirePointXXXXRenderBuffer() or
   * vtkWebGPUPolyDataMapper::AcquireCellXXXXRenderBuffer()
   */
  void AddRenderBuffer(vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer);

  /**
   * Resizes a buffer of the pipeline.
   *
   * @warning: After the resize, the data of the buffer is undefined and should be updated by a call
   * to UpdateBufferData()
   */
  void ResizeBuffer(int bufferIndex, vtkIdType newByteSize);

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
   * Returns true if the buffer is correct.
   * If the buffer is incorrect, returns false.
   *
   * This function makes various checks such as whether the group and binding of the buffer are
   * correct and also if the group/binding combination is unique or not, ...
   */
  bool IsBufferValid(vtkWebGPUComputeBuffer* buffer, const char* bufferLabel);

  /**
   * Updates the data of a buffer.
   * The given data is expected to be at most the size of the buffer.
   * If N bytes are given to update but the buffer size is > N, only the first N bytes
   * will be updated, the rest will remain unchanged.
   * The data is immediately available to the GPU (no call to Update() is necessary for this call to
   * take effect)
   *
   * @note: This method can be used even if the buffer was initially configured with std::vector
   * data and the given data can safely be destroyed directly after calling this function.
   */
  template <typename T>
  void UpdateBufferData(int bufferIndex, const std::vector<T>& newData)
  {
    auto& internals = *this->Internals;

    if (!this->CheckBufferIndex(bufferIndex, std::string("UpdataBufferData")))
    {
      return;
    }

    vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = internals.Buffers[bufferIndex];
    vtkIdType byteSize = buffer->GetByteSize();
    vtkIdType givenSize = newData.size() * sizeof(T);

    if (givenSize > byteSize)
    {
      vtkLog(ERROR,
        "std::vector data given to UpdateBufferData with index "
          << bufferIndex << " is too big. " << givenSize
          << "bytes were given but the buffer is only " << byteSize
          << " bytes long. No data was updated by this call.");

      return;
    }

    wgpu::Buffer wgpuBuffer = internals.WGPUBuffers[bufferIndex];
    internals.Device.GetQueue().WriteBuffer(
      wgpuBuffer, 0, newData.data(), newData.size() * sizeof(T));
  }

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  template <typename T>
  void UpdateBufferData(int bufferIndex, vtkIdType byteOffset, const std::vector<T>& data)
  {
    auto& internals = *this->Internals;

    if (!this->CheckBufferIndex(bufferIndex, std::string("UpdataBufferData with offset")))
    {
      return;
    }

    vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = internals.Buffers[bufferIndex];
    vtkIdType byteSize = buffer->GetByteSize();
    vtkIdType givenSize = data.size() * sizeof(T);

    if (givenSize + byteOffset > byteSize)
    {
      vtkLog(ERROR,
        "std::vector data given to UpdateBufferData with index "
          << bufferIndex << " and offset " << byteOffset << " is too big. " << givenSize
          << "bytes and offset " << byteOffset << " were given but the buffer is only " << byteSize
          << " bytes long. No data was updated by this call.");

      return;
    }

    wgpu::Buffer wgpuBuffer = internals.WGPUBuffers[bufferIndex];
    internals.Device.GetQueue().WriteBuffer(
      wgpuBuffer, byteOffset, data.data(), data.size() * sizeof(T));
  }

  /**
   * Updates the data of a buffer with a vtkDataArray.
   * The given data is expected to be at most the size of the buffer.
   * If N bytes are given to update but the buffer size is > N, only the first N bytes
   * will be updated, the rest will remain unchanged.
   * The data is immediately available to the GPU (no call to Update() is necessary for this call
   * to take effect)
   *
   * @note: This method can be used even if the buffer is configured for using data from an
   * std::vector. The given data can safely be destroyed directly after calling this function.
   */
  void UpdateBufferData(int bufferIndex, vtkDataArray* newData);

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  void UpdateBufferData(int bufferIndex, vtkIdType byteOffset, vtkDataArray* newData);

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
  void Update();

protected:
  /**
   * Constructor that initializes the device and adapter
   */
  vtkWebGPUComputePipeline();
  ~vtkWebGPUComputePipeline() = default;

private:
  friend class vtkWebGPURenderer;
  friend class vtkWebGPURenderWindow;
  friend class vtkWebGPUInternalsComputePipeline;

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
   * Checks if a given index is suitable for indexing this->Buffers. Logs an error if the index is
   * negative or greater than the number of buffer of the pipeline. The callerFunctionName parameter
   * is using to give more information on what function used an invalid buffer index
   *
   * Returns true if the buffer index is valid, false if it's not.
   */
  bool CheckBufferIndex(int bufferIndex, const std::string& callerFunctionName);

  /**
   * Makes some various (and obvious) checks to ensure that the buffer is ready to be created.
   *
   * Returns true if the buffer is correct.
   * If the buffer is incorrect, returns false and logs the error with the ERROR verbosity
   */
  bool CheckBufferCorrectness(vtkWebGPUComputeBuffer* buffer, const char* bufferLabel);

  /**
   * Destroys and recreates a buffer with the given newByteSize
   * Only the wgpu::Buffer object is recreated so the binding/group of the group doesn't change
   */
  void RecreateBuffer(int bufferIndex, vtkIdType newByteSize);

  /**
   * After recreating a wgpu::Buffer, the bind group entry (and the bind group) will need to be
   * updated. This
   */
  void RecreateBufferBindGroup(int bufferIndex);

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

  std::unique_ptr<vtkWebGPUInternalsComputePipeline> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
