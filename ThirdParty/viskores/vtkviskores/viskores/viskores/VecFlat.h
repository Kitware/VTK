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
#ifndef viskores_VecFlat_h
#define viskores_VecFlat_h

#include <viskores/StaticAssert.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

namespace internal
{

template <typename T,
          typename MultipleComponents = typename viskores::VecTraits<T>::HasMultipleComponents>
struct TotalNumComponents;

template <typename T>
struct TotalNumComponents<T, viskores::VecTraitsTagMultipleComponents>
{
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<typename viskores::VecTraits<T>::IsSizeStatic,
                  viskores::VecTraitsTagSizeStatic>::value),
    "viskores::VecFlat can only be used with Vec types with a static number of components.");
  using ComponentType = typename viskores::VecTraits<T>::ComponentType;
  static constexpr viskores::IdComponent value =
    viskores::VecTraits<T>::NUM_COMPONENTS * TotalNumComponents<ComponentType>::value;
};

template <typename T>
struct TotalNumComponents<T, viskores::VecTraitsTagSingleComponent>
{
  static constexpr viskores::IdComponent value = 1;
};

template <typename T>
using FlattenVec = viskores::Vec<typename viskores::VecTraits<T>::BaseComponentType,
                                 viskores::internal::TotalNumComponents<T>::value>;

template <typename T>
using IsFlatVec = typename std::is_same<T, FlattenVec<T>>::type;

namespace detail
{

template <typename T>
VISKORES_EXEC_CONT T GetFlatVecComponentImpl(const T& component,
                                             viskores::IdComponent index,
                                             std::true_type viskoresNotUsed(isBase))
{
  VISKORES_ASSERT(index == 0);
  return component;
}

template <typename T>
VISKORES_EXEC_CONT typename viskores::VecTraits<T>::BaseComponentType GetFlatVecComponentImpl(
  const T& vec,
  viskores::IdComponent index,
  std::false_type viskoresNotUsed(isBase))
{
  using Traits = viskores::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using BaseComponentType = typename Traits::BaseComponentType;

  constexpr viskores::IdComponent subSize = TotalNumComponents<ComponentType>::value;
  return GetFlatVecComponentImpl(Traits::GetComponent(vec, index / subSize),
                                 index % subSize,
                                 typename std::is_same<ComponentType, BaseComponentType>::type{});
}

} // namespace detail

template <typename T>
VISKORES_EXEC_CONT typename viskores::VecTraits<T>::BaseComponentType GetFlatVecComponent(
  const T& vec,
  viskores::IdComponent index)
{
  return detail::GetFlatVecComponentImpl(vec, index, std::false_type{});
}

namespace detail
{

template <typename T, viskores::IdComponent N>
VISKORES_EXEC_CONT void CopyVecNestedToFlatImpl(T nestedVec,
                                                viskores::Vec<T, N>& flatVec,
                                                viskores::IdComponent flatOffset)
{
  flatVec[flatOffset] = nestedVec;
}

template <typename T, viskores::IdComponent NFlat, viskores::IdComponent NNest>
VISKORES_EXEC_CONT void CopyVecNestedToFlatImpl(const viskores::Vec<T, NNest>& nestedVec,
                                                viskores::Vec<T, NFlat>& flatVec,
                                                viskores::IdComponent flatOffset)
{
  for (viskores::IdComponent nestedIndex = 0; nestedIndex < NNest; ++nestedIndex)
  {
    flatVec[nestedIndex + flatOffset] = nestedVec[nestedIndex];
  }
}

template <typename T, viskores::IdComponent N, typename NestedVecType>
VISKORES_EXEC_CONT void CopyVecNestedToFlatImpl(const NestedVecType& nestedVec,
                                                viskores::Vec<T, N>& flatVec,
                                                viskores::IdComponent flatOffset)
{
  using Traits = viskores::VecTraits<NestedVecType>;
  using ComponentType = typename Traits::ComponentType;
  constexpr viskores::IdComponent subSize = TotalNumComponents<ComponentType>::value;

  viskores::IdComponent flatIndex = flatOffset;
  for (viskores::IdComponent nestIndex = 0; nestIndex < Traits::NUM_COMPONENTS; ++nestIndex)
  {
    CopyVecNestedToFlatImpl(Traits::GetComponent(nestedVec, nestIndex), flatVec, flatIndex);
    flatIndex += subSize;
  }
}

} // namespace detail

template <typename T, viskores::IdComponent N, typename NestedVecType>
VISKORES_EXEC_CONT void CopyVecNestedToFlat(const NestedVecType& nestedVec,
                                            viskores::Vec<T, N>& flatVec)
{
  detail::CopyVecNestedToFlatImpl(nestedVec, flatVec, 0);
}

