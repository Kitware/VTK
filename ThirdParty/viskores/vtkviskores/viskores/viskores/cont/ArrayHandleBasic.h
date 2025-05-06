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
#ifndef viskores_cont_ArrayHandleBasic_h
#define viskores_cont_ArrayHandleBasic_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/SerializableTypeString.h>
#include <viskores/cont/Serialization.h>
#include <viskores/cont/Storage.h>

#include <viskores/internal/ArrayPortalBasic.h>

#include <viskores/VecFlat.h>

#include <limits>

namespace viskores
{
namespace cont
{

namespace internal
{

template <typename T>
class VISKORES_ALWAYS_EXPORT Storage<T, viskores::cont::StorageTagBasic>
{
public:
  using ReadPortalType = viskores::internal::ArrayPortalBasicRead<T>;
  using WritePortalType = viskores::internal::ArrayPortalBasicWrite<T>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return std::vector<viskores::cont::internal::Buffer>(1);
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    buffers[0].SetNumberOfBytes(
      viskores::internal::NumberOfValuesToNumberOfBytes<T>(numValues), preserve, token);
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<T>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    return static_cast<viskores::Id>(buffers[0].GetNumberOfBytes() /
                                     static_cast<viskores::BufferSizeType>(sizeof(T)));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const T& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    constexpr viskores::BufferSizeType fillValueSize =
      static_cast<viskores::BufferSizeType>(sizeof(fillValue));
    buffers[0].Fill(
      &fillValue, fillValueSize, startIndex * fillValueSize, endIndex * fillValueSize, token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    return ReadPortalType(reinterpret_cast<const T*>(buffers[0].ReadPointerDevice(device, token)),
                          GetNumberOfValues(buffers));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    VISKORES_ASSERT(buffers.size() == 1);
    return WritePortalType(reinterpret_cast<T*>(buffers[0].WritePointerDevice(device, token)),
                           GetNumberOfValues(buffers));
  }
};

} // namespace internal

/// @brief Basic array storage for an array handle.
///
/// This array handle references a standard C array. It provides a level
/// of safety and management across devices.
/// This is the default used when no storage is specified. Using this subclass
/// allows access to the underlying raw array.
template <typename T>
class VISKORES_ALWAYS_EXPORT ArrayHandleBasic
  : public ArrayHandle<T, viskores::cont::StorageTagBasic>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleBasic,
                                 (ArrayHandleBasic<T>),
                                 (ArrayHandle<T, viskores::cont::StorageTagBasic>));

