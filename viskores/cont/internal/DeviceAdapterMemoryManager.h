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
#ifndef viskores_cont_internal_DeviceAdapterMemoryManager_h
#define viskores_cont_internal_DeviceAdapterMemoryManager_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Flags.h>
#include <viskores/Types.h>

#include <viskores/cont/DeviceAdapterTag.h>

#include <cstring>
#include <memory>
#include <vector>

namespace viskores
{

using BufferSizeType = viskores::Int64;

namespace cont
{
namespace internal
{

struct TransferredBuffer;

namespace detail
{

struct BufferInfoInternals;

} // namespace detail

class VISKORES_CONT_EXPORT BufferInfo
{
public:
  /// Returns a pointer to the memory that is allocated. This pointer may only be referenced on
  /// the associated device.
  ///
  VISKORES_CONT void* GetPointer() const;

  /// Returns the size of the buffer in bytes.
  ///
  VISKORES_CONT viskores::BufferSizeType GetSize() const;

  /// \brief Returns the device on which this buffer is allocated.
  ///
  /// If the buffer is not on a device (i.e. it is on the host), then DeviceAdapterIdUndefined
  /// is returned.
  ///
  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const;

  VISKORES_CONT BufferInfo();
  VISKORES_CONT ~BufferInfo();

  VISKORES_CONT BufferInfo(const BufferInfo& src);
  VISKORES_CONT BufferInfo(BufferInfo&& src);

  VISKORES_CONT BufferInfo& operator=(const BufferInfo& src);
  VISKORES_CONT BufferInfo& operator=(BufferInfo&& src);

  /// Shallow copy buffer from one host/device to another host/device. Make sure that these
  /// two devices share the same memory space. (This is not checked and will cause badness
  /// if not correct.)
  ///
  VISKORES_CONT BufferInfo(const BufferInfo& src, viskores::cont::DeviceAdapterId device);
  VISKORES_CONT BufferInfo(BufferInfo&& src, viskores::cont::DeviceAdapterId device);

  /// A function callback for deleting the memory.
  ///
  using Deleter = void(void* container);

  /// A function callback for reallocating the memory.
  ///
  using Reallocater = void(void*& memory,
                           void*& container,
                           viskores::BufferSizeType oldSize,
                           viskores::BufferSizeType newSize);

  /// Creates a BufferInfo with the given memory, some (unknown) container holding that memory, a
  /// deletion function, and a reallocation function. The deleter will be called with the pointer
  /// to the container when the buffer is released.
  ///
  VISKORES_CONT BufferInfo(viskores::cont::DeviceAdapterId device,
                           void* memory,
                           void* container,
                           viskores::BufferSizeType size,
                           Deleter deleter,
                           Reallocater reallocater);

  /// Reallocates the buffer to a new size.
  ///
  VISKORES_CONT void Reallocate(viskores::BufferSizeType newSize);

  /// Transfers ownership of the underlying allocation and Deleter and Reallocater to the caller.
  /// After ownership has been transferred this buffer will be equivalant to one that was passed
  /// to Viskores as `view` only.
  ///
  /// This means that the Deleter will do nothing, and the Reallocater will throw an `ErrorBadAllocation`.
  ///
  VISKORES_CONT TransferredBuffer TransferOwnership();

private:
  detail::BufferInfoInternals* Internals;
  viskores::cont::DeviceAdapterId Device;
};


/// Represents the buffer being transferred to external ownership
///
/// The Memory pointer represents the actual data allocation to
/// be used for access and execution
///
/// The container represents what needs to be deleted. This might
/// not be equivalent to \c Memory when we have transferred things
/// such as std::vector
///
struct TransferredBuffer
{
  void* Memory;
  void* Container;
  BufferInfo::Deleter* Delete;
  BufferInfo::Reallocater* Reallocate;
  viskores::BufferSizeType Size;
};

/// Allocates a `BufferInfo` object for the host.
///
VISKORES_CONT_EXPORT VISKORES_CONT viskores::cont::internal::BufferInfo AllocateOnHost(
  viskores::BufferSizeType size);

/// \brief The base class for device adapter memory managers.
///
/// Every device adapter is expected to define a specialization of `DeviceAdapterMemoryManager`,
/// and they are all expected to subclass this base class.
///
class VISKORES_CONT_EXPORT DeviceAdapterMemoryManagerBase
{
public:
  VISKORES_CONT virtual ~DeviceAdapterMemoryManagerBase();

