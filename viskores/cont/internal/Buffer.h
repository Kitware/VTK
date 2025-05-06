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
#ifndef viskores_viskores_cont_internal_Buffer_h
#define viskores_viskores_cont_internal_Buffer_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/Serialization.h>
#include <viskores/cont/Token.h>

#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

#include <memory>
#include <mutex>

namespace viskores
{

namespace internal
{

///@{
/// \brief Convert the number of values of a type to the number of bytes needed to store it.
///
/// A convenience function that takes the number of values in an array and either the type or the
/// size of the type and safely converts that to the number of bytes required to store the array.
///
/// This function can throw an `viskores::cont::ErrorBadAllocation` if the number of bytes cannot be
/// stored in the returned `viskores::BufferSizeType`. (That would be a huge array and probably
/// indicative of an error.)
///
VISKORES_CONT_EXPORT viskores::BufferSizeType NumberOfValuesToNumberOfBytes(viskores::Id numValues,
                                                                            std::size_t typeSize);

template <typename T>
viskores::BufferSizeType NumberOfValuesToNumberOfBytes(viskores::Id numValues)
{
  return NumberOfValuesToNumberOfBytes(numValues, sizeof(T));
}
///@}

} // namespace internal

namespace cont
{
namespace internal
{

namespace detail
{

struct BufferHelper;

using DeleterType = void(void*);

template <typename T>
void BasicDeleter(void* mem)
{
  T* obj = reinterpret_cast<T*>(mem);
  delete obj;
}

using CopierType = void*(const void*);
template <typename T>
void* BasicCopier(const void* mem)
{
  return new T(*reinterpret_cast<const T*>(mem));
}

} // namespace detail

/// \brief Manages a buffer data among the host and various devices.
///
/// The `Buffer` class defines a contiguous section of memory of a specified number of bytes. The
/// data in this buffer is managed in the host and across the devices supported by Viskores. `Buffer`
/// will allocate memory and transfer data as necessary.
///
class VISKORES_CONT_EXPORT Buffer final
{
  class InternalsStruct;
  std::shared_ptr<InternalsStruct> Internals;

  friend struct viskores::cont::internal::detail::BufferHelper;

public:
  /// \brief Create an empty `Buffer`.
  ///
  VISKORES_CONT Buffer();

  VISKORES_CONT Buffer(const Buffer& src);
  VISKORES_CONT Buffer(Buffer&& src) noexcept;

  VISKORES_CONT ~Buffer();

  VISKORES_CONT Buffer& operator=(const Buffer& src);
  VISKORES_CONT Buffer& operator=(Buffer&& src) noexcept;

  /// \brief Returns the number of bytes held by the buffer.
  ///
  /// Note that `Buffer` allocates memory lazily. So there might not actually be any memory
  /// allocated anywhere. It is also possible that memory is simultaneously allocated on multiple
  /// devices.
  ///
  VISKORES_CONT viskores::BufferSizeType GetNumberOfBytes() const;

  /// \brief Changes the size of the buffer.
  ///
  /// Note that `Buffer` alloates memory lazily. So there might not be any memory allocated at
  /// the return of the call. (However, later calls to retrieve pointers will allocate memory
  /// as necessary.)
  ///
  /// The `preserve` argument flags whether any existing data in the buffer is preserved.
  /// Preserving data might cost more time or memory.
  ///
  VISKORES_CONT void SetNumberOfBytes(viskores::BufferSizeType numberOfBytes,
                                      viskores::CopyFlag preserve,
                                      viskores::cont::Token& token) const;

private:
  VISKORES_CONT bool MetaDataIsType(const std::string& type) const;
  VISKORES_CONT void SetMetaData(void* data,
                                 const std::string& type,
                                 detail::DeleterType* deleter,
                                 detail::CopierType copier) const;
  VISKORES_CONT void* GetMetaData(const std::string& type) const;

public:
  /// \brief Returns whether this `Buffer` holds metadata.
  ///
  VISKORES_CONT bool HasMetaData() const;

  /// \brief Determines if the metadata for the buffer is set to the given type.
  ///
  template <typename MetaDataType>
  VISKORES_CONT bool MetaDataIsType() const
  {
    return this->MetaDataIsType(viskores::cont::TypeToString<MetaDataType>());
  }

