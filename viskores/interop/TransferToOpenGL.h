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
#ifndef viskores_interop_TransferToOpenGL_h
#define viskores_interop_TransferToOpenGL_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/interop/BufferState.h>
#include <viskores/interop/internal/TransferToOpenGL.h>

#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/tbb/DeviceAdapterTBB.h>

namespace viskores
{
namespace interop
{

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p state to determine
/// what buffer handle to use, and the type to bind the buffer handle too.
/// Lastly state also holds on to per backend resources that allow for efficient
/// updating to open gl.
///
/// This function keeps the buffer as the active buffer of the input type.
///
///
template <typename ValueType, class StorageTag, class DeviceAdapterTag>
VISKORES_CONT void TransferToOpenGL(
  const viskores::cont::ArrayHandle<ValueType, StorageTag>& handle,
  BufferState& state,
  DeviceAdapterTag)
{
  viskores::interop::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(state);
  toGL.Transfer(handle);
}

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p state to determine
/// what buffer handle to use, and the type to bind the buffer handle too.
/// If the type of buffer hasn't been determined the transfer will use
/// deduceAndSetBufferType to do so. Lastly state also holds on to per backend resources
/// that allow for efficient updating to open gl
///
/// This function keeps the buffer as the active buffer of the input type.
///
/// This function will throw exceptions if the transfer wasn't possible
///
template <typename ValueType, typename StorageTag>
VISKORES_CONT void TransferToOpenGL(
  const viskores::cont::ArrayHandle<ValueType, StorageTag>& handle,
  BufferState& state)
{
  // First, try to transfer data that already exists on a device.
  bool success = viskores::cont::TryExecute(
    [&](auto device)
    {
      if (handle.IsOnDevice(device))
      {
        TransferToOpenGL(handle, state, device);
        return true;
      }
      else
      {
        return false;
      }
    });
  if (!success)
  {
    // Generally, we are here because the array is not already on a device
    // or for some reason the transfer failed on that device. Try any device.
    success = viskores::cont::TryExecute(
      [&](auto device)
      {
        TransferToOpenGL(handle, state, device);
        return true;
      });
  }
  if (!success)
  {
    throw viskores::cont::ErrorBadValue("Failed to transfer array to OpenGL on any device.");
  }
}
}
}

#endif //viskores_interop_TransferToOpenGL_h