  /// Allocates a buffer of the specified size in bytes and returns a BufferInfo object
  /// containing information about it.
  VISKORES_CONT virtual viskores::cont::internal::BufferInfo Allocate(
    viskores::BufferSizeType size) const = 0;

  /// Reallocates the provided buffer to a new size. The passed in `BufferInfo` should be
  /// modified to reflect the changes.
  VISKORES_CONT void Reallocate(viskores::cont::internal::BufferInfo& buffer,
                                viskores::BufferSizeType newSize) const;

  /// Manages the provided array. Returns a `BufferInfo` object that contains the data.
  VISKORES_CONT BufferInfo
  ManageArray(void* memory,
              void* container,
              viskores::BufferSizeType size,
              viskores::cont::internal::BufferInfo::Deleter deleter,
              viskores::cont::internal::BufferInfo::Reallocater reallocater) const;

  /// Returns the device that this manager is associated with.
  VISKORES_CONT virtual viskores::cont::DeviceAdapterId GetDevice() const = 0;

  /// Copies data from the provided host buffer provided onto the device and returns a buffer info
  /// object holding the pointer for the device.
  VISKORES_CONT virtual viskores::cont::internal::BufferInfo CopyHostToDevice(
    const viskores::cont::internal::BufferInfo& src) const = 0;

  /// Copies data from the provided host buffer into the provided pre-allocated device buffer. The
  /// `BufferInfo` object for the device was created by a previous call to this object.
  VISKORES_CONT virtual void CopyHostToDevice(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const = 0;

  /// Copies data from the device buffer provided to the host. The passed in `BufferInfo` object
  /// was created by a previous call to this object.
  VISKORES_CONT virtual viskores::cont::internal::BufferInfo CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo& src) const = 0;

  /// Copies data from the device buffer provided into the provided pre-allocated host buffer. The
  /// `BufferInfo` object for the device was created by a previous call to this object.
  VISKORES_CONT virtual void CopyDeviceToHost(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const = 0;

  /// Deep copies data from one device buffer to another device buffer. The passed in `BufferInfo`
  /// object was created by a previous call to this object.
  VISKORES_CONT virtual viskores::cont::internal::BufferInfo CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo& src) const = 0;

  /// Deep copies data from one device buffer to another device buffer. The passed in `BufferInfo`
  /// objects were created by a previous call to this object.
  VISKORES_CONT virtual void CopyDeviceToDevice(
    const viskores::cont::internal::BufferInfo& src,
    const viskores::cont::internal::BufferInfo& dest) const = 0;


  /// \brief Low-level method to allocate memory on the device.
  ///
  /// This method allocates an array of the given number of bytes on the device and returns
  /// a void pointer to the array. The preferred method to allocate memory is to use the
  /// `Allocate` method, which returns a `BufferInfo` that manages its own memory. However,
  /// for cases where you are interfacing with code outside of Viskores and need just a raw
  /// pointer, this method can be used. The returned memory can be freed with
  /// `DeleteRawPointer`.
  VISKORES_CONT virtual void* AllocateRawPointer(viskores::BufferSizeType size) const;

  /// \brief Low-level method to copy data on the device.
  ///
  /// This method copies data from one raw pointer to another. It performs the same
  /// function as `CopyDeviceToDevice`, except that it operates on raw pointers
  /// instead of `BufferInfo` objects. This is a useful low-level mechanism to move
  /// data on a device in memory locations created externally to Viskores.
  VISKORES_CONT virtual void CopyDeviceToDeviceRawPointer(const void* src,
                                                          void* dest,
                                                          viskores::BufferSizeType size) const;

