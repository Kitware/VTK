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
#ifndef viskores_interop_internal_TransferToOpenGL_h
#define viskores_interop_internal_TransferToOpenGL_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Storage.h>

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/tbb/DeviceAdapterTBB.h>

#include <viskores/interop/BufferState.h>
#include <viskores/interop/internal/OpenGLHeaders.h>

namespace viskores
{
namespace interop
{
namespace internal
{

/// \brief smp backend and opengl interop resource management
///
/// \c TransferResource manages any extra memory allocation that is required
///    When binding an implicit array handle to opengl
///
///
class SMPTransferResource : public viskores::interop::internal::TransferResource
{
public:
  template <typename T>
  SMPTransferResource(T, viskores::Id numberOfValues)
    : viskores::interop::internal::TransferResource()
    , Size(0)
    , TempStorage()
  {
    this->resize<T>(numberOfValues);
  }

  ~SMPTransferResource() {}

  template <typename T>
  void resize(viskores::Id numberOfValues)
  {
    if (this->Size != numberOfValues)
    {
      this->Size = numberOfValues;
      T* storage = new T[static_cast<std::size_t>(this->Size)];
      this->TempStorage.reset(reinterpret_cast<viskores::UInt8*>(storage));
    }
  }

  template <typename T>
  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> handle(viskores::Id size) const
  {
    VISKORES_ASSERT(this->Size > 0);
    VISKORES_ASSERT(this->Size >= size);

    T* storage = reinterpret_cast<T*>(this->TempStorage.get());
    //construct a handle that is a view onto the memory
    return viskores::cont::make_ArrayHandle(storage, size, viskores::CopyFlag::Off);
  }

  template <typename T>
  T* as() const
  {
    VISKORES_ASSERT(this->Size > 0);
    T* storage = reinterpret_cast<T*>(this->TempStorage.get());
    return storage;
  }

  viskores::Id Size;
  std::unique_ptr<viskores::UInt8[]> TempStorage;
};

namespace detail
{

template <class ValueType, class StorageTag, class DeviceAdapterTag>
VISKORES_CONT void CopyFromHandle(const viskores::cont::ArrayHandle<ValueType, StorageTag>& handle,
                                  viskores::interop::BufferState& state,
                                  DeviceAdapterTag)
{
  //Generic implementation that will work no matter what. We copy the data
  //in the given handle to storage held by the buffer state.
  const viskores::Id numberOfValues = handle.GetNumberOfValues();
  const GLsizeiptr size =
    static_cast<GLsizeiptr>(sizeof(ValueType)) * static_cast<GLsizeiptr>(numberOfValues);

  //grab the temporary storage from the buffer resource
  viskores::interop::internal::SMPTransferResource* resource =
    dynamic_cast<viskores::interop::internal::SMPTransferResource*>(state.GetResource());

  //Determine if we need to reallocate the buffer
  state.SetSize(size);
  const bool resize = state.ShouldRealloc(size);

  if (resize)
  {
    //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
    glBufferData(state.GetType(), size, 0, GL_DYNAMIC_DRAW);
    state.SetCapacity(size);

    //If we have an existing resource reallocate it to fit our new size
    if (resource)
    {
      resource->resize<ValueType>(numberOfValues);
    }
  }

  //if we don't have a valid resource make a new one. We do this after the
  //resize check so we don't double allocate when resource == nullptr and
  //resize == true
  if (!resource)
  {
    resource = new viskores::interop::internal::SMPTransferResource(ValueType(), numberOfValues);
    state.SetResource(resource);
  }

  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  auto resourceHandle = resource->handle<ValueType>(numberOfValues);
  Algorithm::Copy(handle, resourceHandle);

  //copy into opengl buffer
  glBufferSubData(state.GetType(), 0, size, resource->as<ValueType>());
}

template <class ValueType, class DeviceAdapterTag>
VISKORES_CONT void CopyFromHandle(
  const viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagBasic>& handle,
  viskores::interop::BufferState& state,
  DeviceAdapterTag)
{
  //Specialization given that we are use an C allocated array storage tag
  //that allows us to directly hook into the data. We pull the data
  //back to the control environment using PerpareForInput and give an iterator
  //from the portal to OpenGL to upload to the rendering system
  //This also works because we know that this class isn't used for cuda interop,
  //instead we are specialized
  const GLsizeiptr size = static_cast<GLsizeiptr>(sizeof(ValueType)) *
    static_cast<GLsizeiptr>(handle.GetNumberOfValues());

  //Determine if we need to reallocate the buffer
  state.SetSize(size);
  const bool resize = state.ShouldRealloc(size);
  if (resize)
  {
    //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
    glBufferData(state.GetType(), size, 0, GL_DYNAMIC_DRAW);

    state.SetCapacity(size);
  }

  //Allocate the memory and set it as static draw and copy into opengl
  viskores::cont::Token token;
  auto portal = handle.PrepareForInput(DeviceAdapterTag{}, token);
  const ValueType* memory = &(*viskores::cont::ArrayPortalToIteratorBegin(portal));
  glBufferSubData(state.GetType(), 0, size, memory);
}

} //namespace detail

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible.
///
template <typename ValueType, class DeviceAdapterTag>
class TransferToOpenGL
{
public:
  VISKORES_CONT explicit TransferToOpenGL(BufferState& state)
    : State(state)
  {
    if (!this->State.HasType())
    {
      this->State.DeduceAndSetType(ValueType());
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

    //transfer the data.
    //the primary concern that we have at this point is data locality and
    //the type of storage. Our options include using DeviceAdapterAlgorithm::Copy
    //and provide a temporary location for the values to reside before we give it
    //to openGL this works for all storage types.
    //
    //Second option is to call PrepareForInput and get a PortalConst in the
    //execution environment.
    //if we are StorageTagBasic this would allow us the ability to grab
    //the raw memory value and copy those, which we know are valid and remove
    //a unneeded copy.
    //
    //The end result is that we have CopyFromHandle which does number two
    //from StorageTagBasic, and does the DeviceAdapterAlgorithm::Copy for everything
    //else
    detail::CopyFromHandle(handle, this->State, DeviceAdapterTag());
  }

private:
  viskores::interop::BufferState& State;
};
}
}
} //namespace viskores::interop::internal

//-----------------------------------------------------------------------------
// These includes are intentionally placed here after the declaration of the
// TransferToOpenGL class, so that people get the correct device adapter
/// specializations if they exist.
#if VISKORES_DEVICE_ADAPTER == VISKORES_DEVICE_ADAPTER_CUDA
#include <viskores/interop/cuda/internal/TransferToOpenGL.h>
#endif

#endif //viskores_interop_internal_TransferToOpenGL_h
