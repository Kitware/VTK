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
#ifndef viskores_cont_ArrayHandleSwizzle_h
#define viskores_cont_ArrayHandleSwizzle_h

#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/VecTraits.h>

#include <viskoresstd/integer_sequence.h>

namespace viskores
{
namespace internal
{

template <typename InType, typename OutType>
class SwizzleFunctor
{
  using InTraits = viskores::VecTraits<InType>;
  using InComponentType = typename InTraits::ComponentType;
  static constexpr viskores::IdComponent NUM_IN_COMPONENTS = InTraits::NUM_COMPONENTS;

  using OutTraits = viskores::VecTraits<OutType>;
  using OutComponentType = typename OutTraits::ComponentType;
  static constexpr viskores::IdComponent NUM_OUT_COMPONENTS = OutTraits::NUM_COMPONENTS;

  template <viskores::IdComponent... Is>
  using IndexSequence = viskoresstd::integer_sequence<viskores::IdComponent, Is...>;
  using IndexList = viskoresstd::make_integer_sequence<viskores::IdComponent, NUM_OUT_COMPONENTS>;

public:
  using MapType = viskores::Vec<viskores::IdComponent, NUM_OUT_COMPONENTS>;

  VISKORES_CONT SwizzleFunctor(const MapType& map)
    : Map(map)
  {
  }

  VISKORES_CONT SwizzleFunctor() = default;

  VISKORES_EXEC_CONT OutType operator()(const InType& vec) const
  {
    return this->Swizzle(vec, IndexList{});
  }

  VISKORES_CONT static MapType InitMap() { return IndexListAsMap(IndexList{}); }

private:
  template <viskores::IdComponent... Is>
  VISKORES_CONT static MapType IndexListAsMap(IndexSequence<Is...>)
  {
    return { Is... };
  }

  template <viskores::IdComponent... Is>
  VISKORES_EXEC_CONT OutType Swizzle(const InType& vec, IndexSequence<Is...>) const
  {
    return { InTraits::GetComponent(vec, this->Map[Is])... };
  }

  MapType Map = InitMap();
};

namespace detail
{

template <typename InType, typename OutType, typename Invertible>
struct GetInverseSwizzleImpl;

template <typename InType, typename OutType>
struct GetInverseSwizzleImpl<InType, OutType, std::true_type>
{
  using Type = viskores::internal::SwizzleFunctor<OutType, InType>;
  template <typename ForwardMapType>
  VISKORES_CONT static Type Value(const ForwardMapType& forwardMap)
  {
    // Note that when reversing the map, if the forwardMap repeats any indices, then
    // the map is not 1:1 and is not invertible. We cannot check that at compile time.
    // In this case, results can become unpredictible.
    using InverseMapType = typename Type::MapType;
    InverseMapType inverseMap = Type::InitMap();
    for (viskores::IdComponent inIndex = 0; inIndex < ForwardMapType::NUM_COMPONENTS; ++inIndex)
    {
      inverseMap[forwardMap[inIndex]] = inIndex;
    }

    return Type(inverseMap);
  }
};

template <typename InType, typename OutType>
struct GetInverseSwizzleImpl<InType, OutType, std::false_type>
{
  using Type = viskores::cont::internal::NullFunctorType;
  template <typename ForwardMapType>
  VISKORES_CONT static Type Value(const ForwardMapType&)
  {
    return Type{};
  }
};

template <typename InType, typename OutType>
using SwizzleInvertible = std::integral_constant<bool,
                                                 viskores::VecTraits<InType>::NUM_COMPONENTS ==
                                                   viskores::VecTraits<OutType>::NUM_COMPONENTS>;

} // namespace detail

template <typename InType, typename OutType>
VISKORES_CONT viskores::internal::SwizzleFunctor<InType, OutType> GetSwizzleFunctor(
  const typename viskores::internal::SwizzleFunctor<InType, OutType>::MapType& forwardMap)
{
  return viskores::internal::SwizzleFunctor<InType, OutType>(forwardMap);
}

template <typename InType, typename OutType>
using InverseSwizzleType = typename detail::
  GetInverseSwizzleImpl<InType, OutType, detail::SwizzleInvertible<InType, OutType>>::Type;

template <typename InType, typename OutType>
VISKORES_CONT InverseSwizzleType<InType, OutType> GetInverseSwizzleFunctor(
  const typename viskores::internal::SwizzleFunctor<InType, OutType>::MapType& forwardMap)
{
  return detail::
    GetInverseSwizzleImpl<InType, OutType, detail::SwizzleInvertible<InType, OutType>>::Value(
      forwardMap);
}

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename ArrayHandleType, viskores::IdComponent OutSize>
struct ArrayHandleSwizzleTraits
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  using InType = typename ArrayHandleType::ValueType;
  using OutType = viskores::Vec<typename viskores::VecTraits<InType>::ComponentType, OutSize>;
  using SwizzleFunctor = viskores::internal::SwizzleFunctor<InType, OutType>;
  using InverseSwizzleFunctor = viskores::internal::InverseSwizzleType<InType, OutType>;
  using MapType = typename SwizzleFunctor::MapType;

