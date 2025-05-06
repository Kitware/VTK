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
#ifndef viskores_cont_cuda_internal_DeviceAdapterMemoryManagerCuda_h
#define viskores_cont_cuda_internal_DeviceAdapterMemoryManagerCuda_h

#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class VISKORES_CONT_EXPORT DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCuda>
  : public DeviceAdapterMemoryManagerBase
{
public:
  VISKORES_CONT viskores::cont::internal::BufferInfo Allocate(
    viskores::BufferSizeType size) const override;

  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override;

  VISKORES_CONT viskores::cont::internal::BufferInfo CopyHostToDevice(
    const viskores::cont::internal::BufferInfo& src) const override;

  VISKORES_CONT virtual void CopyHostToDevice(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const override;

  VISKORES_CONT viskores::cont::internal::BufferInfo CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo& src) const override;

  VISKORES_CONT virtual void CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const override;

  VISKORES_CONT viskores::cont::internal::BufferInfo CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo& src) const override;

  VISKORES_CONT virtual void CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const override;

  VISKORES_CONT virtual void DeleteRawPointer(void* mem) const override;
};
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_cuda_internal_DeviceAdapterMemoryManagerCuda_h
