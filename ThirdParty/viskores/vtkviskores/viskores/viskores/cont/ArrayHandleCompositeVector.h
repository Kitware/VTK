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
#ifndef viskores_ArrayHandleCompositeVector_h
#define viskores_ArrayHandleCompositeVector_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/StaticAssert.h>
#include <viskores/Tuple.h>
#include <viskores/VecTraits.h>

#include <viskoresstd/integer_sequence.h>

#include <numeric>
#include <type_traits>

namespace viskores
{
namespace internal
{

namespace compvec
{

template <typename... PortalList>
using AllPortalsAreWritable =
  viskores::ListAll<viskores::List<PortalList...>, viskores::internal::PortalSupportsSets>;

// GetValueType: ---------------------------------------------------------------
// Determines the output `ValueType` of the set of `ArrayHandle` objects. For example, if the input
// set contains 3 types with `viskores::Float32` ValueTypes, then the ValueType defined here will be
// `viskores::Vec<Float32, 3>`. This also validates that all members have the same `ValueType`.

template <typename ExpectedValueType, typename ArrayType>
struct CheckValueType
{
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<ExpectedValueType, typename ArrayType::ValueType>::value),
    "ArrayHandleCompositeVector must be built from "
    "ArrayHandles with the same ValueTypes.");
};

template <typename ArrayType0, typename... ArrayTypes>
struct GetValueType
{
  static constexpr viskores::IdComponent COUNT =
    static_cast<viskores::IdComponent>(sizeof...(ArrayTypes)) + 1;
  using ComponentType = typename ArrayType0::ValueType;
  using ValueCheck = viskores::List<CheckValueType<ComponentType, ArrayTypes>...>;
  using ValueType = viskores::Vec<ComponentType, COUNT>;
};

// Special case for only one component
template <typename ArrayType>
struct GetValueType<ArrayType>
{
  static constexpr viskores::IdComponent COUNT = 1;
  using ComponentType = typename ArrayType::ValueType;
  using ValueType = typename ArrayType::ValueType;
};

// SetToPortals: ---------------------------------------------------------------
// Given a Vec-like object, and index, and a set of array portals, sets each of
// the portals to the respective component of the Vec.
VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename ValueType, viskores::IdComponent... I, typename... Portals>
VISKORES_EXEC_CONT void SetToPortalsImpl(viskores::Id index,
                                         const ValueType& value,
                                         viskoresstd::integer_sequence<viskores::IdComponent, I...>,
                                         const Portals&... portals)
{
  using Traits = viskores::VecTraits<ValueType>;
  (void)std::initializer_list<bool>{ (portals.Set(index, Traits::GetComponent(value, I)),
                                      false)... };
}

VISKORES_SUPPRESS_EXEC_WARNINGS
template <typename ValueType, typename... Portals>
VISKORES_EXEC_CONT void SetToPortals(viskores::Id index,
                                     const ValueType& value,
                                     const Portals&... portals)
{
  SetToPortalsImpl(index,
                   value,
                   viskoresstd::make_integer_sequence<viskores::IdComponent,
                                                      viskores::IdComponent(sizeof...(Portals))>{},
                   portals...);
}

} // namespace compvec

template <typename... PortalTypes>
class VISKORES_ALWAYS_EXPORT ArrayPortalCompositeVector
{
  using Writable = compvec::AllPortalsAreWritable<PortalTypes...>;
  using TupleType = viskores::Tuple<PortalTypes...>;
  TupleType Portals;

public:
  using ValueType = typename compvec::GetValueType<PortalTypes...>::ValueType;

  VISKORES_EXEC_CONT
  ArrayPortalCompositeVector() {}

  VISKORES_CONT
  ArrayPortalCompositeVector(const PortalTypes&... portals)
    : Portals(portals...)
  {
  }

  VISKORES_CONT
  ArrayPortalCompositeVector(const TupleType& portals)
    : Portals(portals)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const
  {
    return viskores::Get<0>(this->Portals).GetNumberOfValues();
  }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    auto getFromPortals = [index](const auto&... portals)
    { return ValueType{ portals.Get(index)... }; };
    return this->Portals.Apply(getFromPortals);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    // Note that we are using a lambda function here to implicitly construct a
    // functor to pass to Apply. Some device compilers will not allow passing a
    // function or function pointer to Tuple::Apply.
    auto setToPortal = [index, &value](const auto&... portals)
    { compvec::SetToPortals(index, value, portals...); };
    this->Portals.Apply(setToPortal);
  }
};

}
} // viskores::internal

