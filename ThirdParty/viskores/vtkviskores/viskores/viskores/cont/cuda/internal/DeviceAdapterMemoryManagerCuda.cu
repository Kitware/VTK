//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/cuda/ErrorCuda.h>
#include <viskores/cont/cuda/internal/CudaAllocator.h>
#include <viskores/cont/cuda/internal/DeviceAdapterMemoryManagerCuda.h>

#include <viskores/cont/ErrorBadAllocation.h>

#include <viskores/Math.h>

namespace
{

void* CudaAllocate(viskores::BufferSizeType size)
{
  try
  {
    return viskores::cont::cuda::internal::CudaAllocator::Allocate(static_cast<std::size_t>(size));
  }
  catch (const std::exception& error)
  {
    std::ostringstream err;
    err << "Failed to allocate " << size << " bytes on CUDA device: " << error.what();
    throw viskores::cont::ErrorBadAllocation(err.str());
  }
}

void CudaDelete(void* memory)
{
  if (memory != nullptr)
  {
    viskores::cont::cuda::internal::CudaAllocator::Free(memory);
  }
};

void CudaReallocate(void*& memory,
                    void*& container,
                    viskores::BufferSizeType oldSize,
                    viskores::BufferSizeType newSize)
{
  VISKORES_ASSERT(memory == container);

  if (newSize > oldSize)
  {
    // Make a new buffer
    void* newMemory = CudaAllocate(newSize);

    // Copy the data to the new buffer
    VISKORES_CUDA_CALL(cudaMemcpyAsync(newMemory,
                                       memory,
                                       static_cast<std::size_t>(oldSize),
                                       cudaMemcpyDeviceToDevice,
                                       cudaStreamPerThread));

    // Reset the buffer in the passed in info
    memory = container = newMemory;
  }
  else
  {
    // Just reuse the buffer.
  }
}

} // anonymous namespace

namespace viskores
{
namespace cont
{
namespace internal
{

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManager<
  viskores::cont::DeviceAdapterTagCuda>::Allocate(viskores::BufferSizeType size) const
{
  void* memory = CudaAllocate(size);
  return viskores::cont::internal::BufferInfo(
    viskores::cont::DeviceAdapterTagCuda{}, memory, memory, size, CudaDelete, CudaReallocate);
}

viskores::cont::DeviceAdapterId
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::GetDevice() const
{
  return viskores::cont::DeviceAdapterTagCuda{};
}

viskores::cont::internal::BufferInfo
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::CopyHostToDevice(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == viskores::cont::DeviceAdapterTagUndefined{});

  if (viskores::cont::cuda::internal::CudaAllocator::IsManagedPointer(src.GetPointer()))
  {
    // In the current code structure, we don't know whether this buffer is going to be used
    // for input or output. (Currently, I don't think there is any difference.)
    viskores::cont::cuda::internal::CudaAllocator::PrepareForOutput(
      src.GetPointer(), static_cast<std::size_t>(src.GetSize()));

    // The provided control pointer is already cuda managed and can be accessed on the device
    // via unified memory. Just shallow copy the pointer.
    return viskores::cont::internal::BufferInfo(src, viskores::cont::DeviceAdapterTagCuda{});
  }
  else
  {
    // Make a new buffer
    viskores::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());

    this->CopyHostToDevice(src, dest);

    return dest;
  }
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::CopyHostToDevice(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  if (viskores::cont::cuda::internal::CudaAllocator::IsManagedPointer(src.GetPointer()) &&
      src.GetPointer() == dest.GetPointer())
  {
    // In the current code structure, we don't know whether this buffer is going to be used
    // for input or output. (Currently, I don't think there is any difference.)
    viskores::cont::cuda::internal::CudaAllocator::PrepareForOutput(
      src.GetPointer(), static_cast<std::size_t>(src.GetSize()));

    // The provided pointers are both cuda managed and the same, so the data are already
    // the same.
  }
  else
  {
    viskores::BufferSizeType size = viskores::Min(src.GetSize(), dest.GetSize());

    VISKORES_LOG_F(viskores::cont::LogLevel::MemTransfer,
                   "Copying host --> CUDA dev: %s (%lld bytes)",
                   viskores::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
                   size);

    VISKORES_CUDA_CALL(cudaMemcpyAsync(dest.GetPointer(),
                                       src.GetPointer(),
                                       static_cast<std::size_t>(size),
                                       cudaMemcpyHostToDevice,
                                       cudaStreamPerThread));
  }
}


viskores::cont::internal::BufferInfo
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::CopyDeviceToHost(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == viskores::cont::DeviceAdapterTagCuda{});

  viskores::cont::internal::BufferInfo dest;

  if (viskores::cont::cuda::internal::CudaAllocator::IsManagedPointer(src.GetPointer()))
  {
    // The provided control pointer is already cuda managed and can be accessed on the host
    // via unified memory. Just shallow copy the pointer.
    viskores::cont::cuda::internal::CudaAllocator::PrepareForControl(
      src.GetPointer(), static_cast<std::size_t>(src.GetSize()));
    dest = viskores::cont::internal::BufferInfo(src, viskores::cont::DeviceAdapterTagUndefined{});

    //In all cases we have possibly multiple async calls queued up in
    //our stream. We need to block on the copy back to control since
    //we don't wanting it accessing memory that hasn't finished
    //being used by the GPU
    viskores::cont::DeviceAdapterAlgorithm<DeviceAdapterTagCuda>::Synchronize();
  }
  else
  {
    // Make a new buffer
    dest = viskores::cont::internal::AllocateOnHost(src.GetSize());

    this->CopyDeviceToHost(src, dest);
  }

  return dest;
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::CopyDeviceToHost(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  if (viskores::cont::cuda::internal::CudaAllocator::IsManagedPointer(dest.GetPointer()) &&
      src.GetPointer() == dest.GetPointer())
  {
    // The provided pointers are both cuda managed and the same, so the data are already
    // the same.
  }
  else
  {
    viskores::BufferSizeType size = viskores::Min(src.GetSize(), dest.GetSize());

    VISKORES_LOG_F(viskores::cont::LogLevel::MemTransfer,
                   "Copying CUDA dev --> host: %s (%lld bytes)",
                   viskores::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
                   size);

    VISKORES_CUDA_CALL(cudaMemcpyAsync(dest.GetPointer(),
                                       src.GetPointer(),
                                       static_cast<std::size_t>(size),
                                       cudaMemcpyDeviceToHost,
                                       cudaStreamPerThread));
  }

  //In all cases we have possibly multiple async calls queued up in
  //our stream. We need to block on the copy back to control since
  //we don't wanting it accessing memory that hasn't finished
  //being used by the GPU
  viskores::cont::DeviceAdapterAlgorithm<DeviceAdapterTagCuda>::Synchronize();
}

viskores::cont::internal::BufferInfo
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::CopyDeviceToDevice(
  const viskores::cont::internal::BufferInfo& src) const
{
  viskores::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());
  this->CopyDeviceToDevice(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::CopyDeviceToDevice(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  VISKORES_CUDA_CALL(cudaMemcpyAsync(dest.GetPointer(),
                                     src.GetPointer(),
                                     static_cast<std::size_t>(src.GetSize()),
                                     cudaMemcpyDeviceToDevice,
                                     cudaStreamPerThread));
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>::DeleteRawPointer(
  void* mem) const
{
  CudaDelete(mem);
};
}
}
} // namespace viskores::cont::internal
