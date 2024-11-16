// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePassBufferStorageInternals_h
#define vtkWebGPUComputePassBufferStorageInternals_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"              // for smart pointers
#include "vtkWeakPointer.h"               // for weak pointers
#include "vtkWebGPUComputeBuffer.h"       // for compute buffers
#include "vtkWebGPUComputePass.h"         // for enum
#include "vtkWebGPUComputeRenderBuffer.h" // for compute render buffers
#include "vtkWebGPUConfiguration.h"       // for ivar
#include "vtk_wgpu.h"                     // for webgpu

VTK_ABI_NAMESPACE_BEGIN

/**
 * This class manages the creation/deletion/recreation/resizing/updating of compute buffers used by
 * a compute pass.
 *
 * A compute pass delegates calls that want to modify buffers to this class
 */
class vtkWebGPUComputePassBufferStorageInternals : public vtkObject
{
public:
  static vtkWebGPUComputePassBufferStorageInternals* New();
  vtkTypeMacro(vtkWebGPUComputePassBufferStorageInternals, vtkObject);

  /**
   * Enum used by the returned value of UpdateWebGPUBuffer() to indicate what operation was done
   * internally
   *
   * SUCCESS: The buffer was successfully updated
   *
   * UP_TO_DATE: The buffer was already up to date (the given wgpu::Buffer was already being used).
   * No-op.
   *
   * BUFFER_NOT_FOUND: The given buffer did not belong to this buffer storage. No-op.
   */
  enum UpdateBufferStatusCode
  {
    SUCCESS = 0,
    UP_TO_DATE,
    BUFFER_NOT_FOUND
  };

  /**
   * Sets the device that will be used by this buffer storage when creating buffers.
   *
   * This device must be the one used by the parent compute pass.
   */
  vtkSetSmartPointerMacro(ParentPassWGPUConfiguration, vtkWebGPUConfiguration);

  /**
   * Sets the compute pass that uses the buffers of this storage
   */
  void SetComputePass(vtkWeakPointer<vtkWebGPUComputePass> parentComputePass);

