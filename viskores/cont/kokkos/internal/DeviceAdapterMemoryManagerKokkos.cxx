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
#include <viskores/cont/kokkos/internal/DeviceAdapterMemoryManagerKokkos.h>

#include <viskores/cont/kokkos/DeviceAdapterKokkos.h>
#include <viskores/cont/kokkos/internal/KokkosAlloc.h>
#include <viskores/cont/kokkos/internal/KokkosTypes.h>

namespace
{

void* KokkosAllocate(viskores::BufferSizeType size)
{
  return viskores::cont::kokkos::internal::Allocate(static_cast<std::size_t>(size));
}

void KokkosDelete(void* memory)
{
  viskores::cont::kokkos::internal::Free(memory);
}

void KokkosReallocate(void*& memory,
                      void*& container,
                      viskores::BufferSizeType oldSize,
                      viskores::BufferSizeType newSize)
{
  VISKORES_ASSERT(memory == container);
  if (newSize > oldSize)
  {
    if (oldSize == 0)
    {
      memory = container = viskores::cont::kokkos::internal::Allocate(newSize);
    }
    else
    {
      memory = container = viskores::cont::kokkos::internal::Reallocate(container, newSize);
    }
  }
}
}

namespace viskores
{
namespace cont
{
namespace internal
{

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManager<
  viskores::cont::DeviceAdapterTagKokkos>::Allocate(viskores::BufferSizeType size) const
{
  void* memory = KokkosAllocate(size);
  return viskores::cont::internal::BufferInfo(
    viskores::cont::DeviceAdapterTagKokkos{}, memory, memory, size, KokkosDelete, KokkosReallocate);
}

viskores::cont::DeviceAdapterId
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::GetDevice() const
{
  return viskores::cont::DeviceAdapterTagKokkos{};
}

viskores::cont::internal::BufferInfo
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::CopyHostToDevice(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == viskores::cont::DeviceAdapterTagUndefined{});

  // Make a new buffer
  viskores::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());
  this->CopyHostToDevice(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::CopyHostToDevice(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  viskores::BufferSizeType size = viskores::Min(src.GetSize(), dest.GetSize());

  VISKORES_LOG_F(viskores::cont::LogLevel::MemTransfer,
                 "Copying host --> Kokkos dev: %s (%lld bytes)",
                 viskores::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
                 size);

  viskores::cont::kokkos::internal::KokkosViewConstCont<viskores::UInt8> srcView(
    static_cast<viskores::UInt8*>(src.GetPointer()), static_cast<std::size_t>(size));
  viskores::cont::kokkos::internal::KokkosViewExec<viskores::UInt8> destView(
    static_cast<viskores::UInt8*>(dest.GetPointer()), static_cast<std::size_t>(size));
  Kokkos::deep_copy(
    viskores::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
}

viskores::cont::internal::BufferInfo
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::CopyDeviceToHost(
  const viskores::cont::internal::BufferInfo& src) const
{
  VISKORES_ASSERT(src.GetDevice() == viskores::cont::DeviceAdapterTagKokkos{});

  // Make a new buffer
  viskores::cont::internal::BufferInfo dest;
  dest = viskores::cont::internal::AllocateOnHost(src.GetSize());
  this->CopyDeviceToHost(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::CopyDeviceToHost(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  viskores::BufferSizeType size = viskores::Min(src.GetSize(), dest.GetSize());

  VISKORES_LOG_F(viskores::cont::LogLevel::MemTransfer,
                 "Copying Kokkos dev --> host: %s (%lld bytes)",
                 viskores::cont::GetHumanReadableSize(static_cast<std::size_t>(size)).c_str(),
                 size);

  viskores::cont::kokkos::internal::KokkosViewConstExec<viskores::UInt8> srcView(
    static_cast<viskores::UInt8*>(src.GetPointer()), static_cast<std::size_t>(size));
  viskores::cont::kokkos::internal::KokkosViewCont<viskores::UInt8> destView(
    static_cast<viskores::UInt8*>(dest.GetPointer()), static_cast<std::size_t>(size));
  Kokkos::deep_copy(
    viskores::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
  viskores::cont::kokkos::internal::GetExecutionSpaceInstance().fence();
}

viskores::cont::internal::BufferInfo
DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::CopyDeviceToDevice(
  const viskores::cont::internal::BufferInfo& src) const
{
  viskores::cont::internal::BufferInfo dest = this->Allocate(src.GetSize());
  this->CopyDeviceToDevice(src, dest);

  return dest;
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::CopyDeviceToDevice(
  const viskores::cont::internal::BufferInfo& src,
  const viskores::cont::internal::BufferInfo& dest) const
{
  viskores::BufferSizeType size = viskores::Min(src.GetSize(), dest.GetSize());

  viskores::cont::kokkos::internal::KokkosViewConstExec<viskores::UInt8> srcView(
    static_cast<viskores::UInt8*>(src.GetPointer()), static_cast<std::size_t>(size));
  viskores::cont::kokkos::internal::KokkosViewExec<viskores::UInt8> destView(
    static_cast<viskores::UInt8*>(dest.GetPointer()), static_cast<std::size_t>(size));
  Kokkos::deep_copy(
    viskores::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
}

// Low level memory management methods
void* DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::AllocateRawPointer(
  viskores::BufferSizeType size) const
{
  return viskores::cont::kokkos::internal::Allocate(size);
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::
  CopyDeviceToDeviceRawPointer(const void* src, void* dest, viskores::BufferSizeType size) const
{
  Kokkos::View<char*, Kokkos::MemoryTraits<Kokkos::Unmanaged>> destView(static_cast<char*>(dest),
                                                                        size);
  Kokkos::View<const char*, Kokkos::MemoryTraits<Kokkos::Unmanaged>> srcView(
    static_cast<const char*>(src), size);
  Kokkos::deep_copy(
    viskores::cont::kokkos::internal::GetExecutionSpaceInstance(), destView, srcView);
}

void DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagKokkos>::DeleteRawPointer(
  void* mem) const
{
  viskores::cont::kokkos::internal::Free(mem);
}

}
}
} // viskores::cont::internal