  ArrayHandleBasic(
    T* array,
    viskores::Id numberOfValues,
    viskores::cont::internal::BufferInfo::Deleter deleter,
    viskores::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(
        std::vector<viskores::cont::internal::Buffer>{ viskores::cont::internal::MakeBuffer(
          viskores::cont::DeviceAdapterTagUndefined{},
          array,
          array,
          viskores::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
          deleter,
          reallocater) })
  {
  }

  ArrayHandleBasic(
    T* array,
    viskores::Id numberOfValues,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::internal::BufferInfo::Deleter deleter,
    viskores::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(
        std::vector<viskores::cont::internal::Buffer>{ viskores::cont::internal::MakeBuffer(
          device,
          array,
          array,
          viskores::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
          deleter,
          reallocater) })
  {
  }

  ArrayHandleBasic(
    T* array,
    void* container,
    viskores::Id numberOfValues,
    viskores::cont::internal::BufferInfo::Deleter deleter,
    viskores::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(
        std::vector<viskores::cont::internal::Buffer>{ viskores::cont::internal::MakeBuffer(
          viskores::cont::DeviceAdapterTagUndefined{},
          array,
          container,
          viskores::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
          deleter,
          reallocater) })
  {
  }

  ArrayHandleBasic(
    T* array,
    void* container,
    viskores::Id numberOfValues,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::internal::BufferInfo::Deleter deleter,
    viskores::cont::internal::BufferInfo::Reallocater reallocater = internal::InvalidRealloc)
    : Superclass(
        std::vector<viskores::cont::internal::Buffer>{ viskores::cont::internal::MakeBuffer(
          device,
          array,
          container,
          viskores::internal::NumberOfValuesToNumberOfBytes<T>(numberOfValues),
          deleter,
          reallocater) })
  {
  }

  /// @brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  const T* GetReadPointer() const
  {
    viskores::cont::Token token;
    return this->GetReadPointer(token);
  }
  /// @brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// @param token When a `viskores::cont::Token` is provided, the array is locked
  /// from being used by any write operations until the token goes out of scope.
  ///
  const T* GetReadPointer(viskores::cont::Token& token) const
  {
    return reinterpret_cast<const T*>(this->GetBuffers()[0].ReadPointerHost(token));
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  T* GetWritePointer() const
  {
    viskores::cont::Token token;
    return this->GetWritePointer(token);
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// @param token When a `viskores::cont::Token` is provided, the array is locked
  /// from being used by any read or write operations until the token goes out of scope.
  ///
  T* GetWritePointer(viskores::cont::Token& token) const
  {
    return reinterpret_cast<T*>(this->GetBuffers()[0].WritePointerHost(token));
  }

  /// @brief Gets raw access to the `ArrayHandle`'s data on a particular device.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  ///
  const T* GetReadPointer(viskores::cont::DeviceAdapterId device) const
  {
    viskores::cont::Token token;
    return this->GetReadPointer(device, token);
  }
  /// @brief Gets raw access to the `ArrayHandle`'s data.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  /// @param token When a `viskores::cont::Token` is provided, the array is locked
  /// from being used by any write operations until the token goes out of scope.
  ///
  const T* GetReadPointer(viskores::cont::DeviceAdapterId device,
                          viskores::cont::Token& token) const
  {
    return reinterpret_cast<const T*>(this->GetBuffers()[0].ReadPointerDevice(device, token));
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// Note that the returned array may become invalidated by other operations on the ArryHandle.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  ///
  T* GetWritePointer(viskores::cont::DeviceAdapterId device) const
  {
    viskores::cont::Token token;
    return this->GetWritePointer(device, token);
  }
  /// @brief Gets raw write access to the `ArrayHandle`'s data.
  ///
  /// @param device The device ID or device tag specifying on which device the array will
  /// be valid on.
  /// @param token When a `viskores::cont::Token` is provided, the array is locked
  /// from being used by any read or write operations until the token goes out of scope.
  ///
  T* GetWritePointer(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return reinterpret_cast<T*>(this->GetBuffers()[0].WritePointerDevice(device, token));
  }
};

/// A convenience function for creating an ArrayHandle from a standard C array.
///
template <typename T>
VISKORES_CONT viskores::cont::ArrayHandleBasic<T> make_ArrayHandle(const T* array,
                                                                   viskores::Id numberOfValues,
                                                                   viskores::CopyFlag copy)
{
  if (copy == viskores::CopyFlag::On)
  {
    viskores::cont::ArrayHandleBasic<T> handle;
    handle.Allocate(numberOfValues);
    std::copy(array,
              array + numberOfValues,
              viskores::cont::ArrayPortalToIteratorBegin(handle.WritePortal()));
    return handle;
  }
  else
  {
    return viskores::cont::ArrayHandleBasic<T>(const_cast<T*>(array), numberOfValues, [](void*) {});
  }
}

/// A convenience function to move a user-allocated array into an `ArrayHandle`.
/// The provided array pointer will be reset to `nullptr`.
/// If the array was not allocated with the `new[]` operator, then deleter and reallocater
/// functions must be provided.
///
template <typename T>
VISKORES_CONT viskores::cont::ArrayHandleBasic<T> make_ArrayHandleMove(
  T*& array,
  viskores::Id numberOfValues,
  viskores::cont::internal::BufferInfo::Deleter deleter = internal::SimpleArrayDeleter<T>,
  viskores::cont::internal::BufferInfo::Reallocater reallocater =
    internal::SimpleArrayReallocater<T>)
{
  viskores::cont::ArrayHandleBasic<T> arrayHandle(array, numberOfValues, deleter, reallocater);
  array = nullptr;
  return arrayHandle;
}

/// A convenience function for creating an ArrayHandle from an std::vector.
///
template <typename T, typename Allocator>
VISKORES_CONT viskores::cont::ArrayHandleBasic<T> make_ArrayHandle(
  const std::vector<T, Allocator>& array,
  viskores::CopyFlag copy)
{
  if (!array.empty())
  {
    return make_ArrayHandle(array.data(), static_cast<viskores::Id>(array.size()), copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>();
  }
}

/// Move an std::vector into an ArrayHandle.
///
template <typename T, typename Allocator>
VISKORES_CONT viskores::cont::ArrayHandleBasic<T> make_ArrayHandleMove(
  std::vector<T, Allocator>&& array)
{
  using vector_type = std::vector<T, Allocator>;
  vector_type* container = new vector_type(std::move(array));
  return viskores::cont::ArrayHandleBasic<T>(container->data(),
                                             container,
                                             static_cast<viskores::Id>(container->size()),
                                             internal::StdVectorDeleter<T, Allocator>,
                                             internal::StdVectorReallocater<T, Allocator>);
}

/// Move an std::vector into an ArrayHandle.
///
template <typename T, typename Allocator>
VISKORES_CONT viskores::cont::ArrayHandleBasic<T> make_ArrayHandle(
  std::vector<T, Allocator>&& array,
  viskores::CopyFlag viskoresNotUsed(copy))
{
  return make_ArrayHandleMove(array);
}

/// Create an ArrayHandle directly from an initializer list of values.
///
template <typename T>
VISKORES_CONT viskores::cont::ArrayHandleBasic<T> make_ArrayHandle(
  std::initializer_list<T>&& values)
{
  return make_ArrayHandle(
    values.begin(), static_cast<viskores::Id>(values.size()), viskores::CopyFlag::On);
}
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleBasic<T>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<ArrayHandle<T, viskores::cont::StorageTagBasic>>
  : SerializableTypeString<viskores::cont::ArrayHandleBasic<T>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<viskores::cont::ArrayHandleBasic<T>>
{
  static VISKORES_CONT void save(
    BinaryBuffer& bb,
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& obj)
  {
    viskoresdiy::save(bb, obj.GetBuffers()[0]);
  }

  static VISKORES_CONT void load(
    BinaryBuffer& bb,
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& obj)
  {
    viskores::cont::internal::Buffer buffer;
    viskoresdiy::load(bb, buffer);

    obj = viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>(
      viskores::cont::internal::CreateBuffers(buffer));
  }
};

template <typename T>
struct Serialization<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>>
  : Serialization<viskores::cont::ArrayHandleBasic<T>>
{
};

} // diy
/// @endcond SERIALIZATION

#ifndef viskores_cont_ArrayHandleBasic_cxx

/// @cond
/// Make doxygen ignore this section

namespace viskores
{
namespace cont
{

namespace internal
{

#define VISKORES_STORAGE_EXPORT(Type)                                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT Storage<Type, StorageTagBasic>; \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                 \
    Storage<viskores::Vec<Type, 2>, StorageTagBasic>;                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                 \
    Storage<viskores::Vec<Type, 3>, StorageTagBasic>;                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                 \
    Storage<viskores::Vec<Type, 4>, StorageTagBasic>;

VISKORES_STORAGE_EXPORT(char)
VISKORES_STORAGE_EXPORT(viskores::Int8)
VISKORES_STORAGE_EXPORT(viskores::UInt8)
VISKORES_STORAGE_EXPORT(viskores::Int16)
VISKORES_STORAGE_EXPORT(viskores::UInt16)
VISKORES_STORAGE_EXPORT(viskores::Int32)
VISKORES_STORAGE_EXPORT(viskores::UInt32)
VISKORES_STORAGE_EXPORT(viskores::Int64)
VISKORES_STORAGE_EXPORT(viskores::UInt64)
VISKORES_STORAGE_EXPORT(viskores::Float32)
VISKORES_STORAGE_EXPORT(viskores::Float64)

#undef VISKORES_STORAGE_EXPORT

} // namespace internal

#define VISKORES_ARRAYHANDLE_EXPORT(Type)                                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>; \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                     \
    ArrayHandle<viskores::Vec<Type, 2>, StorageTagBasic>;                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                     \
    ArrayHandle<viskores::Vec<Type, 3>, StorageTagBasic>;                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                     \
    ArrayHandle<viskores::Vec<Type, 4>, StorageTagBasic>;

VISKORES_ARRAYHANDLE_EXPORT(char)
VISKORES_ARRAYHANDLE_EXPORT(viskores::Int8)
VISKORES_ARRAYHANDLE_EXPORT(viskores::UInt8)
VISKORES_ARRAYHANDLE_EXPORT(viskores::Int16)
VISKORES_ARRAYHANDLE_EXPORT(viskores::UInt16)
VISKORES_ARRAYHANDLE_EXPORT(viskores::Int32)
VISKORES_ARRAYHANDLE_EXPORT(viskores::UInt32)
VISKORES_ARRAYHANDLE_EXPORT(viskores::Int64)
VISKORES_ARRAYHANDLE_EXPORT(viskores::UInt64)
VISKORES_ARRAYHANDLE_EXPORT(viskores::Float32)
VISKORES_ARRAYHANDLE_EXPORT(viskores::Float64)

#undef VISKORES_ARRAYHANDLE_EXPORT
}
} // end viskores::cont

/// @endcond

#endif // !viskores_cont_ArrayHandleBasic_cxx

#endif //viskores_cont_ArrayHandleBasic_h
