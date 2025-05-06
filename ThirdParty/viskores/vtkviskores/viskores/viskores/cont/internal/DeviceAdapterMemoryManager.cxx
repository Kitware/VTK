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

#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

#include <viskores/Math.h>

#include <atomic>
#include <cstring>

//----------------------------------------------------------------------------------------
// Special allocation/deallocation code

#if defined(VISKORES_POSIX)
#define VISKORES_MEMALIGN_POSIX
#elif defined(_WIN32)
#define VISKORES_MEMALIGN_WIN
#elif defined(__SSE__)
#define VISKORES_MEMALIGN_SSE
#else
#define VISKORES_MEMALIGN_NONE
#endif

#if defined(VISKORES_MEMALIGN_POSIX)
#include <stdlib.h>
#elif defined(VISKORES_MEMALIGN_WIN)
#include <malloc.h>
#elif defined(VISKORES_MEMALIGN_SSE)
#include <xmmintrin.h>
#else
#include <malloc.h>
#endif

#include <cstddef>
#include <cstdlib>

namespace viskores
{
namespace cont
{
namespace internal
{

/// A deleter object that can be used with our aligned mallocs
void HostDeleter(void* memory)
{
  if (memory == nullptr)
  {
    return;
  }

#if defined(VISKORES_MEMALIGN_POSIX)
  free(memory);
#elif defined(VISKORES_MEMALIGN_WIN)
  _aligned_free(memory);
#elif defined(VISKORES_MEMALIGN_SSE)
  _mm_free(memory);
#else
  free(memory);
#endif
}

/// Allocates a buffer of a specified size using Viskores's preferred memory alignment.
/// Returns a void* pointer that should be deleted with `HostDeleter`.
void* HostAllocate(viskores::BufferSizeType numBytes)
{
  VISKORES_ASSERT(numBytes >= 0);
  if (numBytes <= 0)
  {
    return nullptr;
  }

  const std::size_t size = static_cast<std::size_t>(numBytes);
  constexpr std::size_t align = VISKORES_ALLOCATION_ALIGNMENT;

#if defined(VISKORES_MEMALIGN_POSIX)
  void* memory = nullptr;
  if (posix_memalign(&memory, align, size) != 0)
  {
    memory = nullptr;
  }
#elif defined(VISKORES_MEMALIGN_WIN)
  void* memory = _aligned_malloc(size, align);
#elif defined(VISKORES_MEMALIGN_SSE)
  void* memory = _mm_malloc(size, align);
#else
  void* memory = malloc(size);
#endif

  return memory;
}

/// Reallocates a buffer on the host.
void HostReallocate(void*& memory,
                    void*& container,
                    viskores::BufferSizeType oldSize,
                    viskores::BufferSizeType newSize)
{
  VISKORES_ASSERT(memory == container);

  // If the new size is not much smaller than the old size, just reuse the buffer (and waste a
  // little memory).
  if ((newSize > ((3 * oldSize) / 4)) && (newSize <= oldSize))
  {
    return;
  }

  void* newBuffer = HostAllocate(newSize);
  std::memcpy(newBuffer, memory, static_cast<std::size_t>(viskores::Min(newSize, oldSize)));

  if (memory != nullptr)
  {
    HostDeleter(memory);
  }

  memory = container = newBuffer;
}

VISKORES_CONT void InvalidRealloc(void*&,
                                  void*&,
                                  viskores::BufferSizeType,
                                  viskores::BufferSizeType)
{
  throw viskores::cont::ErrorBadAllocation("User provided memory does not have a reallocater.");
}


namespace detail
{

//----------------------------------------------------------------------------------------
// The BufferInfo internals behaves much like a std::shared_ptr. However, we do not use
// std::shared_ptr for compile efficiency issues.
struct BufferInfoInternals
{
  void* Memory;
  void* Container;
  BufferInfo::Deleter* Delete;
  BufferInfo::Reallocater* Reallocate;
  viskores::BufferSizeType Size;

  using CountType = viskores::IdComponent;
  std::atomic<CountType> Count;

  VISKORES_CONT BufferInfoInternals(void* memory,
                                    void* container,
                                    viskores::BufferSizeType size,
                                    BufferInfo::Deleter deleter,
                                    BufferInfo::Reallocater reallocater)
    : Memory(memory)
    , Container(container)
    , Delete(deleter)
    , Reallocate(reallocater)
    , Size(size)
    , Count(1)
  {
  }

