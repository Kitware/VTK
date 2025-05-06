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
#ifndef viskores_cont_internal_DeviceAdapterMemoryManagerShared_h
#define viskores_cont_internal_DeviceAdapterMemoryManagerShared_h

#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// \brief An implementation of DeviceAdapterMemoryManager for devices that share memory with the
/// host.
///
/// Device adapters that have shared memory with the host can implement their
/// `DeviceAdapterMemoryManager` as a simple subclass of this.
///
class VISKORES_CONT_EXPORT DeviceAdapterMemoryManagerShared : public DeviceAdapterMemoryManagerBase
{
public:
  VISKORES_CONT BufferInfo Allocate(viskores::BufferSizeType size) const override;

  VISKORES_CONT viskores::cont::internal::BufferInfo CopyHostToDevice(
    const viskores::cont::internal::BufferInfo& src) const override;

  VISKORES_CONT void CopyHostToDevice(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const override;

  VISKORES_CONT viskores::cont::internal::BufferInfo CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo& src) const override;

  VISKORES_CONT void CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const override;

  VISKORES_CONT viskores::cont::internal::BufferInfo CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo& src) const override;

  VISKORES_CONT void CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const override;

  VISKORES_CONT void DeleteRawPointer(void* mem) const override;
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_DeviceAdapterMemoryManagerShared_h