  /// \brief Sets the metadata for the buffer.
  ///
  /// Takes an arbitrary object and copies it to the metadata of this buffer. Any existing
  /// metadata is deleted. Any object can be set as the metadata as long as the object has
  /// a default constructor and is copyable.
  ///
  /// Holding metadata in a `Buffer` is optional, but helpful for storing additional
  /// information or objects that cannot be implied by the buffer itself.
  ///
  template <typename MetaDataType>
  VISKORES_CONT void SetMetaData(const MetaDataType& metadata) const
  {
    MetaDataType* metadataCopy = new MetaDataType(metadata);
    this->SetMetaData(metadataCopy,
                      viskores::cont::TypeToString(metadata),
                      detail::BasicDeleter<MetaDataType>,
                      detail::BasicCopier<MetaDataType>);
  }

  /// \brief Gets the metadata for the buffer.
  ///
  /// When you call this method, you have to specify a template parameter for the type
  /// of the metadata. If the metadata has not yet been set in this buffer, a new metadata
  /// object is created, set to this buffer, and returned. If metadata of a different type
  /// has already been set, then an exception is thrown.
  ///
  /// The returned value is a reference that can be manipulated to alter the metadata of
  /// this buffer.
  ///
  template <typename MetaDataType>
  VISKORES_CONT MetaDataType& GetMetaData() const
  {
    if (!this->HasMetaData())
    {
      this->SetMetaData(MetaDataType{});
    }
    return *reinterpret_cast<MetaDataType*>(
      this->GetMetaData(viskores::cont::TypeToString<MetaDataType>()));
  }

  /// \brief Returns `true` if the buffer is allocated on the host.
  ///
  VISKORES_CONT bool IsAllocatedOnHost() const;

  /// \brief Returns `true` if the buffer is allocated on the given device.
  ///
  /// If `device` is `DeviceAdapterTagUnknown`, then this returns the same value as
  /// `IsAllocatedOnHost`. If `device` is `DeviceAdapterTagAny`, then this returns true if
  /// allocated on any device.
  ///
  VISKORES_CONT bool IsAllocatedOnDevice(viskores::cont::DeviceAdapterId device) const;

  /// \brief Returns a readable host (control environment) pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any write operation to this buffer will be
  /// blocked until the `token` goes out of scope.
  ///
  VISKORES_CONT const void* ReadPointerHost(viskores::cont::Token& token) const;

  /// \brief Returns a readable device pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any write operation to this buffer will be
  /// blocked until the `token` goes out of scope.
  ///
  /// If `device` is `DeviceAdapterTagUnknown`, then this has the same behavior as
  /// `ReadPointerHost`. It is an error to set `device` to `DeviceAdapterTagAny`.
  ///
  VISKORES_CONT const void* ReadPointerDevice(viskores::cont::DeviceAdapterId device,
                                              viskores::cont::Token& token) const;

  /// \brief Returns a writable host (control environment) pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any read or write operation to this buffer
  /// will be blocked until the `token` goes out of scope.
  ///
  VISKORES_CONT void* WritePointerHost(viskores::cont::Token& token) const;