  /// \brief Low-level method to delete memory on the device.
  ///
  /// This method takes a pointer to memory allocated on the device and frees it.
  /// The preferred method to delete memory is to use the deallocation routines in
  /// `BufferInfo` objects created with `Allocate`. But for cases where you only
  /// have a raw pointer to the data, this method can be used to manage it. This
  /// method should only be used on memory allocated with this
  /// `DeviceAdaperMemoryManager`.
  VISKORES_CONT virtual void DeleteRawPointer(void*) const = 0;
};

/// \brief The device adapter memory manager.
///
/// Every device adapter is expected to define a specialization of `DeviceAdapterMemoryManager`.
/// This class must be a (perhaps indirect) subclass of `DeviceAdapterMemoryManagerBase`. All
/// abstract methods must be implemented.
///
template <typename DeviceAdapterTag>
class DeviceAdapterMemoryManager;

VISKORES_CONT_EXPORT VISKORES_CONT void HostDeleter(void*);
VISKORES_CONT_EXPORT VISKORES_CONT void* HostAllocate(viskores::BufferSizeType);
VISKORES_CONT_EXPORT VISKORES_CONT void HostReallocate(void*&,
                                                       void*&,
                                                       viskores::BufferSizeType,
                                                       viskores::BufferSizeType);


VISKORES_CONT_EXPORT VISKORES_CONT void InvalidRealloc(void*&,
                                                       void*&,
                                                       viskores::BufferSizeType,
                                                       viskores::BufferSizeType);

// Deletes a container object by casting it to a pointer of a given type (the template argument)
// and then using delete[] on the object.
template <typename T>
VISKORES_CONT inline void SimpleArrayDeleter(void* container_)
{
  T* container = reinterpret_cast<T*>(container_);
  delete[] container;
}

// Reallocates a standard C array. Note that the allocation method is different than the default
// host allocation of viskores::cont::internal::BufferInfo and may be less efficient.
template <typename T>
VISKORES_CONT inline void SimpleArrayReallocater(void*& memory,
                                                 void*& container,
                                                 viskores::BufferSizeType oldSize,
                                                 viskores::BufferSizeType newSize)
{
  VISKORES_ASSERT(memory == container);
  VISKORES_ASSERT(static_cast<std::size_t>(newSize) % sizeof(T) == 0);

  // If the new size is not much smaller than the old size, just reuse the buffer (and waste a
  // little memory).
  if ((newSize > ((3 * oldSize) / 4)) && (newSize <= oldSize))
  {
    return;
  }

  void* newBuffer = new T[static_cast<std::size_t>(newSize) / sizeof(T)];
  std::memcpy(newBuffer, memory, static_cast<std::size_t>(newSize < oldSize ? newSize : oldSize));

  if (memory != nullptr)
  {
    SimpleArrayDeleter<T>(memory);
  }

  memory = container = newBuffer;
}

// Deletes a container object by casting it to a pointer of a given type (the template argument)
// and then using delete on the object.
template <typename T>
VISKORES_CONT inline void CastDeleter(void* container_)
{
  T* container = reinterpret_cast<T*>(container_);
  delete container;
}

template <typename T, typename Allocator>
VISKORES_CONT inline void StdVectorDeleter(void* container)
{
  CastDeleter<std::vector<T, Allocator>>(container);
}

template <typename T, typename Allocator>
VISKORES_CONT inline void StdVectorReallocater(void*& memory,
                                               void*& container,
                                               viskores::BufferSizeType oldSize,
                                               viskores::BufferSizeType newSize)
{
  using vector_type = std::vector<T, Allocator>;
  vector_type* vector = reinterpret_cast<vector_type*>(container);
  VISKORES_ASSERT(vector->empty() || (memory == vector->data()));
  VISKORES_ASSERT(oldSize == static_cast<viskores::BufferSizeType>(vector->size() * sizeof(T)));

  vector->resize(static_cast<std::size_t>(newSize));
  memory = vector->data();
}
}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_DeviceAdapterMemoryManager_h
