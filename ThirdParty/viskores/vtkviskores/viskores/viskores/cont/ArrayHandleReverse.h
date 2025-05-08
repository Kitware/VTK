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

#ifndef viskores_cont_ArrayHandleReverse_h
#define viskores_cont_ArrayHandleReverse_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorBadValue.h>

namespace viskores
{
namespace cont
{

namespace internal
{

template <typename PortalType>
class VISKORES_ALWAYS_EXPORT ArrayPortalReverse
{
  using Writable = viskores::internal::PortalSupportsSets<PortalType>;

public:
  using ValueType = typename PortalType::ValueType;

  VISKORES_EXEC_CONT
  ArrayPortalReverse()
    : portal()
  {
  }

  VISKORES_EXEC_CONT
  ArrayPortalReverse(const PortalType& p)
    : portal(p)
  {
  }

  template <typename OtherPortal>
  VISKORES_EXEC_CONT ArrayPortalReverse(const ArrayPortalReverse<OtherPortal>& src)
    : portal(src.GetPortal())
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->portal.GetNumberOfValues(); }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    return this->portal.Get(portal.GetNumberOfValues() - index - 1);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    this->portal.Set(portal.GetNumberOfValues() - index - 1, value);
  }

private:
  PortalType portal;
};
}

template <typename StorageTag>
class VISKORES_ALWAYS_EXPORT StorageTagReverse
{
};

namespace internal
{

template <typename T, typename ST>
class Storage<T, StorageTagReverse<ST>>
{
  using SourceStorage = Storage<T, ST>;

public:
  using ArrayHandleType = viskores::cont::ArrayHandle<T, ST>;
  using ReadPortalType = ArrayPortalReverse<typename ArrayHandleType::ReadPortalType>;
  using WritePortalType = ArrayPortalReverse<typename ArrayHandleType::WritePortalType>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return SourceStorage::CreateBuffers();
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    SourceStorage::ResizeBuffers(numValues, buffers, preserve, token);
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfComponentsFlat(buffers);
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(buffers);
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const T& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    viskores::Id numValues = GetNumberOfValues(buffers);
    SourceStorage::Fill(buffers, fillValue, numValues - endIndex, numValues - startIndex, token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(SourceStorage::CreateReadPortal(buffers, device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(SourceStorage::CreateWritePortal(buffers, device, token));
  }
}; // class storage

} // namespace internal

/// \brief Reverse the order of an array, on demand.
///
/// ArrayHandleReverse is a specialization of ArrayHandle. Given an ArrayHandle,
/// it creates a new handle that returns the elements of the array in reverse
/// order (i.e. from end to beginning).
///
template <typename ArrayHandleType>
class ArrayHandleReverse
  : public viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                       StorageTagReverse<typename ArrayHandleType::StorageTag>>

{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleReverse,
    (ArrayHandleReverse<ArrayHandleType>),
    (viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                 StorageTagReverse<typename ArrayHandleType::StorageTag>>));

  ArrayHandleReverse(const ArrayHandleType& handle)
    : Superclass(handle.GetBuffers())
  {
  }

  VISKORES_CONT ArrayHandleType GetSourceArray() const
  {
    return viskores::cont::ArrayHandle<ValueType, typename ArrayHandleType::StorageTag>(
      this->GetBuffers());
  }
};

/// make_ArrayHandleReverse is convenience function to generate an
/// ArrayHandleReverse.
///
template <typename HandleType>
VISKORES_CONT ArrayHandleReverse<HandleType> make_ArrayHandleReverse(const HandleType& handle)
{
  return ArrayHandleReverse<HandleType>(handle);
}

namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if
// the sub-storage is inefficient (thus making everything inefficient).
template <typename StorageTag>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagReverse<StorageTag>>
  : viskores::cont::internal::ArrayExtractComponentImpl<StorageTag>
{
  template <typename T>
  using StrideArrayType =
    viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>;

  template <typename T>
  StrideArrayType<T> operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagReverse<StorageTag>>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<T, StorageTag>> srcArray(src);
    StrideArrayType<T> subArray =
      ArrayExtractComponentImpl<StorageTag>{}(srcArray.GetSourceArray(), componentIndex, allowCopy);
    // Reverse the array by starting at the end and striding backward
    return StrideArrayType<T>(subArray.GetBasicArray(),
                              srcArray.GetNumberOfValues(),
                              -subArray.GetStride(),
                              subArray.GetOffset() +
                                (subArray.GetStride() * (subArray.GetNumberOfValues() - 1)),
                              subArray.GetModulo(),
                              subArray.GetDivisor());
  }
};

} // namespace internal

}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename AH>
struct SerializableTypeString<viskores::cont::ArrayHandleReverse<AH>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Reverse<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST>
struct SerializableTypeString<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagReverse<ST>>>
  : SerializableTypeString<viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<T, ST>>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<viskores::cont::ArrayHandleReverse<AH>>
{
private:
  using Type = viskores::cont::ArrayHandleReverse<AH>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    viskoresdiy::save(bb, obj.GetSourceArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    viskoresdiy::load(bb, array);
    obj = viskores::cont::make_ArrayHandleReverse(array);
  }
};

template <typename T, typename ST>
struct Serialization<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagReverse<ST>>>
  : Serialization<viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<T, ST>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // viskores_cont_ArrayHandleReverse_h