  /// \brief Returns a writable device pointer to the buffer.
  ///
  /// Memory will be allocated and data will be copied as necessary. The memory at the pointer will
  /// be valid as long as `token` is still in scope. Any read or write operation to this buffer
  /// will be blocked until the `token` goes out of scope.
  ///
  /// If `device` is `DeviceAdapterTagUnknown`, then this has the same behavior as
  /// `WritePointerHost`. It is an error to set `device` to `DeviceAdapterTagAny`.
  ///
  VISKORES_CONT void* WritePointerDevice(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const;

  /// \brief Enqueue a token for access to the buffer.
  ///
  /// This method places the given `Token` into the queue of `Token`s waiting for
  /// access to this `Buffer` and then returns immediately. When this token
  /// is later used to get data from this `Buffer` (for example, in a call to
  /// `ReadPointerDevice`), it will use this place in the queue while waiting for
  ///
  /// \warning After calling this method it is required to subsequently call a
  /// method that attaches the token to this `Buffer`. Otherwise, the enqueued
  /// token will block any subsequent access to the `ArrayHandle`, even if the
  /// `Token` is destroyed.
  ///
  VISKORES_CONT void Enqueue(const viskores::cont::Token& token) const;

  /// @{
  /// \brief Copies the data from the provided buffer into this buffer.
  ///
  /// If a device is given, then the copy will be preferred for that device. Otherwise, a device
  /// already containing the data will be used for the copy. If no such device exists, the host
  /// will be used.
  ///
  VISKORES_CONT void DeepCopyFrom(const viskores::cont::internal::Buffer& source) const;
  VISKORES_CONT void DeepCopyFrom(const viskores::cont::internal::Buffer& source,
                                  viskores::cont::DeviceAdapterId device) const;
  /// @}

  /// \brief Resets the `Buffer` to the memory allocated at the `BufferInfo`.
  ///
  /// The `Buffer` is initialized to a state that contains the given `buffer` of data. The
  /// `BufferInfo` object self-describes the pointer, size, and device of the memory.
  ///
  /// The given memory is "pinned" in the `Buffer`. This means that this memory will always
  /// be used on the given host or device. If `SetNumberOfBytes` is later called with a size
  /// that is inconsistent with the size of this buffer, an exception will be thrown.
  ///
  VISKORES_CONT void Reset(const viskores::cont::internal::BufferInfo& buffer);

  /// \brief Unallocates the buffer from all devices.
  ///
  /// This method preserves the data on the host even if the data must be transferred
  /// there.
  ///
  /// Note that this method will not physically deallocate memory on a device that shares
  /// a memory space with the host (since the data must be preserved on the host). This
  /// is true even for memory spaces that page data between host and device. This method
  /// will not attempt to unpage data from a device with shared memory.
  ///
  VISKORES_CONT void ReleaseDeviceResources() const;

  /// \brief Gets the `BufferInfo` object to the memory allocated on the host.
  ///
  VISKORES_CONT viskores::cont::internal::BufferInfo GetHostBufferInfo() const;

  /// \brief Gets the `BufferInfo` object to the memory allocated on the given device.
  ///
  /// If the device is `DeviceAdapterTagUndefined`, the pointer for the host is returned. It is
  /// invalid to select `DeviceAdapterTagAny`.
  ///
  VISKORES_CONT viskores::cont::internal::BufferInfo GetDeviceBufferInfo(
    viskores::cont::DeviceAdapterId device) const;

  /// \brief Transfer ownership of the Host `BufferInfo` from this buffer
  /// to the caller. This is used to allow memory owned by Viskores to be
  /// transferred to an owner whose lifespan is longer
  VISKORES_CONT viskores::cont::internal::TransferredBuffer TakeHostBufferOwnership() const;

  /// \brief Transfer ownership of the device `BufferInfo` from this buffer
  /// to the caller. This is used to allow memory owned by Viskores to be
  /// transferred to an owner whose lifespan is longer
  VISKORES_CONT viskores::cont::internal::TransferredBuffer TakeDeviceBufferOwnership(
    viskores::cont::DeviceAdapterId device) const;

  /// \brief Fill up the buffer with particular values.
  ///
  /// Given a short `source` C array (defined on the host), sets all values in the buffer
  /// to that source. The `sourceSize`, in bytes, is also specified. You also specify an
  /// offset to where the fill should `start` and `end`. Values before the `start` and
  /// after the `end` are not affected.
  ///
  /// Both `start` and `end` must be divisible by `sourceSize`.
  ///
  VISKORES_CONT void Fill(const void* source,
                          viskores::BufferSizeType sourceSize,
                          viskores::BufferSizeType start,
                          viskores::BufferSizeType end,
                          viskores::cont::Token& token) const;

  VISKORES_CONT bool operator==(const viskores::cont::internal::Buffer& rhs) const
  {
    return (this->Internals == rhs.Internals);
  }

  VISKORES_CONT bool operator!=(const viskores::cont::internal::Buffer& rhs) const
  {
    return (this->Internals != rhs.Internals);
  }
};

template <typename... ResetArgs>
VISKORES_CONT viskores::cont::internal::Buffer MakeBuffer(ResetArgs&&... resetArgs)
{
  viskores::cont::internal::Buffer buffer;
  buffer.Reset(viskores::cont::internal::BufferInfo(std::forward<ResetArgs>(resetArgs)...));
  return buffer;
}
}
}
} // namespace viskores::cont::internal

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <>
struct VISKORES_CONT_EXPORT Serialization<viskores::cont::internal::Buffer>
{
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::cont::internal::Buffer& obj);
  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::cont::internal::Buffer& obj);
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_viskores_cont_internal_Buffer_h
