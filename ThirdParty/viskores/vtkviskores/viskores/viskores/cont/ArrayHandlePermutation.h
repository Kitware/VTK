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
#ifndef viskores_cont_ArrayHandlePermutation_h
#define viskores_cont_ArrayHandlePermutation_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorBadValue.h>

namespace viskores
{
namespace internal
{

template <typename IndexPortalType, typename ValuePortalType>
class VISKORES_ALWAYS_EXPORT ArrayPortalPermutation
{
  using Writable = viskores::internal::PortalSupportsSets<ValuePortalType>;

public:
  using ValueType = typename ValuePortalType::ValueType;

  VISKORES_EXEC_CONT
  ArrayPortalPermutation()
    : IndexPortal()
    , ValuePortal()
  {
  }

  VISKORES_EXEC_CONT
  ArrayPortalPermutation(const IndexPortalType& indexPortal, const ValuePortalType& valuePortal)
    : IndexPortal(indexPortal)
    , ValuePortal(valuePortal)
  {
  }

  /// Copy constructor for any other ArrayPortalPermutation with delegate
  /// portal types that can be copied to these portal types. This allows us to
  /// do any type casting that the delegate portals do (like the non-const to
  /// const cast).
  ///
  template <typename OtherIP, typename OtherVP>
  VISKORES_EXEC_CONT ArrayPortalPermutation(const ArrayPortalPermutation<OtherIP, OtherVP>& src)
    : IndexPortal(src.GetIndexPortal())
    , ValuePortal(src.GetValuePortal())
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->IndexPortal.GetNumberOfValues(); }

  VISKORES_EXEC
  ValueType Get(viskores::Id index) const
  {
    viskores::Id permutedIndex = this->IndexPortal.Get(index);
    return this->ValuePortal.Get(permutedIndex);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC void Set(viskores::Id index, const ValueType& value) const
  {
    viskores::Id permutedIndex = this->IndexPortal.Get(index);
    this->ValuePortal.Set(permutedIndex, value);
  }

  VISKORES_EXEC_CONT
  const IndexPortalType& GetIndexPortal() const { return this->IndexPortal; }

  VISKORES_EXEC_CONT
  const ValuePortalType& GetValuePortal() const { return this->ValuePortal; }

private:
  IndexPortalType IndexPortal;
  ValuePortalType ValuePortal;
};
}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

template <typename IndexStorageTag, typename ValueStorageTag>
struct VISKORES_ALWAYS_EXPORT StorageTagPermutation
{
};

namespace internal
{

template <typename T, typename IndexStorageTag, typename ValueStorageTag>
class Storage<T, viskores::cont::StorageTagPermutation<IndexStorageTag, ValueStorageTag>>
{
  VISKORES_STATIC_ASSERT_MSG(
    (viskores::cont::internal::IsValidArrayHandle<viskores::Id, IndexStorageTag>::value),
    "Invalid index storage tag.");
  VISKORES_STATIC_ASSERT_MSG(
    (viskores::cont::internal::IsValidArrayHandle<T, ValueStorageTag>::value),
    "Invalid value storage tag.");

  using IndexStorage = viskores::cont::internal::Storage<viskores::Id, IndexStorageTag>;
  using ValueStorage = viskores::cont::internal::Storage<T, ValueStorageTag>;

  using IndexArray = viskores::cont::ArrayHandle<viskores::Id, IndexStorageTag>;
  using ValueArray = viskores::cont::ArrayHandle<T, ValueStorageTag>;

  struct Info
  {
    std::size_t ValueBufferOffset;
  };

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> IndexBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1,
                                                         buffers.begin() + info.ValueBufferOffset);
  }
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> ValueBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + info.ValueBufferOffset,
                                                         buffers.end());
  }

public:
  VISKORES_STORAGE_NO_RESIZE;

  using ReadPortalType =
    viskores::internal::ArrayPortalPermutation<typename IndexStorage::ReadPortalType,
                                               typename ValueStorage::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalPermutation<typename IndexStorage::ReadPortalType,
                                               typename ValueStorage::WritePortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ValueStorage::GetNumberOfComponentsFlat(ValueBuffers(buffers));
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return IndexStorage::GetNumberOfValues(IndexBuffers(buffers));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const T&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadType("Fill not supported for ArrayHandlePermutation.");
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(IndexStorage::CreateReadPortal(IndexBuffers(buffers), device, token),
                          ValueStorage::CreateReadPortal(ValueBuffers(buffers), device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    // Note: the index portal is always a read-only portal.
    return WritePortalType(IndexStorage::CreateReadPortal(IndexBuffers(buffers), device, token),
                           ValueStorage::CreateWritePortal(ValueBuffers(buffers), device, token));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const IndexArray& indexArray = IndexArray{},
    const ValueArray& valueArray = ValueArray{})
  {
    Info info;
    info.ValueBufferOffset = 1 + indexArray.GetBuffers().size();
    return viskores::cont::internal::CreateBuffers(info, indexArray, valueArray);
  }

  VISKORES_CONT static IndexArray GetIndexArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return IndexArray(IndexBuffers(buffers));
  }

  VISKORES_CONT static ValueArray GetValueArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ValueArray(ValueBuffers(buffers));
  }
};

} // namespace internal

