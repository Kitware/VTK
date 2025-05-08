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
#ifndef viskores_cont_UncertainArrayHandle_h
#define viskores_cont_UncertainArrayHandle_h

#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/UnknownArrayHandle.h>

namespace viskores
{
namespace cont
{

/// \brief An ArrayHandle of an uncertain value type and storage.
///
/// `UncertainArrayHandle` holds an `ArrayHandle` object using runtime polymorphism
/// to manage different value and storage types. It behaves like its superclass,
/// `UnknownArrayHandle`, except that it also contains two template parameters that
/// provide `viskores::List`s of potential value and storage types, respectively.
///
/// These potential value and storage types come into play when the `CastAndCall`
/// method is called (or the `UncertainArrayHandle` is used in the
/// `viskores::cont::CastAndCall` function). In this case, the `CastAndCall` will
/// search for `ArrayHandle`s of types that match these two lists.
///
/// Both `UncertainArrayHandle` and `UnknownArrayHandle` have a method named
/// `ResetTypes` that redefine the lists of potential value and storage types
/// by returning a new `UncertainArrayHandle` containing the same `ArrayHandle`
/// but with the new value and storage type lists.
///
template <typename ValueTypeList, typename StorageTypeList>
class VISKORES_ALWAYS_EXPORT UncertainArrayHandle : public viskores::cont::UnknownArrayHandle
{
  VISKORES_IS_LIST(ValueTypeList);
  VISKORES_IS_LIST(StorageTypeList);

  VISKORES_STATIC_ASSERT_MSG((!std::is_same<ValueTypeList, viskores::ListUniversal>::value),
                             "Cannot use viskores::ListUniversal with UncertainArrayHandle.");
  VISKORES_STATIC_ASSERT_MSG((!std::is_same<StorageTypeList, viskores::ListUniversal>::value),
                             "Cannot use viskores::ListUniversal with UncertainArrayHandle.");

  using Superclass = UnknownArrayHandle;
  using Thisclass = UncertainArrayHandle<ValueTypeList, StorageTypeList>;

public:
  VISKORES_CONT UncertainArrayHandle() = default;

  template <typename T, typename S>
  VISKORES_CONT UncertainArrayHandle(const viskores::cont::ArrayHandle<T, S>& array)
    : Superclass(array)
  {
  }

  explicit VISKORES_CONT UncertainArrayHandle(const viskores::cont::UnknownArrayHandle& src)
    : Superclass(src)
  {
  }

  template <typename OtherValues, typename OtherStorage>
  VISKORES_CONT UncertainArrayHandle(const UncertainArrayHandle<OtherValues, OtherStorage>& src)
    : Superclass(src)
  {
  }

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new `UncertainArrayHandle` for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VISKORES_CONT Thisclass NewInstance() const { return Thisclass(this->Superclass::NewInstance()); }

  /// Like `ResetTypes` except it only resets the value types.
  ///
  template <typename NewValueTypeList>
  VISKORES_CONT UncertainArrayHandle<NewValueTypeList, StorageTypeList> ResetValueTypes(
    NewValueTypeList = NewValueTypeList{}) const
  {
    return this->ResetTypes<NewValueTypeList, StorageTypeList>();
  }

  /// Like `ResetTypes` except it only resets the storage types.
  ///
  template <typename NewStorageTypeList>
  VISKORES_CONT UncertainArrayHandle<ValueTypeList, NewStorageTypeList> ResetStorageTypes(
    NewStorageTypeList = NewStorageTypeList{}) const
  {
    return this->ResetTypes<ValueTypeList, NewStorageTypeList>();
  }

  /// \brief Call a functor using the underlying array type.
  ///
  /// `CastAndCall` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array.
  ///
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCall(Functor&& functor, Args&&... args) const
  {
    this->CastAndCallForTypes<ValueTypeList, StorageTypeList>(std::forward<Functor>(functor),
                                                              std::forward<Args>(args)...);
  }

  /// \brief Call a functor using the underlying array type with a float cast fallback.
  ///
  /// `CastAndCallWithFloatFallback()` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array. If the underlying array
  /// does not match any of the requested array types, the array is copied to a new
  /// `ArrayHandleBasic` with `viskores::FloatDefault` components in its value and attempts to
  /// cast to those types.
  ///
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallWithFloatFallback(Functor&& functor, Args&&... args) const
  {
    this->template CastAndCallForTypesWithFloatFallback<ValueTypeList, StorageTypeList>(
      std::forward<Functor>(functor), std::forward<Args>(args)...);
  }
};

// Defined here to avoid circular dependencies between UnknownArrayHandle and UncertainArrayHandle.
template <typename NewValueTypeList, typename NewStorageTypeList>
VISKORES_CONT viskores::cont::UncertainArrayHandle<NewValueTypeList, NewStorageTypeList>
UnknownArrayHandle::ResetTypes(NewValueTypeList, NewStorageTypeList) const
{
  return viskores::cont::UncertainArrayHandle<NewValueTypeList, NewStorageTypeList>(*this);
}

namespace internal
{

template <typename ValueTypeList, typename StorageTypeList>
struct DynamicTransformTraits<viskores::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
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

template <typename ValueTypeList, typename StorageTypeList>
struct SerializableTypeString<viskores::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>>
{
  static VISKORES_CONT std::string Get() { return "UncertainAH"; }
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

namespace internal
{

struct UncertainArrayHandleSerializeFunctor
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& ah, BinaryBuffer& bb) const
  {
    viskoresdiy::save(bb, viskores::cont::SerializableTypeString<ArrayHandleType>::Get());
    viskoresdiy::save(bb, ah);
  }
};

struct UncertainArrayHandleDeserializeFunctor
{
  template <typename T, typename S>
  void operator()(viskores::List<T, S>,
                  viskores::cont::UnknownArrayHandle& unknownArray,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    using ArrayHandleType = viskores::cont::ArrayHandle<T, S>;

    if (!success && (typeString == viskores::cont::SerializableTypeString<ArrayHandleType>::Get()))
    {
      ArrayHandleType knownArray;
      viskoresdiy::load(bb, knownArray);
      unknownArray = knownArray;
      success = true;
    }
  }
};

} // internal

template <typename ValueTypeList, typename StorageTypeList>
struct Serialization<viskores::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>>
{
  using Type = viskores::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(internal::UncertainArrayHandleSerializeFunctor{}, bb);
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    viskoresdiy::load(bb, typeString);

    bool success = false;
    viskores::ListForEach(
      internal::UncertainArrayHandleDeserializeFunctor{},
      viskores::cont::internal::ListAllArrayTypes<ValueTypeList, StorageTypeList>{},
      obj,
      typeString,
      success,
      bb);

    if (!success)
    {
      throw viskores::cont::ErrorBadType(
        "Error deserializing Unknown/UncertainArrayHandle. Message TypeString: " + typeString);
    }
  }
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

#endif //viskores_cont_UncertainArrayHandle_h