namespace viskores
{
namespace cont
{
namespace internal
{

namespace compvec
{

template <typename ArrayType>
struct VerifyArrayHandle
{
  VISKORES_STATIC_ASSERT_MSG(viskores::cont::internal::ArrayHandleCheck<ArrayType>::type::value,
                             "Template parameters for ArrayHandleCompositeVector "
                             "must be a list of ArrayHandle types.");
};

} // end namespace compvec

} // namespace internal

template <typename... StorageTags>
struct VISKORES_ALWAYS_EXPORT StorageTagCompositeVec
{
};

namespace internal
{

template <typename... ArrayTs>
struct CompositeVectorTraits
{
  // Need to check this here, since this traits struct is used in the
  // ArrayHandleCompositeVector superclass definition before any other
  // static_asserts could be used.
  using CheckArrayHandles = viskores::List<compvec::VerifyArrayHandle<ArrayTs>...>;

  using ValueType = typename viskores::internal::compvec::GetValueType<ArrayTs...>::ValueType;
  using StorageTag = viskores::cont::StorageTagCompositeVec<typename ArrayTs::StorageTag...>;
  using Superclass = ArrayHandle<ValueType, StorageTag>;
};

template <typename T, typename... StorageTags>
class Storage<viskores::Vec<T, static_cast<viskores::IdComponent>(sizeof...(StorageTags))>,
              viskores::cont::StorageTagCompositeVec<StorageTags...>>
{
  using ValueType = viskores::Vec<T, static_cast<viskores::IdComponent>(sizeof...(StorageTags))>;

  struct Info
  {
    std::array<std::size_t, sizeof...(StorageTags) + 1> BufferOffset;
  };

  template <typename S>
  using StorageFor = viskores::cont::internal::Storage<T, S>;

  using StorageTuple = viskores::Tuple<StorageFor<StorageTags>...>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> GetBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    std::size_t subArray)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(
      buffers.begin() + info.BufferOffset[subArray],
      buffers.begin() + info.BufferOffset[subArray + 1]);
  }

  template <std::size_t I>
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> Buffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetBuffers(buffers, I);
  }

  using IndexList = viskoresstd::make_index_sequence<sizeof...(StorageTags)>;

public:
  using ReadPortalType = viskores::internal::ArrayPortalCompositeVector<
    typename StorageFor<StorageTags>::ReadPortalType...>;
  using WritePortalType = viskores::internal::ArrayPortalCompositeVector<
    typename StorageFor<StorageTags>::WritePortalType...>;

private:
  template <std::size_t... Is>
  static void ResizeBuffersImpl(viskoresstd::index_sequence<Is...>,
                                viskores::Id numValues,
                                const std::vector<viskores::cont::internal::Buffer>& buffers,
                                viskores::CopyFlag preserve,
                                viskores::cont::Token& token)
  {
    std::vector<std::vector<viskores::cont::internal::Buffer>> bufferPartitions = { Buffers<Is>(
      buffers)... };
    auto init_list = { (viskores::tuple_element_t<Is, StorageTuple>::ResizeBuffers(
                          numValues, bufferPartitions[Is], preserve, token),
                        false)... };
    (void)init_list;
  }

  template <std::size_t... Is>
  static void FillImpl(viskoresstd::index_sequence<Is...>,
                       const std::vector<viskores::cont::internal::Buffer>& buffers,
                       const ValueType& fillValue,
                       viskores::Id startIndex,
                       viskores::Id endIndex,
                       viskores::cont::Token& token)
  {
    auto init_list = { (viskores::tuple_element_t<Is, StorageTuple>::Fill(
                          Buffers<Is>(buffers),
                          fillValue[static_cast<viskores::IdComponent>(Is)],
                          startIndex,
                          endIndex,
                          token),
                        false)... };
    (void)init_list;
  }

  template <std::size_t... Is>
  static ReadPortalType CreateReadPortalImpl(
    viskoresstd::index_sequence<Is...>,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(viskores::tuple_element_t<Is, StorageTuple>::CreateReadPortal(
      Buffers<Is>(buffers), device, token)...);
  }

