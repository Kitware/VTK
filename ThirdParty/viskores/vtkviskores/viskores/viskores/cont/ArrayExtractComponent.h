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
#ifndef viskores_cont_ArrayExtractComponent_h
#define viskores_cont_ArrayExtractComponent_h

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Logging.h>

#include <viskores/TypeTraits.h>
#include <viskores/VecFlat.h>
#include <viskores/VecTraits.h>

#include <viskoresstd/integer_sequence.h>

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

namespace internal
{

// Note: Using partial template specialization instead of function overloading to
// specialize ArrayExtractComponent for different types of array handles. This is
// because function overloading from a templated function is done when the template
// is defined rather than where it is resolved. This causes problems when extracting
// components of, say, an ArrayHandleMultiplexer holding an ArrayHandleSOA.
template <typename T, typename S>
viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>
ArrayExtractComponentFallback(const viskores::cont::ArrayHandle<T, S>& src,
                              viskores::IdComponent componentIndex,
                              viskores::CopyFlag allowCopy)
{
  if (allowCopy != viskores::CopyFlag::On)
  {
    throw viskores::cont::ErrorBadValue(
      "Cannot extract component of " +
      viskores::cont::TypeToString<viskores::cont::ArrayHandle<T, S>>() + " without copying");
  }
  VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                 "Extracting component "
                   << componentIndex << " of "
                   << viskores::cont::TypeToString<viskores::cont::ArrayHandle<T, S>>()
                   << " requires an inefficient memory copy.");

  using BaseComponentType = typename viskores::VecTraits<T>::BaseComponentType;
  viskores::Id numValues = src.GetNumberOfValues();
  viskores::cont::ArrayHandleBasic<BaseComponentType> dest;
  dest.Allocate(numValues);
  auto srcPortal = src.ReadPortal();
  auto destPortal = dest.WritePortal();
  for (viskores::Id arrayIndex = 0; arrayIndex < numValues; ++arrayIndex)
  {
    destPortal.Set(
      arrayIndex,
      viskores::internal::GetFlatVecComponent(srcPortal.Get(arrayIndex), componentIndex));
  }

  return viskores::cont::ArrayHandleStride<BaseComponentType>(dest, numValues, 1, 0);
}

// Used as a superclass for ArrayHandleComponentImpls that are inefficient (and should be
// avoided).
struct ArrayExtractComponentImplInefficient
{
};

template <typename S>
struct ArrayExtractComponentImpl : ArrayExtractComponentImplInefficient
{
  template <typename T>
  viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> operator()(
    const viskores::cont::ArrayHandle<T, S>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    // This is the slow "default" implementation. ArrayHandle implementations should provide
    // more efficient overloads where applicable.
    return viskores::cont::internal::ArrayExtractComponentFallback(src, componentIndex, allowCopy);
  }
};

template <>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagStride>
{
  template <typename T>
  viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagStride>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    return this->DoExtract(
      src, componentIndex, allowCopy, typename viskores::VecTraits<T>::HasMultipleComponents{});
  }

private:
  template <typename T>
  auto DoExtract(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagStride>& src,
                 viskores::IdComponent componentIndex,
                 viskores::CopyFlag viskoresNotUsed(allowCopy),
                 viskores::VecTraitsTagSingleComponent) const
  {
    VISKORES_ASSERT(componentIndex == 0);
    using VTraits = viskores::VecTraits<T>;
    using TBase = typename VTraits::BaseComponentType;
    VISKORES_STATIC_ASSERT(VTraits::NUM_COMPONENTS == 1);

    viskores::cont::ArrayHandleStride<T> array(src);

    // Note, we are initializing the result in this strange way for cases where type
    // T has a single component but does not equal its own BaseComponentType. A viskores::Vec
    // of size 1 fits into this category.
    return viskores::cont::ArrayHandleStride<TBase>(array.GetBuffers()[1],
                                                    array.GetNumberOfValues(),
                                                    array.GetStride(),
                                                    array.GetOffset(),
                                                    array.GetModulo(),
                                                    array.GetDivisor());
  }

  template <typename VecType>
  auto DoExtract(const viskores::cont::ArrayHandle<VecType, viskores::cont::StorageTagStride>& src,
                 viskores::IdComponent componentIndex,
                 viskores::CopyFlag allowCopy,
                 viskores::VecTraitsTagMultipleComponents) const
  {
    using VTraits = viskores::VecTraits<VecType>;
    using T = typename VTraits::ComponentType;
    constexpr viskores::IdComponent N = VTraits::NUM_COMPONENTS;

    constexpr viskores::IdComponent subStride = viskores::internal::TotalNumComponents<T>::value;
    viskores::cont::ArrayHandleStride<VecType> array(src);
    viskores::cont::ArrayHandleStride<T> tmpIn(array.GetBuffers()[1],
                                               array.GetNumberOfValues(),
                                               array.GetStride() * N,
                                               (array.GetOffset() * N) +
                                                 (componentIndex / subStride),
                                               array.GetModulo() * N,
                                               array.GetDivisor());
    return (*this)(tmpIn, componentIndex % subStride, allowCopy);
  }
};

