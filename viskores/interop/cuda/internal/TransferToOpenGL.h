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
#ifndef viskores_interop_cuda_internal_TransferToOpenGL_h
#define viskores_interop_cuda_internal_TransferToOpenGL_h

#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/ErrorExecution.h>

#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <viskores/interop/internal/TransferToOpenGL.h>

// Disable warnings we check viskores for but Thrust does not.
#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <thrust/copy.h>
#include <thrust/device_ptr.h>
#include <thrust/system/cuda/execution_policy.h>
#include <viskores/exec/cuda/internal/ExecutionPolicy.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace interop
{
namespace internal
{

/// \brief cuda backend and opengl interop resource management
///
/// \c TransferResource manages cuda resource binding for a given buffer
///
///
class CudaTransferResource : public viskores::interop::internal::TransferResource
{
public:
  CudaTransferResource()
    : viskores::interop::internal::TransferResource()
  {
    this->Registered = false;
  }

  ~CudaTransferResource()
  {
    //unregister the buffer
    if (this->Registered)
    {
      cudaGraphicsUnregisterResource(this->CudaResource);
    }
  }

  bool IsRegistered() const { return Registered; }

  void Register(GLuint* handle)
  {
    if (this->Registered)
    {
      //If you don't release the cuda resource before re-registering you
      //will leak memory on the OpenGL side.
      cudaGraphicsUnregisterResource(this->CudaResource);
    }

    this->Registered = true;
    cudaError_t cError =
      cudaGraphicsGLRegisterBuffer(&this->CudaResource, *handle, cudaGraphicsMapFlagsWriteDiscard);
    if (cError != cudaSuccess)
    {
      throw viskores::cont::ErrorExecution("Could not register the OpenGL buffer handle to CUDA.");
    }
  }

  void Map()
  {
    //map the resource into cuda, so we can copy it
    cudaError_t cError = cudaGraphicsMapResources(1, &this->CudaResource, cudaStreamPerThread);
    if (cError != cudaSuccess)
    {
      throw viskores::cont::ErrorBadAllocation(
        "Could not allocate enough memory in CUDA for OpenGL interop.");
    }
  }

  template <typename ValueType>
  ValueType* GetMappedPoiner(viskores::Int64 desiredSize)
  {
    //get the mapped pointer
    std::size_t cuda_size;
    ValueType* pointer = nullptr;
    cudaError_t cError =
      cudaGraphicsResourceGetMappedPointer((void**)&pointer, &cuda_size, this->CudaResource);

    if (cError != cudaSuccess)
    {
      throw viskores::cont::ErrorExecution(
        "Unable to get pointers to CUDA memory for OpenGL buffer.");
    }

    //assert that cuda_size is the same size as the buffer we created in OpenGL
    VISKORES_ASSERT(cuda_size >= static_cast<std::size_t>(desiredSize));
    return pointer;
  }

  void UnMap() { cudaGraphicsUnmapResources(1, &this->CudaResource, cudaStreamPerThread); }

private:
  bool Registered;
  cudaGraphicsResource_t CudaResource;
};

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible.
///
template <typename ValueType>
class TransferToOpenGL<ValueType, viskores::cont::DeviceAdapterTagCuda>
{
  using DeviceAdapterTag = viskores::cont::DeviceAdapterTagCuda;

public:
  VISKORES_CONT explicit TransferToOpenGL(BufferState& state)
    : State(state)
    , Resource(nullptr)
  {
    if (!this->State.HasType())
    {
      this->State.DeduceAndSetType(ValueType());
    }

    this->Resource =
      dynamic_cast<viskores::interop::internal::CudaTransferResource*>(state.GetResource());
    if (!this->Resource)
    {
      viskores::interop::internal::CudaTransferResource* cudaResource =
        new viskores::interop::internal::CudaTransferResource();

      //reset the resource to be a cuda resource
      this->State.SetResource(cudaResource);
      this->Resource = cudaResource;
    }
  }

  template <typename StorageTag>
  VISKORES_CONT void Transfer(
    const viskores::cont::ArrayHandle<ValueType, StorageTag>& handle) const
  {
    //make a buffer for the handle if the user has forgotten too
    if (!glIsBuffer(*this->State.GetHandle()))
    {
      glGenBuffers(1, this->State.GetHandle());
    }

    //bind the buffer to the given buffer type
    glBindBuffer(this->State.GetType(), *this->State.GetHandle());

    //Determine if we need to reallocate the buffer
    const viskores::Int64 size =
      static_cast<viskores::Int64>(sizeof(ValueType)) * handle.GetNumberOfValues();
    this->State.SetSize(size);
    const bool resize = this->State.ShouldRealloc(size);
    if (resize)
    {
      //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
      glBufferData(this->State.GetType(), static_cast<GLsizeiptr>(size), 0, GL_DYNAMIC_DRAW);

      this->State.SetCapacity(size);
    }

    if (!this->Resource->IsRegistered() || resize)
    {
      //register the buffer as being used by cuda. This needs to be done everytime
      //we change the size of the buffer. That is why we only change the buffer
      //size as infrequently as possible
      this->Resource->Register(this->State.GetHandle());
    }

    this->Resource->Map();

    ValueType* beginPointer = this->Resource->GetMappedPoiner<ValueType>(size);
    viskores::cont::ArrayHandleBasic<ValueType> deviceMemory(
      beginPointer, handle.GetNumberOfValues(), DeviceAdapterTag{}, [](void*) {});

    //Do a device to device memory copy
    viskores::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>::Copy(handle, deviceMemory);

    //unmap the resource
    this->Resource->UnMap();
  }

private:
  viskores::interop::BufferState& State;
  viskores::interop::internal::CudaTransferResource* Resource;
};
}
}
} //namespace viskores::interop::cuda::internal

#endif