  static SwizzleFunctor GetFunctor(const MapType& forwardMap)
  {
    return viskores::internal::GetSwizzleFunctor<InType, OutType>(forwardMap);
  }

  static InverseSwizzleFunctor GetInverseFunctor(const MapType& forwardMap)
  {
    return viskores::internal::GetInverseSwizzleFunctor<InType, OutType>(forwardMap);
  }

  using Superclass =
    viskores::cont::ArrayHandleTransform<ArrayHandleType, SwizzleFunctor, InverseSwizzleFunctor>;
};

} // namespace detail

/// \brief Swizzle the components of the values in an `ArrayHandle`.
///
/// Given an `ArrayHandle` with `Vec` values, `ArrayHandleSwizzle` allows you to
/// reorder the components of all the `Vec` values. This reordering is done in place,
/// so the array does not have to be duplicated.
///
/// The resulting array does not have to contain all of the components of the input.
/// For example, you could use `ArrayHandleSwizzle` to drop one of the components of
/// each vector. However, if you do that, then the swizzled array is read-only. If
/// there is a 1:1 map from input components to output components, writing to the
/// array will be enabled.
///
/// The swizzle map given to `ArrayHandleSwizzle` must comprise valid component indices
/// (between 0 and number of components - 1). Also, the component indices should not
/// be repeated, particularly if you expect to write to the array. These conditions are
/// not checked.
///
template <typename ArrayHandleType, viskores::IdComponent OutSize>
class ArrayHandleSwizzle
  : public detail::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>::Superclass
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  using Traits = detail::ArrayHandleSwizzleTraits<ArrayHandleType, OutSize>;

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleSwizzle,
                                 (ArrayHandleSwizzle<ArrayHandleType, OutSize>),
                                 (typename Traits::Superclass));

  using MapType = typename Traits::MapType;

  /// Construct an `ArrayHandleSwizzle` with a source array and a swizzle map.
  /// The swizzle map is a `viskores::Vec` containing `viskores::IdComponent` components
  /// and sized to the number of components in the array. Each value in the map
  /// specifies from which component of the input the corresponding component of
  /// the output should come from.
  VISKORES_CONT ArrayHandleSwizzle(const ArrayHandleType& array, const MapType& map)
    : Superclass(array, Traits::GetFunctor(map), Traits::GetInverseFunctor(map))
  {
  }
};

/// Construct an `ArrayHandleSwizzle` from a provided array and swizzle map.
/// The swizzle map is a `viskores::Vec` containing `viskores::IdComponent` components
/// and sized to the number of components in the array. Each value in the map
/// specifies from which component of the input the corresponding component of
/// the output should come from.
template <typename ArrayHandleType, viskores::IdComponent OutSize>
VISKORES_CONT ArrayHandleSwizzle<ArrayHandleType, OutSize> make_ArrayHandleSwizzle(
  const ArrayHandleType& array,
  const viskores::Vec<viskores::IdComponent, OutSize>& map)
{
  return ArrayHandleSwizzle<ArrayHandleType, OutSize>(array, map);
}

/// Construct an `ArrayHandleSwizzle` from a provided array and swizzle map.
/// The swizzle map is specified as independent function parameters after the array.
/// Each value in the map specifies from which component of the input the corresponding
/// component of the output should come from.
template <typename ArrayHandleType, typename... SwizzleIndexTypes>
VISKORES_CONT auto make_ArrayHandleSwizzle(const ArrayHandleType& array,
                                           viskores::IdComponent swizzleIndex0,
                                           SwizzleIndexTypes... swizzleIndices)
{
  return make_ArrayHandleSwizzle(array, viskores::make_Vec(swizzleIndex0, swizzleIndices...));
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

template <typename InType, typename OutType>
struct SerializableTypeString<viskores::internal::SwizzleFunctor<InType, OutType>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "Swizzle<" + SerializableTypeString<InType>::Get() + "," +
      SerializableTypeString<OutType>::Get() + ">";
    return name;
  }
};

template <typename AH, viskores::IdComponent NComps>
struct SerializableTypeString<viskores::cont::ArrayHandleSwizzle<AH, NComps>>
  : SerializableTypeString<typename viskores::cont::ArrayHandleSwizzle<AH, NComps>::Superclass>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH, viskores::IdComponent NComps>
struct Serialization<viskores::cont::ArrayHandleSwizzle<AH, NComps>>
  : Serialization<typename viskores::cont::ArrayHandleSwizzle<AH, NComps>::Superclass>
{
};

} // diy
/// @endcond SERIALIZATION

#endif // viskores_cont_ArrayHandleSwizzle_h