template <>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagBasic>
{
  template <typename T>
  auto operator()(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& src,
                  viskores::IdComponent componentIndex,
                  viskores::CopyFlag allowCopy) const
    -> decltype(ArrayExtractComponentImpl<viskores::cont::StorageTagStride>{}(
      viskores::cont::ArrayHandleStride<T>{},
      componentIndex,
      allowCopy))
  {
    return ArrayExtractComponentImpl<viskores::cont::StorageTagStride>{}(
      viskores::cont::ArrayHandleStride<T>(src, src.GetNumberOfValues(), 1, 0),
      componentIndex,
      allowCopy);
  }
};

namespace detail
{

template <std::size_t, typename Super>
struct ForwardSuper : Super
{
};

template <typename sequence, typename... Supers>
struct SharedSupersImpl;

template <std::size_t... Indices, typename... Supers>
struct SharedSupersImpl<viskoresstd::index_sequence<Indices...>, Supers...>
  : ForwardSuper<Indices, Supers>...
{
};

} // namespace detail

// `ArrayExtractComponentImpl`s that modify the behavior from other storage types might
// want to inherit from the `ArrayExtractComponentImpl`s of these storage types. However,
// if the template specifies multiple storage types, two of the same might be specified,
// and it is illegal in C++ to directly inherit from the same type twice. This special
// superclass accepts a variable amout of superclasses. Inheriting from this will inherit
// from all these superclasses, and duplicates are allowed.
template <typename... Supers>
using DuplicatedSuperclasses =
  detail::SharedSupersImpl<viskoresstd::make_index_sequence<sizeof...(Supers)>, Supers...>;

template <typename... StorageTags>
using ArrayExtractComponentImplInherit =
  DuplicatedSuperclasses<viskores::cont::internal::ArrayExtractComponentImpl<StorageTags>...>;

/// \brief Resolves to true if ArrayHandleComponent of the array handle would be inefficient.
///
template <typename ArrayHandleType>
using ArrayExtractComponentIsInefficient = typename std::is_base_of<
  viskores::cont::internal::ArrayExtractComponentImplInefficient,
  viskores::cont::internal::ArrayExtractComponentImpl<typename ArrayHandleType::StorageTag>>::type;

} // namespace internal

/// \brief Pulls a component out of an `ArrayHandle`.
///
/// Given an `ArrayHandle` of any type, `ArrayExtractComponent` returns an
/// `ArrayHandleStride` of the base component type that contains the data for the
/// specified array component. This function can be used to apply an operation on
/// an `ArrayHandle` one component at a time. Because the array type is always
/// `ArrayHandleStride`, you can drastically cut down on the number of templates
/// to instantiate (at a possible cost to performance).
///
/// Note that `ArrayExtractComponent` will flatten out the indices of any vec value
/// type and return an `ArrayExtractComponent` of the base component type. For
/// example, if you call `ArrayExtractComponent` on an `ArrayHandle` with a value
/// type of `viskores::Vec<viskores::Vec<viskores::Float32, 2>, 3>`, you will get an
/// `ArrayExtractComponent<viskores::Float32>` returned. The `componentIndex` provided
/// will be applied to the nested vector in depth first order. So in the previous
/// example, a `componentIndex` of 0 gets the values at [0][0], `componentIndex`
/// of 1 gets [0][1], `componentIndex` of 2 gets [1][0], and so on.
///
/// Some `ArrayHandle`s allow this method to return an `ArrayHandleStride` that
/// shares the same memory as the the original `ArrayHandle`. This form will be
/// used if possible. In this case, if data are written into the `ArrayHandleStride`,
/// they are also written into the original `ArrayHandle`. However, other forms will
/// require copies into a new array. In this case, writes into `ArrayHandleStride`
/// will not affect the original `ArrayHandle`.
///
/// For some operations, such as writing into an output array, this behavior of
/// shared arrays is necessary. For this case, the optional argument `allowCopy`
/// can be set to `viskores::CopyFlag::Off` to prevent the copying behavior into the
/// return `ArrayHandleStride`. If this is the case, an `ErrorBadValue` is thrown.
/// If the arrays can be shared, they always will be regardless of the value of
/// `allowCopy`.
///
/// Many forms of `ArrayHandle` have optimized versions to pull out a component.
/// Some, however, do not. In these cases, a fallback array copy, done in serial,
/// will be performed. A warning will be logged to alert users of this likely
/// performance bottleneck.
///
/// As an implementation note, this function should not be overloaded directly.
/// Instead, `ArrayHandle` implementations should provide a specialization of
/// `viskores::cont::internal::ArrayExtractComponentImpl`.
///
template <typename T, typename S>
viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>
ArrayExtractComponent(const viskores::cont::ArrayHandle<T, S>& src,
                      viskores::IdComponent componentIndex,
                      viskores::CopyFlag allowCopy = viskores::CopyFlag::On)
{
  return internal::ArrayExtractComponentImpl<S>{}(src, componentIndex, allowCopy);
}

}
} // namespace viskores::cont

#endif //viskores_cont_ArrayExtractComponent_h