namespace detail
{

template <typename T, viskores::IdComponent N>
VISKORES_EXEC_CONT void CopyVecFlatToNestedImpl(const viskores::Vec<T, N>& flatVec,
                                                viskores::IdComponent flatOffset,
                                                T& nestedVec)
{
  nestedVec = flatVec[flatOffset];
}

template <typename T, viskores::IdComponent NFlat, viskores::IdComponent NNest>
VISKORES_EXEC_CONT void CopyVecFlatToNestedImpl(const viskores::Vec<T, NFlat>& flatVec,
                                                viskores::IdComponent flatOffset,
                                                viskores::Vec<T, NNest>& nestedVec)
{
  for (viskores::IdComponent nestedIndex = 0; nestedIndex < NNest; ++nestedIndex)
  {
    nestedVec[nestedIndex] = flatVec[nestedIndex + flatOffset];
  }
}

template <typename T,
          viskores::IdComponent NFlat,
          typename ComponentType,
          viskores::IdComponent NNest>
VISKORES_EXEC_CONT void CopyVecFlatToNestedImpl(const viskores::Vec<T, NFlat>& flatVec,
                                                viskores::IdComponent flatOffset,
                                                viskores::Vec<ComponentType, NNest>& nestedVec)
{
  constexpr viskores::IdComponent subSize = TotalNumComponents<ComponentType>::value;

  viskores::IdComponent flatIndex = flatOffset;
  for (viskores::IdComponent nestIndex = 0; nestIndex < NNest; ++nestIndex)
  {
    CopyVecFlatToNestedImpl(flatVec, flatIndex, nestedVec[nestIndex]);
    flatIndex += subSize;
  }
}

template <typename T, viskores::IdComponent N, typename NestedVecType>
VISKORES_EXEC_CONT void CopyVecFlatToNestedImpl(const viskores::Vec<T, N>& flatVec,
                                                viskores::IdComponent flatOffset,
                                                NestedVecType& nestedVec)
{
  using Traits = viskores::VecTraits<NestedVecType>;
  using ComponentType = typename Traits::ComponentType;
  constexpr viskores::IdComponent subSize = TotalNumComponents<ComponentType>::value;

  viskores::IdComponent flatIndex = flatOffset;
  for (viskores::IdComponent nestIndex = 0; nestIndex < Traits::NUM_COMPONENTS; ++nestIndex)
  {
    ComponentType component;
    CopyVecFlatToNestedImpl(flatVec, flatIndex, component);
    Traits::SetComponent(nestedVec, nestIndex, component);
    flatIndex += subSize;
  }
}

} // namespace detail

template <typename T, viskores::IdComponent N, typename NestedVecType>
VISKORES_EXEC_CONT void CopyVecFlatToNested(const viskores::Vec<T, N>& flatVec,
                                            NestedVecType& nestedVec)
{
  detail::CopyVecFlatToNestedImpl(flatVec, 0, nestedVec);
}

} // namespace internal

/// \brief Treat a `Vec` or `Vec`-like object as a flat `Vec`.
///
/// The `VecFlat` template wraps around another object that is a nested `Vec` object
/// (that is, a vector of vectors) and treats it like a flat, 1 dimensional `Vec`.
/// For example, let's say that you have a `Vec` of size 3 holding `Vec`s of size 2.
///
/// ```cpp
/// void Foo(const viskores::Vec<viskores::Vec<viskores::Id, 2>, 3>& nestedVec)
/// {
///   auto flatVec = viskores::make_VecFlat(nestedVec);
/// ```
///
/// `flatVec` is now of type `viskores::VecFlat<viskores::Vec<viskores::Vec<T, 2>, 3>`.
/// `flatVec::NUM_COMPONENTS` is 6 (3 * 2). The `[]` operator takes an index between
/// 0 and 5 and returns a value of type `viskores::Id`. The indices are explored in
/// depth-first order. So `flatVec[0] == nestedVec[0][0]`, `flatVec[1] == nestedVec[0][1]`,
/// `flatVec[2] == nestedVec[1][0]`, and so on.
///
/// Note that `flatVec` only works with types that have `VecTraits` defined where
/// the `IsSizeStatic` field is `viskores::VecTraitsTagSizeStatic` (that is, the `NUM_COMPONENTS`
/// constant is defined).
///
template <typename T, bool = internal::IsFlatVec<T>::value>
class VecFlat;

// Case where T is not a viskores::Vec<T, N> where T is not a Vec.
template <typename T>
class VecFlat<T, false> : public internal::FlattenVec<T>
{
  using Superclass = internal::FlattenVec<T>;

public:
  using Superclass::Superclass;
  VecFlat() = default;

  VISKORES_EXEC_CONT VecFlat(const T& src) { *this = src; }

  VISKORES_EXEC_CONT VecFlat& operator=(const T& src)
  {
    internal::CopyVecNestedToFlat(src, *this);
    return *this;
  }

  VISKORES_EXEC_CONT operator T() const
  {
    T nestedVec;
    internal::CopyVecFlatToNested(*this, nestedVec);
    return nestedVec;
  }
};

// Specialization of VecFlat where the Vec is already flat Vec
template <typename T>
class VecFlat<T, true> : public T
{
public:
  using T::T;
  VecFlat() = default;

  VISKORES_EXEC_CONT VecFlat(const T& src)
    : T(src)
  {
  }

  VISKORES_EXEC_CONT VecFlat& operator=(const T& src)
  {
    this->T::operator=(src);
    return *this;
  }

  VISKORES_EXEC_CONT VecFlat& operator=(T&& src)
  {
    this->T::operator=(std::move(src));
    return *this;
  }
};

/// \brief Converts a `Vec`-like object to a `VecFlat`.
///
template <typename T>
VISKORES_EXEC_CONT viskores::VecFlat<T> make_VecFlat(const T& vec)
{
  return viskores::VecFlat<T>(vec);
}

template <typename T>
struct TypeTraits<viskores::VecFlat<T>> : TypeTraits<internal::FlattenVec<T>>
{
};

template <typename T>
struct VecTraits<viskores::VecFlat<T>> : VecTraits<internal::FlattenVec<T>>
{
};

} // namespace viskores

#endif //viskores_VecFlat_h