  template <std::size_t... Is>
  static WritePortalType CreateWritePortalImpl(
    viskoresstd::index_sequence<Is...>,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(viskores::tuple_element_t<Is, StorageTuple>::CreateWritePortal(
      Buffers<Is>(buffers), device, token)...);
  }

public:
  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    // Assume that all subcomponents are the same size. Things are not well defined otherwise.
    return viskores::tuple_element_t<0, StorageTuple>::GetNumberOfComponentsFlat(
             GetBuffers(buffers, 0)) *
      ValueType::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return viskores::TupleElement<0, StorageTuple>::GetNumberOfValues(Buffers<0>(buffers));
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    ResizeBuffersImpl(IndexList{}, numValues, buffers, preserve, token);
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const ValueType& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    FillImpl(IndexList{}, buffers, fillValue, startIndex, endIndex, token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return CreateReadPortalImpl(IndexList{}, buffers, device, token);
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return CreateWritePortalImpl(IndexList{}, buffers, device, token);
  }

public:
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const viskores::cont::ArrayHandle<T, StorageTags>&... arrays)
  {
    auto numBuffers = { std::size_t{ 1 }, arrays.GetBuffers().size()... };
    Info info;
    std::partial_sum(numBuffers.begin(), numBuffers.end(), info.BufferOffset.begin());
    return viskores::cont::internal::CreateBuffers(info, arrays...);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return CreateBuffers(viskores::cont::ArrayHandle<T, StorageTags>{}...);
  }

private:
  using ArrayTupleType = viskores::Tuple<viskores::cont::ArrayHandle<T, StorageTags>...>;

  template <std::size_t... Is>
  VISKORES_CONT static ArrayTupleType GetArrayTupleImpl(
    viskoresstd::index_sequence<Is...>,
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ArrayTupleType(viskores::cont::ArrayHandle<T, StorageTags>(Buffers<Is>(buffers))...);
  }

public:
  VISKORES_CONT static ArrayTupleType GetArrayTuple(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return GetArrayTupleImpl(IndexList{}, buffers);
  }
};

// Special degenerative case when there is only one array being composited
template <typename T, typename StorageTag>
struct Storage<T, viskores::cont::StorageTagCompositeVec<StorageTag>> : Storage<T, StorageTag>
{
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const viskores::cont::ArrayHandle<T, StorageTag>& array =
      viskores::cont::ArrayHandle<T, StorageTag>{})
  {
    return viskores::cont::internal::CreateBuffers(array);
  }

  VISKORES_CONT static viskores::Tuple<viskores::cont::ArrayHandle<T, StorageTag>> GetArrayTuple(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return viskores::cont::ArrayHandle<T, StorageTag>(buffers);
  }
};

} // namespace internal

/// @brief An `ArrayHandle` that combines components from other arrays.
///
/// `ArrayHandleCompositeVector` is a specialization of `ArrayHandle` that
/// derives its content from other arrays. It takes any number of
/// single-component \c ArrayHandle objects and mimics an array that contains
/// vectors with components that come from these delegate arrays.
///
/// The easiest way to create and type an `ArrayHandleCompositeVector` is
/// to use the \c make_ArrayHandleCompositeVector functions.
///
/// The `ArrayHandleExtractComponent` class may be helpful when a desired
/// component is part of an `ArrayHandle` with a `viskores::Vec` `ValueType`.
///
/// If you are attempted to combine components that you know are stored in
/// basic `ArrayHandle`s, consider using `ArrayHandleSOA` instead.
///
template <typename... ArrayTs>
class ArrayHandleCompositeVector
  : public ArrayHandle<typename internal::CompositeVectorTraits<ArrayTs...>::ValueType,
                       typename internal::CompositeVectorTraits<ArrayTs...>::StorageTag>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCompositeVector,
    (ArrayHandleCompositeVector<ArrayTs...>),
    (typename internal::CompositeVectorTraits<ArrayTs...>::Superclass));

  /// Construct an `ArrayHandleCompositeVector` from a set of component vectors.
  VISKORES_CONT
  ArrayHandleCompositeVector(const ArrayTs&... arrays)
    : Superclass(StorageType::CreateBuffers(arrays...))
  {
  }

  /// Return the arrays of all of the components in a `viskores::Tuple` object.
  VISKORES_CONT viskores::Tuple<ArrayTs...> GetArrayTuple() const
  {
    return StorageType::GetArrayTuple(this->GetBuffers());
  }
};