  /**
   * Adds a buffer to the pipeline and uploads its data to the device.
   *
   * Returns the index of the buffer that can for example be used as input to the
   * ReadBufferFromGPU() function.
   */
  int AddBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer);

  /**
   * Returns the wgpu::Buffer object for a buffer in this compute pass buffer storage given its
   * index
   */
  wgpu::Buffer GetWGPUBuffer(std::size_t bufferIndex);

  /**
   * Adds a render texture to the pipeline. A render texture can be obtained from
   * vtkWebGPUPolyDataMapper::AcquirePointXXXXRenderBuffer() or
   * vtkWebGPUPolyDataMapper::AcquireCellXXXXRenderBuffer()
   */
  void AddRenderBuffer(vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer);

  /**
   * Returns the size in bytes of a buffer
   */
  unsigned int GetBufferByteSize(std::size_t bufferIndex);

  /**
   * Resizes a buffer.
   *
   * @warning: After the resize, the data of the buffer is undefined and should be updated by a
   * call to UpdateBufferData()
   */
  void ResizeBuffer(std::size_t bufferIndex, vtkIdType newByteSize);

  /**
   * Destroys and recreates a buffer with the given newByteSize
   * Only the wgpu::Buffer object is recreated so the binding/group of the group doesn't change
   */
  void RecreateBuffer(std::size_t bufferIndex, vtkIdType newByteSize);

  /*
   * This function maps the buffer, making it accessible to the CPU. This is
   * an asynchronous operation, meaning that the given callback will be called
   * when the mapping is done.
   *
   * The buffer data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadBufferFromGPU(
    std::size_t bufferIndex, vtkWebGPUComputePass::BufferMapAsyncCallback callback, void* userdata);

  /**
   * Updates the wgpu::Buffer reference that a compute buffer is associated to.
   *
   * Useful when a compute buffer has been recreated and the associated wgpu::Buffer needs to be
   * updated with the newly created buffer.
   *
   * Also recreates the bind group of the buffer.
   *
   * Returns the status of the operation.
   * The index of the buffer that was modified (basically the index of 'buffer' within Buffers) is
   * stored in the outBufferIndex parameter.
   */
  UpdateBufferStatusCode UpdateWebGPUBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer,
    wgpu::Buffer wgpuBuffer, std::size_t& outBufferIndex);

  /**
   * Updates the data of a buffer.
   * The given data is expected to be at most the size of the buffer.
   * If N bytes are given to update but the buffer size is > N, only the first N bytes
   * will be updated, the rest will remain unchanged.
   * The data is immediately available to the GPU (no call to vtkWebGPUComputePipeline::Update() is
   * necessary for this call to take effect)
   *
   * @note: This method can be used even if the buffer was initially configured with std::vector
   * data and the given data can safely be destroyed directly after calling this function.
   *
   */
  void WriteBuffer(std::size_t bufferIndex, const void* bytes, std::size_t numBytes)
  {
    vtkWebGPUComputeBuffer* buffer = this->Buffers[bufferIndex];
    vtkIdType byteSize = buffer->GetByteSize();
    vtkIdType givenSize = numBytes;

    if (givenSize > byteSize)
    {
      vtkLog(ERROR,
        "void* data given to UpdateBufferData with index "
          << bufferIndex << " is too big. " << givenSize
          << "bytes were given but the buffer is only " << byteSize
          << " bytes long. No data was updated by this call.");

      return;
    }

    wgpu::Buffer wgpuBuffer = this->WebGPUBuffers[bufferIndex];
    this->ParentPassWGPUConfiguration->WriteBuffer(wgpuBuffer, 0, bytes, numBytes);
  }

  void WriteBuffer(
    std::size_t bufferIndex, vtkIdType byteOffset, const void* bytes, std::size_t numBytes)
  {
    vtkWebGPUComputeBuffer* buffer = this->Buffers[bufferIndex];
    vtkIdType byteSize = buffer->GetByteSize();
    vtkIdType givenSize = numBytes;

    if (givenSize + byteOffset > byteSize)
    {
      vtkLog(ERROR,
        "void* data given to WriteBuffer with index "
          << bufferIndex << " and offset " << byteOffset << " is too big. " << givenSize
          << "bytes and offset " << byteOffset << " were given but the buffer is only " << byteSize
          << " bytes long. No data was updated by this call.");

      return;
    }

    wgpu::Buffer wgpuBuffer = this->WebGPUBuffers[bufferIndex];
    this->ParentPassWGPUConfiguration->WriteBuffer(wgpuBuffer, byteOffset, bytes, numBytes);
  }

  /**
   * Updates the data of a buffer with a vtkDataArray.
   * The given data is expected to be at most the size of the buffer.
   * If N bytes are given to update but the buffer size is > N, only the first N bytes
   * will be updated, the rest will remain unchanged.
   * The data is immediately available to the GPU (no call to vtkWebGPUComputePipeline::Update() is
   * necessary for this call to take effect
   *
   * @note: This method can be used even if the buffer was initially configured with std::vector
   * data and the given data can safely be destroyed directly after calling this function.
   */
  void UpdateBufferData(std::size_t bufferIndex, vtkDataArray* newData);

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  void UpdateBufferData(std::size_t bufferIndex, vtkIdType byteOffset, vtkDataArray* newData);

  /**
   * Checks if a given index is suitable for indexing a buffer of this storage.
   *
   * Logs an error if the index is negative or greater than the number of buffer of the storage. The
   * callerFunctionName parameter is used to give more information on what function used an invalid
   * buffer index in case of error.
   *
   * Returns true if the buffer index is valid, false if it's not.
   */
  bool CheckBufferIndex(std::size_t bufferIndex, const std::string& callerFunctionName);

  /**
   * Makes some various (and obvious) checks to ensure that the buffer is ready to be created.
   *
   * Returns true if the buffer is correct.
   * If the buffer is incorrect, returns false.
   */
  bool CheckBufferCorrectness(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer);

  /**
   * Binds the buffer to the pipeline at the WebGPU level.
   * To use once the buffer has been properly set up with SetupRenderBuffer.
   */
  void SetupRenderBuffer(vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer);

  /**
   * Releases the buffers & resources held by this buffer storage.
   */
  void ReleaseResources();

  /**
   * Internal method used to convert the user friendly BufferMode to the internal enum
   * wgpu::BufferUsage
   */
  static wgpu::BufferUsage ComputeBufferModeToBufferUsage(vtkWebGPUComputeBuffer::BufferMode mode);

  /**
   * Internal method used to convert the user friendly BufferMode to the internal enum
   * wgpu::BufferBindingType
   */
  static wgpu::BufferBindingType ComputeBufferModeToBufferBindingType(
    vtkWebGPUComputeBuffer::BufferMode mode);

protected:
  vtkWebGPUComputePassBufferStorageInternals() = default;
  ~vtkWebGPUComputePassBufferStorageInternals() override;

private:
  friend class vtkWebGPUComputePassInternals;

  vtkWebGPUComputePassBufferStorageInternals(
    const vtkWebGPUComputePassBufferStorageInternals&) = delete;
  void operator=(const vtkWebGPUComputePassBufferStorageInternals&) = delete;

  // Compute pass that uses this buffer storage
  vtkWeakPointer<vtkWebGPUComputePass> ParentComputePass;
  // Device of the parent compute pass that is used when creating buffers
  vtkSmartPointer<vtkWebGPUConfiguration> ParentPassWGPUConfiguration;

  // Compute buffers
  std::vector<vtkSmartPointer<vtkWebGPUComputeBuffer>> Buffers;
  // WebGPU buffers associated with the compute buffers, in the same order
  std::vector<wgpu::Buffer> WebGPUBuffers;
};

#endif

VTK_ABI_NAMESPACE_END
