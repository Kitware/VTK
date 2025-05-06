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

#include <viskores/cont/internal/DeviceAdapterMemoryManagerShared.h>

#include <cstring>

namespace viskores
{
namespace cont
{
namespace internal
{

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::Allocate(
  viskores::BufferSizeType size) const
{
  return viskores::cont::internal::BufferInfo(viskores::cont::internal::AllocateOnHost(size),
                                              this->GetDevice());
}

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::CopyHostToDevice(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == viskores::cont::DeviceAdapterTagUndefined{});
  return viskores::cont::internal::BufferInfo(src, this->GetDevice());
}

void DeviceAdapterMemoryManagerShared::CopyHostToDevice(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  VISKORES_ASSERT(src.GetDevice() == viskores::cont::DeviceAdapterTagUndefined{});
  VISKORES_ASSERT(dest.GetDevice() == this->GetDevice());
  if (src.GetPointer() != dest.GetPointer())
  {
    this->CopyDeviceToDevice(src, dest);
  }
}

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::CopyDeviceToHost(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == this->GetDevice());
  return viskores::cont::internal::BufferInfo(src, viskores::cont::DeviceAdapterTagUndefined{});
}

void DeviceAdapterMemoryManagerShared::CopyDeviceToHost(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  VISKORES_ASSERT(src.GetDevice() == this->GetDevice());
  VISKORES_ASSERT(dest.GetDevice() == viskores::cont::DeviceAdapterTagUndefined{});
  if (src.GetPointer() != dest.GetPointer())
  {
    this->CopyDeviceToDevice(src, dest);
  }
}

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManagerShared::CopyDeviceToDevice(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == this->GetDevice());

  viskores::BufferSizeType size = src.GetSize();
  viskores::cont::internal::BufferInfo dest = this->Allocate(size);
  this->CopyDeviceToDevice(src, dest);
  return dest;
}

void DeviceAdapterMemoryManagerShared::CopyDeviceToDevice(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  VISKORES_ASSERT(src.GetSize() == dest.GetSize());

  std::memcpy(dest.GetPointer(), src.GetPointer(), static_cast<std::size_t>(src.GetSize()));
}

void DeviceAdapterMemoryManagerShared::DeleteRawPointer(void* mem) const
{
  viskores::cont::internal::HostDeleter(mem);
}

}
}
} // namespace viskores::cont::internal