/// Create a composite vector array from other arrays.
///
template <typename... ArrayTs>
VISKORES_CONT ArrayHandleCompositeVector<ArrayTs...> make_ArrayHandleCompositeVector(
  const ArrayTs&... arrays)
{
  // Will issue compiler error if any of ArrayTs is not a valid ArrayHandle.
  viskores::List<internal::compvec::VerifyArrayHandle<ArrayTs>...> checkArrayHandles;
  (void)checkArrayHandles;
  return ArrayHandleCompositeVector<ArrayTs...>(arrays...);
}

//--------------------------------------------------------------------------------
// Specialization of ArrayExtractComponent
namespace internal
{

namespace detail
{

template <typename T>
struct ExtractComponentCompositeVecFunctor
{
  using ResultArray =
    viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>;

  ResultArray operator()(viskores::IdComponent, viskores::IdComponent, viskores::CopyFlag) const
  {
    throw viskores::cont::ErrorBadValue("Invalid component index given to ArrayExtractComponent.");
  }

  template <typename A0, typename... As>
  ResultArray operator()(viskores::IdComponent compositeIndex,
                         viskores::IdComponent subIndex,
                         viskores::CopyFlag allowCopy,
                         const A0& array0,
                         const As&... arrays) const
  {
    if (compositeIndex == 0)
    {
      return viskores::cont::internal::ArrayExtractComponentImpl<typename A0::StorageTag>{}(
        array0, subIndex, allowCopy);
    }
    else
    {
      return (*this)(--compositeIndex, subIndex, allowCopy, arrays...);
    }
  }
};

} // namespace detail

// Superclass will inherit the ArrayExtractComponentImplInefficient property if any
// of the sub-storage are inefficient (thus making everything inefficient).
template <typename... StorageTags>
struct ArrayExtractComponentImpl<StorageTagCompositeVec<StorageTags...>>
  : viskores::cont::internal::ArrayExtractComponentImplInherit<StorageTags...>
{
  template <typename VecT>
  auto operator()(
    const viskores::cont::ArrayHandle<VecT, viskores::cont::StorageTagCompositeVec<StorageTags...>>&
      src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    using T = typename viskores::VecTraits<VecT>::ComponentType;
    viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<T, StorageTags>...>
      array(src);
    constexpr viskores::IdComponent NUM_SUB_COMPONENTS = viskores::VecFlat<T>::NUM_COMPONENTS;

    return array.GetArrayTuple().Apply(detail::ExtractComponentCompositeVecFunctor<T>{},
                                       componentIndex / NUM_SUB_COMPONENTS,
                                       componentIndex % NUM_SUB_COMPONENTS,
                                       allowCopy);
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

template <typename... AHs>
struct SerializableTypeString<viskores::cont::ArrayHandleCompositeVector<AHs...>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name =
      "AH_CompositeVector<" + internal::GetVariadicSerializableTypeString(AHs{}...) + ">";
    return name;
  }
};

template <typename T, typename... STs>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Vec<T, static_cast<viskores::IdComponent>(sizeof...(STs))>,
                              viskores::cont::StorageTagCompositeVec<STs...>>>
  : SerializableTypeString<
      viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<T, STs>...>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename... AHs>
struct Serialization<viskores::cont::ArrayHandleCompositeVector<AHs...>>
{
private:
  using Type = typename viskores::cont::ArrayHandleCompositeVector<AHs...>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

  struct SaveFunctor
  {
    BinaryBuffer& Buffer;
    SaveFunctor(BinaryBuffer& bb)
      : Buffer(bb)
    {
    }

    template <typename AH>
    void operator()(const AH& ah) const
    {
      viskoresdiy::save(this->Buffer, ah);
    }
  };

  struct LoadFunctor
  {
    BinaryBuffer& Buffer;
    LoadFunctor(BinaryBuffer& bb)
      : Buffer(bb)
    {
    }

    template <typename AH>
    void operator()(AH& ah) const
    {
      viskoresdiy::load(this->Buffer, ah);
    }
  };

  static BaseType Create(const AHs&... arrays) { return Type(arrays...); }

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type(obj).GetArrayTuple().ForEach(SaveFunctor{ bb });
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::Tuple<AHs...> tuple;
    tuple.ForEach(LoadFunctor{ bb });
    obj = tuple.Apply(Create);
  }
};

template <typename T, typename... STs>
struct Serialization<
  viskores::cont::ArrayHandle<viskores::Vec<T, static_cast<viskores::IdComponent>(sizeof...(STs))>,
                              viskores::cont::StorageTagCompositeVec<STs...>>>
  : Serialization<
      viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<T, STs>...>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //viskores_ArrayHandleCompositeVector_h