/// \brief Implicitly permutes the values in an array.
///
/// ArrayHandlePermutation is a specialization of ArrayHandle. It takes two
/// delegate array handles: an array of indices and an array of values. The
/// array handle created contains the values given permuted by the indices
/// given. So for a given index i, ArrayHandlePermutation looks up the i-th
/// value in the index array to get permuted index j and then gets the j-th
/// value in the value array. This index permutation is done on the fly rather
/// than creating a copy of the array.
///
/// An ArrayHandlePermutation can be used for either input or output. However,
/// if used for output the array must be pre-allocated. That is, the indices
/// must already be established and the values must have an allocation large
/// enough to accommodate the indices. An output ArrayHandlePermutation will
/// only have values changed. The indices are never changed.
///
/// When using ArrayHandlePermutation great care should be taken to make sure
/// that every index in the index array points to a valid position in the value
/// array. Otherwise, access validations will occur. Also, be wary of duplicate
/// indices that point to the same location in the value array. For input
/// arrays, this is fine. However, this could result in unexpected results for
/// using as output and is almost certainly wrong for using as in-place.
///
template <typename IndexArrayHandleType, typename ValueArrayHandleType>
class ArrayHandlePermutation
  : public viskores::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      viskores::cont::StorageTagPermutation<typename IndexArrayHandleType::StorageTag,
                                            typename ValueArrayHandleType::StorageTag>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VISKORES_IS_ARRAY_HANDLE(IndexArrayHandleType);
  VISKORES_IS_ARRAY_HANDLE(ValueArrayHandleType);

  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<viskores::Id, typename IndexArrayHandleType::ValueType>::value),
    "Permutation array in ArrayHandlePermutation must have viskores::Id value type.");

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandlePermutation,
    (ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>),
    (viskores::cont::ArrayHandle<
      typename ValueArrayHandleType::ValueType,
      viskores::cont::StorageTagPermutation<typename IndexArrayHandleType::StorageTag,
                                            typename ValueArrayHandleType::StorageTag>>));

  /// Construct a permuation array with index and value arrays.
  VISKORES_CONT
  ArrayHandlePermutation(const IndexArrayHandleType& indexArray,
                         const ValueArrayHandleType& valueArray)
    : Superclass(StorageType::CreateBuffers(indexArray, valueArray))
  {
  }

  /// @brief Return the array used for indices.
  ///
  /// The index array provides how indices get permuted. When a value is retrieved from an
  /// `ArrayHandlePermutation`, an index is retrived from this index array, and this new
  /// index is used to retrieve a value from the value array.
  VISKORES_CONT IndexArrayHandleType GetIndexArray() const
  {
    return StorageType::GetIndexArray(this->GetBuffers());
  }

  /// @brief Return the array used for values.
  ///
  /// The index array provides how indices get permuted. When a value is retrieved from an
  /// `ArrayHandlePermutation`, an index is retrived from this index array, and this new
  /// index is used to retrieve a value from the value array.
  VISKORES_CONT ValueArrayHandleType GetValueArray() const
  {
    return StorageType::GetValueArray(this->GetBuffers());
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.
template <typename IndexArrayHandleType, typename ValueArrayHandleType>
VISKORES_CONT viskores::cont::ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>
make_ArrayHandlePermutation(IndexArrayHandleType indexArray, ValueArrayHandleType valueArray)
{
  return ArrayHandlePermutation<IndexArrayHandleType, ValueArrayHandleType>(indexArray, valueArray);
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

template <typename IdxAH, typename ValAH>
struct SerializableTypeString<viskores::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Permutation<" + SerializableTypeString<IdxAH>::Get() + "," +
      SerializableTypeString<ValAH>::Get() + ">";
    return name;
  }
};

template <typename T, typename IdxST, typename ValST>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagPermutation<IdxST, ValST>>>
  : SerializableTypeString<
      viskores::cont::ArrayHandlePermutation<viskores::cont::ArrayHandle<viskores::Id, IdxST>,
                                             viskores::cont::ArrayHandle<T, ValST>>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename IdxAH, typename ValAH>
struct Serialization<viskores::cont::ArrayHandlePermutation<IdxAH, ValAH>>
{
private:
  using Type = viskores::cont::ArrayHandlePermutation<IdxAH, ValAH>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, Type(obj).GetIndexArray());
    viskoresdiy::save(bb, Type(obj).GetValueArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    IdxAH indices;
    ValAH values;

    viskoresdiy::load(bb, indices);
    viskoresdiy::load(bb, values);

    obj = viskores::cont::make_ArrayHandlePermutation(indices, values);
  }
};

template <typename T, typename IdxST, typename ValST>
struct Serialization<
  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagPermutation<IdxST, ValST>>>
  : Serialization<
      viskores::cont::ArrayHandlePermutation<viskores::cont::ArrayHandle<viskores::Id, IdxST>,
                                             viskores::cont::ArrayHandle<T, ValST>>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandlePermutation_h