  BufferInfoInternals(const BufferInfoInternals&) = delete;
  void operator=(const BufferInfoInternals&) = delete;
};

} // namespace detail

void* BufferInfo::GetPointer() const
{
  return this->Internals->Memory;
}

viskores::BufferSizeType BufferInfo::GetSize() const
{
  return this->Internals->Size;
}

viskores::cont::DeviceAdapterId BufferInfo::GetDevice() const
{
  return this->Device;
}

BufferInfo::BufferInfo()
  : Internals(new detail::BufferInfoInternals(nullptr, nullptr, 0, HostDeleter, HostReallocate))
  , Device(viskores::cont::DeviceAdapterTagUndefined{})
{
}

BufferInfo::~BufferInfo()
{
  if (this->Internals != nullptr)
  {
    detail::BufferInfoInternals::CountType oldCount =
      this->Internals->Count.fetch_sub(1, std::memory_order::memory_order_seq_cst);
    if (oldCount == 1)
    {
      this->Internals->Delete(this->Internals->Container);
      delete this->Internals;
      this->Internals = nullptr;
    }
  }
}

BufferInfo::BufferInfo(const BufferInfo& src)
  : Internals(src.Internals)
  , Device(src.Device)
{
  // Can add with relaxed because order does not matter. (But order does matter for decrement.)
  this->Internals->Count.fetch_add(1, std::memory_order::memory_order_relaxed);
}

BufferInfo::BufferInfo(BufferInfo&& src)
  : Internals(src.Internals)
  , Device(src.Device)
{
  src.Internals = nullptr;
}

BufferInfo& BufferInfo::operator=(const BufferInfo& src)
{
  detail::BufferInfoInternals::CountType oldCount =
    this->Internals->Count.fetch_sub(1, std::memory_order::memory_order_seq_cst);
  if (oldCount == 1)
  {
    this->Internals->Delete(this->Internals->Container);
    delete this->Internals;
    this->Internals = nullptr;
  }

  this->Internals = src.Internals;
  this->Device = src.Device;

  // Can add with relaxed because order does not matter. (But order does matter for decrement.)
  this->Internals->Count.fetch_add(1, std::memory_order::memory_order_relaxed);

  return *this;
}

BufferInfo& BufferInfo::operator=(BufferInfo&& src)
{
  detail::BufferInfoInternals::CountType oldCount =
    this->Internals->Count.fetch_sub(1, std::memory_order::memory_order_seq_cst);
  if (oldCount == 1)
  {
    this->Internals->Delete(this->Internals->Container);
    delete this->Internals;
    this->Internals = nullptr;
  }

  this->Internals = src.Internals;
  this->Device = src.Device;

  src.Internals = nullptr;

  return *this;
}

BufferInfo::BufferInfo(const BufferInfo& src, viskores::cont::DeviceAdapterId device)
  : Internals(src.Internals)
  , Device(device)
{
  // Can add with relaxed because order does not matter. (But order does matter for decrement.)
  this->Internals->Count.fetch_add(1, std::memory_order::memory_order_relaxed);
}

BufferInfo::BufferInfo(BufferInfo&& src, viskores::cont::DeviceAdapterId device)
  : Internals(src.Internals)
  , Device(device)
{
  src.Internals = nullptr;
}

BufferInfo::BufferInfo(viskores::cont::DeviceAdapterId device,
                       void* memory,
                       void* container,
                       viskores::BufferSizeType size,
                       Deleter deleter,
                       Reallocater reallocater)
  : Internals(new detail::BufferInfoInternals(memory, container, size, deleter, reallocater))
  , Device(device)
{
}

void BufferInfo::Reallocate(viskores::BufferSizeType newSize)
{
  this->Internals->Reallocate(
    this->Internals->Memory, this->Internals->Container, this->Internals->Size, newSize);
  this->Internals->Size = newSize;
}

TransferredBuffer BufferInfo::TransferOwnership()
{
  TransferredBuffer tbufffer = { this->Internals->Memory,
                                 this->Internals->Container,
                                 this->Internals->Delete,
                                 this->Internals->Reallocate,
                                 this->Internals->Size };

  this->Internals->Delete = [](void*) {};
  this->Internals->Reallocate = viskores::cont::internal::InvalidRealloc;

  return tbufffer;
}



//----------------------------------------------------------------------------------------
viskores::cont::internal::BufferInfo AllocateOnHost(viskores::BufferSizeType size)
{
  void* memory = HostAllocate(size);

  return viskores::cont::internal::BufferInfo(
    viskores::cont::DeviceAdapterTagUndefined{}, memory, memory, size, HostDeleter, HostReallocate);
}

//----------------------------------------------------------------------------------------
DeviceAdapterMemoryManagerBase::~DeviceAdapterMemoryManagerBase() {}

void DeviceAdapterMemoryManagerBase::Reallocate(viskores::cont::internal::BufferInfo& buffer,
                                                viskores::BufferSizeType newSize) const
{
  VISKORES_ASSERT(buffer.GetDevice() == this->GetDevice());
  buffer.Reallocate(newSize);
}

viskores::cont::internal::BufferInfo DeviceAdapterMemoryManagerBase::ManageArray(
  void* memory,
  void* container,
  viskores::BufferSizeType size,
  BufferInfo::Deleter deleter,
  BufferInfo::Reallocater reallocater) const
{
  return viskores::cont::internal::BufferInfo(
    this->GetDevice(), memory, container, size, deleter, reallocater);
}

void* DeviceAdapterMemoryManagerBase::AllocateRawPointer(viskores::BufferSizeType size) const
{
  return this->Allocate(size).TransferOwnership().Memory;
}

void DeviceAdapterMemoryManagerBase::CopyDeviceToDeviceRawPointer(
  const void* src,
  void* dest,
  viskores::BufferSizeType size) const
{
  this->CopyDeviceToDevice(
    viskores::cont::internal::BufferInfo(
      this->GetDevice(),
      const_cast<void*>(src),
      const_cast<void*>(src),
      size,
      [](void*) {},
      viskores::cont::internal::InvalidRealloc),
    viskores::cont::internal::BufferInfo(
      this->GetDevice(), dest, dest, size, [](void*) {}, viskores::cont::internal::InvalidRealloc));
}

}
}
} // namespace viskores::cont::internal
