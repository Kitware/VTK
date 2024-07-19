// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUInternalsComputePassBufferStorage_h
#define vtkWebGPUInternalsComputePassBufferStorage_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"              // for smart pointers
#include "vtkWeakPointer.h"               // for weak pointers
#include "vtkWebGPUComputeBuffer.h"       // for compute buffers
#include "vtkWebGPUComputeRenderBuffer.h" // for compute render buffers
#include "vtk_wgpu.h"                     // for webgpu

VTK_ABI_NAMESPACE_BEGIN

/**
 * This class manages the creation/deletion/recreation/resizing/updating of compute buffers used by
 * a compute pass.
 *
 * A compute pass delegates calls that want to modify buffers to this class
 */
class vtkWebGPUInternalsComputePassBufferStorage : public vtkObject
{
public:
  static vtkWebGPUInternalsComputePassBufferStorage* New();
  vtkTypeMacro(vtkWebGPUInternalsComputePassBufferStorage, vtkObject);

  /*
   * Callback called when the asynchronous mapping of a buffer is done
   * and data is ready to be copied.
   * This callback takes three parameters:
   *
   * - A first void pointer to the data mapped from the GPU ready to be copied
   * - A second void pointer pointing to user data, which can essentially be anything
   *      needed by the callback to copy the data to the CPU
   */
  using BufferMapAsyncCallback = std::function<void(const void*, void*)>;

  /**
   * Sets the device that will be used by this buffer storage when creating buffers.
   *
   * This device must be the one used by the parent compute pass.
   */
  void SetParentDevice(wgpu::Device device);

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
   * Adds a render texture to the pipeline. A render texture can be obtained from
   * vtkWebGPUPolyDataMapper::AcquirePointXXXXRenderBuffer() or
   * vtkWebGPUPolyDataMapper::AcquireCellXXXXRenderBuffer()
   */
  void AddRenderBuffer(vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer);

  /**
   * Returns the size in bytes of a buffer
   */
  unsigned int GetBufferByteSize(int bufferIndex);

  /**
   * Resizes a buffer.
   *
   * @warning: After the resize, the data of the buffer is undefined and should be updated by a
   * call to UpdateBufferData()
   */
  void ResizeBuffer(int bufferIndex, vtkIdType newByteSize);

  /**
   * Destroys and recreates a buffer with the given newByteSize
   * Only the wgpu::Buffer object is recreated so the binding/group of the group doesn't change
   */
  void RecreateBuffer(int bufferIndex, vtkIdType newByteSize);

  /*
   * This function maps the buffer, making it accessible to the CPU. This is
   * an asynchronous operation, meaning that the given callback will be called
   * when the mapping is done.
   *
   * The buffer data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadBufferFromGPU(int bufferIndex,
    vtkWebGPUInternalsComputePassBufferStorage::BufferMapAsyncCallback callback, void* userdata);

  /**
   * Updates the wgpu::Buffer reference that a compute buffer is associated to. Useful when a
   * compute buffer has been recreated and the associated wgpu::Buffer needs to be updated with the
   * newly created buffer
   */
  void UpdateWebGPUBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer);

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
  template <typename T>
  void UpdateBufferData(int bufferIndex, const std::vector<T>& newData)
  {
    if (!CheckBufferIndex(bufferIndex, std::string("UpdataBufferData")))
    {
      return;
    }

    vtkWebGPUComputeBuffer* buffer = this->Buffers[bufferIndex];
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

    wgpu::Buffer wgpuBuffer = this->WebGPUBuffers[bufferIndex];
    this->ParentPassDevice.GetQueue().WriteBuffer(
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
    if (!CheckBufferIndex(bufferIndex, std::string("UpdataBufferData with offset")))
    {
      return;
    }

    vtkWebGPUComputeBuffer* buffer = this->Buffers[bufferIndex];
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

    wgpu::Buffer wgpuBuffer = this->WebGPUBuffers[bufferIndex];
    this->ParentPassDevice.GetQueue().WriteBuffer(
      wgpuBuffer, byteOffset, data.data(), data.size() * sizeof(T));
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
  void UpdateBufferData(int bufferIndex, vtkDataArray* newData);

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  void UpdateBufferData(int bufferIndex, vtkIdType byteOffset, vtkDataArray* newData);

  /**
   * Checks if a given index is suitable for indexing a buffer of this storage.
   *
   * Logs an error if the index is negative or greater than the number of buffer of the storage. The
   * callerFunctionName parameter is used to give more information on what function used an invalid
   * buffer index in case of error.
   *
   * Returns true if the buffer index is valid, false if it's not.
   */
  bool CheckBufferIndex(int bufferIndex, const std::string& callerFunctionName);

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

private:
  friend class vtkWebGPUInternalsComputePass;

  // Compute pass that uses this buffer storage
  vtkWeakPointer<vtkWebGPUComputePass> ParentComputePass;
  // Device of the parent compute pass that is used when creating buffers
  wgpu::Device ParentPassDevice;

  // Compute buffers
  std::vector<vtkSmartPointer<vtkWebGPUComputeBuffer>> Buffers;
  // WebGPU buffers associated with the compute buffers, in the same order
  std::vector<wgpu::Buffer> WebGPUBuffers;
};

#endif

VTK_ABI_NAMESPACE_END
