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
#ifndef viskores_ArrayHandleDecorator_h
#define viskores_ArrayHandleDecorator_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/Storage.h>

#include <viskores/List.h>
#include <viskores/StaticAssert.h>
#include <viskores/Tuple.h>
#include <viskores/VecTraits.h>

#include <viskores/internal/ArrayPortalHelpers.h>

#include <viskoresstd/integer_sequence.h>

#include <array>
#include <numeric>
#include <type_traits>
#include <utility>

namespace viskores
{
namespace internal
{

namespace decor
{

// Generic InverseFunctor implementation that does nothing.
struct NoOpInverseFunctor
{
  NoOpInverseFunctor() = default;
  template <typename... Ts>
  VISKORES_EXEC_CONT NoOpInverseFunctor(Ts...)
  {
  }
  template <typename VT>
  VISKORES_EXEC_CONT void operator()(viskores::Id, VT) const
  {
  }
};

} // namespace decor

// The portal for ArrayHandleDecorator. Get calls FunctorType::operator(), and
// Set calls InverseFunctorType::operator(), but only if the DecoratorImpl
// provides an inverse.
template <typename ValueType_, typename FunctorType_, typename InverseFunctorType_>
class VISKORES_ALWAYS_EXPORT ArrayPortalDecorator
{
public:
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;
  using InverseFunctorType = InverseFunctorType_;
  using ReadOnly = std::is_same<InverseFunctorType, decor::NoOpInverseFunctor>;

  VISKORES_EXEC_CONT
  ArrayPortalDecorator() {}

  VISKORES_CONT
  ArrayPortalDecorator(FunctorType func, InverseFunctorType iFunc, viskores::Id numValues)
    : Functor(func)
    , InverseFunctor(iFunc)
    , NumberOfValues(numValues)
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return this->Functor(index); }

  template <typename ReadOnly_ = ReadOnly,
            typename = typename std::enable_if<!ReadOnly_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    this->InverseFunctor(index, value);
  }

private:
  FunctorType Functor;
  InverseFunctorType InverseFunctor;
  viskores::Id NumberOfValues;
};
}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

namespace internal
{

namespace decor
{

// Ensures that all types in variadic container ArrayHandleList are subclasses
// of ArrayHandleBase.
template <typename ArrayHandleList>
using AllAreArrayHandles =
  viskores::ListAll<ArrayHandleList, viskores::cont::internal::ArrayHandleCheck>;

namespace detail
{

// Tests whether DecoratorImplT has a CreateInverseFunctor(Portals...) method.
template <typename DecoratorImplT, typename PortalList>
struct IsFunctorInvertibleImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... PortalTs>
struct IsFunctorInvertibleImpl<DecoratorImplT, List<PortalTs...>>
{
private:
  template <
    typename T,
    typename U = decltype(std::declval<T>().CreateInverseFunctor(std::declval<PortalTs&&>()...))>
  static std::true_type InverseExistsTest(int);

  template <typename T>
  static std::false_type InverseExistsTest(...);

public:
  using type = decltype(InverseExistsTest<DecoratorImplT>(0));
};

// Tests whether DecoratorImplT has an AllocateSourceArrays(size, Arrays...) method.
template <typename DecoratorImplT, typename ArrayList>
struct IsDecoratorAllocatableImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... ArrayTs>
struct IsDecoratorAllocatableImpl<DecoratorImplT, List<ArrayTs...>>
{
private:
  template <typename T,
            typename U = decltype(std::declval<T>().AllocateSourceArrays(
              0,
              viskores::CopyFlag::Off,
              std::declval<viskores::cont::Token&>(),
              std::declval<ArrayTs&>()...))>
  static std::true_type Exists(int);
  template <typename T>
  static std::false_type Exists(...);

public:
  using type = decltype(Exists<DecoratorImplT>(0));
};

// Deduces the type returned by DecoratorImplT::CreateFunctor when given
// the specified portals.
template <typename DecoratorImplT, typename PortalList>
struct GetFunctorTypeImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... PortalTs>
struct GetFunctorTypeImpl<DecoratorImplT, List<PortalTs...>>
{
  using type =
    decltype(std::declval<DecoratorImplT>().CreateFunctor(std::declval<PortalTs&&>()...));
};

// Deduces the type returned by DecoratorImplT::CreateInverseFunctor when given
// the specified portals. If DecoratorImplT doesn't have a CreateInverseFunctor
// method, a NoOp functor will be used instead.
template <typename CanWrite, typename DecoratorImplT, typename PortalList>
struct GetInverseFunctorTypeImpl;

template <typename DecoratorImplT, template <typename...> class List, typename... PortalTs>
struct GetInverseFunctorTypeImpl<std::true_type, DecoratorImplT, List<PortalTs...>>
{
  using type =
    decltype(std::declval<DecoratorImplT>().CreateInverseFunctor(std::declval<PortalTs&&>()...));
};

template <typename DecoratorImplT, typename PortalList>
struct GetInverseFunctorTypeImpl<std::false_type, DecoratorImplT, PortalList>
{
  using type = viskores::internal::decor::NoOpInverseFunctor;
};

// Get appropriate portals from a source array.
// See note below about using non-writable portals in invertible functors.
// We need to sub in const portals when writable ones don't exist.
template <typename ArrayT>
typename std::decay<ArrayT>::type::WritePortalType GetWritePortalImpl(
  std::true_type,
  ArrayT&& array,
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  return ArrayT::StorageType::CreateWritePortal(array.GetBuffers(), device, token);
}

template <typename ArrayT>
typename std::decay<ArrayT>::type::ReadPortalType GetWritePortalImpl(
  std::false_type,
  ArrayT&& array,
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  return ArrayT::StorageType::CreateReadPortal(array.GetBuffers(), device, token);
}

} // namespace detail

// Get portal types:
// We allow writing to an AHDecorator if *any* of the ArrayHandles are writable.
// This means we have to avoid calling PrepareForOutput, etc on non-writable
// array handles, since these may throw. On non-writable handles, use the
// const array handles so we can at least read from them in the inverse
// functors.
template <typename ArrayT>
using GetWritePortalType = std::conditional_t<
  viskores::internal::PortalSupportsSets<typename std::decay<ArrayT>::type::WritePortalType>::value,
  typename std::decay<ArrayT>::type::WritePortalType,
  typename std::decay<ArrayT>::type::ReadPortalType>;

template <typename ArrayT>
using GetReadPortalType = typename std::decay<ArrayT>::type::ReadPortalType;

// Get portal objects:
// See note above -- we swap in const portals sometimes.
template <typename ArrayT>
GetWritePortalType<typename std::decay<ArrayT>::type>
WritePortal(ArrayT&& array, viskores::cont::DeviceAdapterId device, viskores::cont::Token& token)
{
  return detail::GetWritePortalImpl(
    IsWritableArrayHandle<ArrayT>{}, std::forward<ArrayT>(array), device, token);
}

template <typename ArrayT>
GetReadPortalType<typename std::decay<ArrayT>::type> ReadPortal(
  const ArrayT& array,
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  return ArrayT::StorageType::CreateReadPortal(array.GetBuffers(), device, token);
}

// Equivalent to std::true_type if *any* portal in PortalList can be written to.
// If all are read-only, std::false_type is used instead.
template <typename PortalList>
using AnyPortalIsWritable = viskores::ListAny<PortalList, viskores::internal::PortalSupportsSets>;

// Set to std::true_type if DecoratorImplT::CreateInverseFunctor can be called
// with the supplied portals, or std::false_type otherwise.
template <typename DecoratorImplT, typename PortalList>
using IsFunctorInvertible =
  typename detail::IsFunctorInvertibleImpl<DecoratorImplT, PortalList>::type;

// Set to std::true_type if DecoratorImplT::AllocateSourceArrays can be called
// with the supplied arrays, or std::false_type otherwise.
template <typename DecoratorImplT, typename ArrayList>
using IsDecoratorAllocatable =
  typename detail::IsDecoratorAllocatableImpl<DecoratorImplT, ArrayList>::type;

// std::true_type/std::false_type depending on whether the decorator impl has a
// CreateInversePortal method AND any of the arrays are writable.
template <typename DecoratorImplT, typename PortalList>
using CanWriteToFunctor =
  viskores::internal::meta::And<IsFunctorInvertible<DecoratorImplT, PortalList>,
                                AnyPortalIsWritable<PortalList>>;

// The FunctorType for the provided implementation and portal types.
template <typename DecoratorImplT, typename PortalList>
using GetFunctorType = typename detail::GetFunctorTypeImpl<DecoratorImplT, PortalList>::type;

// The InverseFunctorType for the provided implementation and portal types.
// Will detect when inversion is not possible and return a NoOp functor instead.
template <typename DecoratorImplT, typename PortalList>
using GetInverseFunctorType =
  typename detail::GetInverseFunctorTypeImpl<CanWriteToFunctor<DecoratorImplT, PortalList>,
                                             DecoratorImplT,
                                             PortalList>::type;

// Convert a sequence of array handle types to a list of portals:

// Some notes on this implementation:
// - A more straightforward way to implement these to types would be to take
//   a simple template that takes a viskores::List of array types and convert
//   that with viskores::ListTransform<ArrayList, GetWritePortalType>. However,
//   this causes a strange compiler error in VS 2017 when evaluating the
//   `ValueType` in `DecoratorStorageTraits`. (It does not recognize the
//   decorator impl functor as a function taking one argument, even when it
//   effectively is.) A previous similar implementation caused an ICE in
//   VS 2015 (although that compiler is no longer supported anyway).
// - The same problem happens with VS 2017 if you just try to create a list
//   with viskores::List<GetWritePortalType<ArrayTs>...>.
// - So we jump through some decltype/declval hoops here to get this to work:
template <typename... ArrayTs>
using GetReadPortalList =
  viskores::List<decltype((ReadPortal(std::declval<ArrayTs&>(),
                                      std::declval<viskores::cont::DeviceAdapterId>(),
                                      std::declval<viskores::cont::Token&>())))...>;

template <typename... ArrayTs>
using GetWritePortalList =
  viskores::List<decltype((WritePortal(std::declval<ArrayTs&>(),
                                       std::declval<viskores::cont::DeviceAdapterId>(),
                                       std::declval<viskores::cont::Token&>())))...>;

template <typename DecoratorImplT, std::size_t NumArrays>
struct DecoratorMetaData
{
  DecoratorImplT Implementation;
  viskores::Id NumberOfValues = 0;
  std::array<std::size_t, NumArrays + 1> BufferOffsets;

  template <typename... ArrayTs>
  DecoratorMetaData(const DecoratorImplT& implementation,
                    viskores::Id numValues,
                    const ArrayTs... arrays)
    : Implementation(implementation)
    , NumberOfValues(numValues)
  {
    auto numBuffers = { std::size_t{ 1 }, arrays.GetBuffers().size()... };
    std::partial_sum(numBuffers.begin(), numBuffers.end(), this->BufferOffsets.begin());
  }

  DecoratorMetaData() = default;
};

template <typename DecoratorImplT, typename... ArrayTs>
struct DecoratorStorageTraits
{
  using ArrayList = viskores::List<ArrayTs...>;

  VISKORES_STATIC_ASSERT_MSG(sizeof...(ArrayTs) > 0,
                             "Must specify at least one source array handle for "
                             "ArrayHandleDecorator. Consider using "
                             "ArrayHandleImplicit instead.");

  // Need to check this here, since this traits struct is used in the
  // ArrayHandleDecorator superclass definition before any other
  // static_asserts could be used.
  VISKORES_STATIC_ASSERT_MSG(decor::AllAreArrayHandles<ArrayList>::value,
                             "Trailing template parameters for "
                             "ArrayHandleDecorator must be a list of ArrayHandle "
                             "types.");

  using ArrayTupleType = viskores::Tuple<ArrayTs...>;

  // size_t integral constants that index ArrayTs:
  using IndexList = viskoresstd::make_index_sequence<sizeof...(ArrayTs)>;

  using MetaData = DecoratorMetaData<DecoratorImplT, sizeof...(ArrayTs)>;

  static MetaData& GetMetaData(const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<MetaData>();
  }

  // Converts a buffers array to the ArrayHandle at the given index.
  template <viskores::IdComponent I>
  static viskores::TupleElement<I, ArrayTupleType> BuffersToArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    const MetaData& metaData = GetMetaData(buffers);
    std::vector<viskores::cont::internal::Buffer> subBuffers(
      buffers.begin() + metaData.BufferOffsets[I], buffers.begin() + metaData.BufferOffsets[I + 1]);
    return viskores::TupleElement<I, ArrayTupleType>(std::move(subBuffers));
  }

  // true_type/false_type depending on whether the decorator supports Allocate:
  using IsAllocatable = IsDecoratorAllocatable<DecoratorImplT, ArrayList>;

  // Portal lists:
  // NOTE we have to pass the parameter pack here instead of using ArrayList
  // with viskores::ListTransform, since that's causing problems with VS 2017:
  using WritePortalList = GetWritePortalList<ArrayTs...>;
  using ReadPortalList = GetReadPortalList<ArrayTs...>;

  // Functors:
  using WriteFunctorType = GetFunctorType<DecoratorImplT, WritePortalList>;
  using ReadFunctorType = GetFunctorType<DecoratorImplT, ReadPortalList>;

  // Inverse functors:
  using InverseWriteFunctorType = GetInverseFunctorType<DecoratorImplT, WritePortalList>;

  using InverseReadFunctorType = viskores::internal::decor::NoOpInverseFunctor;

  // Misc:
  // ValueType is derived from DecoratorImplT::CreateFunctor(...)'s operator().
  using ValueType = decltype(std::declval<WriteFunctorType>()(0));

  // Decorator portals:
  using WritePortalType =
    viskores::internal::ArrayPortalDecorator<ValueType, WriteFunctorType, InverseWriteFunctorType>;

  using ReadPortalType =
    viskores::internal::ArrayPortalDecorator<ValueType, ReadFunctorType, InverseReadFunctorType>;

  // helper for constructing portals with the appropriate functors. This is
  // where we decide whether or not to call `CreateInverseFunctor` on the
  // implementation class.
  // Do not use these directly, they are helpers for the MakePortal[...]
  // methods below.
  template <typename DecoratorPortalType, typename... PortalTs>
  VISKORES_CONT static
    typename std::enable_if<DecoratorPortalType::ReadOnly::value, DecoratorPortalType>::type
    CreatePortalDecorator(viskores::Id numVals, const DecoratorImplT& impl, PortalTs&&... portals)
  { // Portal is read only:
    return { impl.CreateFunctor(std::forward<PortalTs>(portals)...),
             typename DecoratorPortalType::InverseFunctorType{},
             numVals };
  }

  template <typename DecoratorPortalType, typename... PortalTs>
  VISKORES_CONT static
    typename std::enable_if<!DecoratorPortalType::ReadOnly::value, DecoratorPortalType>::type
    CreatePortalDecorator(viskores::Id numVals, const DecoratorImplT& impl, PortalTs... portals)
  { // Portal is read/write:
    return { impl.CreateFunctor(portals...), impl.CreateInverseFunctor(portals...), numVals };
  }

  // Static dispatch for calling AllocateSourceArrays on supported implementations:
  VISKORES_CONT [[noreturn]] static void CallAllocate(
    std::false_type,
    viskores::Id,
    const std::vector<viskores::cont::internal::Buffer>&,
    viskores::CopyFlag,
    viskores::cont::Token&,
    ArrayTs...)
  {
    throw viskores::cont::ErrorBadType("Allocate not supported by this ArrayHandleDecorator.");
  }

  VISKORES_CONT static void CallAllocate(
    std::true_type,
    viskores::Id newSize,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token,
    ArrayTs... arrays)
  {
    MetaData& metadata = GetMetaData(buffers);
    metadata.Implementation.AllocateSourceArrays(newSize, preserve, token, arrays...);
    metadata.NumberOfValues = newSize;
  }


  // Portal construction methods. These actually create portals.
  template <std::size_t... Indices>
  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::Id numValues,
    viskoresstd::index_sequence<Indices...>,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return CreatePortalDecorator<WritePortalType>(
      numValues,
      GetMetaData(buffers).Implementation,
      WritePortal(BuffersToArray<Indices>(buffers), device, token)...);
  }

  template <std::size_t... Indices>
  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::Id numValues,
    viskoresstd::index_sequence<Indices...>,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return CreatePortalDecorator<ReadPortalType>(
      numValues,
      GetMetaData(buffers).Implementation,
      ReadPortal(BuffersToArray<Indices>(buffers), device, token)...);
  }

  template <std::size_t... Indices>
  VISKORES_CONT static void AllocateSourceArrays(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token,
    viskoresstd::index_sequence<Indices...>)
  {
    CallAllocate(
      IsAllocatable{}, numValues, buffers, preserve, token, BuffersToArray<Indices>(buffers)...);
  }
};

} // end namespace decor

template <typename DecoratorImplT, typename... ArrayTs>
struct VISKORES_ALWAYS_EXPORT StorageTagDecorator
{
};

template <typename DecoratorImplT, typename... ArrayTs>
class Storage<typename decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>::ValueType,
              StorageTagDecorator<DecoratorImplT, ArrayTs...>>
{
  using Traits = decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>;
  using IndexList = typename Traits::IndexList;
  using MetaData = typename Traits::MetaData;

public:
  using ReadPortalType = typename Traits::ReadPortalType;
  using WritePortalType = typename Traits::WritePortalType;

private:
  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlatImpl(
    viskores::VecTraitsTagSizeStatic)
  {
    return viskores::VecFlat<DecoratorImplT>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlatImpl(
    viskores::VecTraitsTagSizeVariable)
  {
    // Currently only support getting the number of components for statically sized types.
    return 0;
  }

public:
  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return GetNumberOfComponentsFlatImpl(
      typename viskores::VecTraits<DecoratorImplT>::IsSizeStatic{});
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Traits::GetMetaData(buffers).NumberOfValues;
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    if (numValues != GetNumberOfValues(buffers))
    {
      Traits::AllocateSourceArrays(numValues, buffers, preserve, token, IndexList{});
    }
    else
    {
      // Do nothing. We have this condition to allow allocating the same size when the
      // array cannot be resized.
    }
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return Traits::CreateReadPortal(
      buffers, GetNumberOfValues(buffers), IndexList{}, device, token);
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return Traits::CreateWritePortal(
      buffers, GetNumberOfValues(buffers), IndexList{}, device, token);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const DecoratorImplT& implementation,
    viskores::Id numValues,
    const ArrayTs&... arrays)
  {
    return viskores::cont::internal::CreateBuffers(MetaData(implementation, numValues, arrays...),
                                                   arrays...);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return CreateBuffers(DecoratorImplT{}, 0, ArrayTs{}...);
  }
};

template <typename DecoratorImplT, typename... ArrayTs>
struct DecoratorHandleTraits
{
  using StorageTraits = decor::DecoratorStorageTraits<DecoratorImplT, ArrayTs...>;
  using ValueType = typename StorageTraits::ValueType;
  using StorageTag = StorageTagDecorator<DecoratorImplT, ArrayTs...>;
  using Superclass = viskores::cont::ArrayHandle<ValueType, StorageTag>;
};

} // namespace internal

/// \brief A fancy ArrayHandle that can be used to modify the results from one
/// or more source ArrayHandle.
///
/// ArrayHandleDecorator is given a `DecoratorImplT` class and a list of one or
/// more source ArrayHandles. There are no restrictions on the size or type of
/// the source ArrayHandles.
///
/// The decorator implementation class is described below:
///
/// ```
/// struct ExampleDecoratorImplementation
/// {
///
///   // Takes one portal for each source array handle (only two shown).
///   // Returns a functor that defines:
///   //
///   // VISKORES_EXEC_CONT ValueType operator()(viskores::Id id) const;
///   //
///   // which takes an index and returns a value which should be produced by
///   // the source arrays somehow. This ValueType will be the ValueType of the
///   // ArrayHandleDecorator.
///   //
///   // Both SomeFunctor::operator() and CreateFunctor must be const.
///   //
///   template <typename Portal1Type, typename Portal2Type>
///   VISKORES_CONT
///   SomeFunctor CreateFunctor(Portal1Type portal1, Portal2Type portal2) const;
///
///   // Takes one portal for each source array handle (only two shown).
///   // Returns a functor that defines:
///   //
///   // VISKORES_EXEC_CONT void operator()(viskores::Id id, ValueType val) const;
///   //
///   // which takes an index and a value, which should be used to modify one
///   // or more of the source arrays.
///   //
///   // CreateInverseFunctor is optional; if not provided, the
///   // ArrayHandleDecorator will be read-only. In addition, if all of the
///   // source ArrayHandles are read-only, the inverse functor will not be used
///   // and the ArrayHandleDecorator will be read only.
///   //
///   // Both SomeInverseFunctor::operator() and CreateInverseFunctor must be
///   // const.
///   //
///   template <typename Portal1Type, typename Portal2Type>
///   VISKORES_CONT
///   SomeInverseFunctor CreateInverseFunctor(Portal1Type portal1,
///                                           Portal2Type portal2) const;
///
///   // Given a set of ArrayHandles and a size, implement what should happen
///   // to the source ArrayHandles when Allocate() is called on the decorator
///   // handle.
///   //
///   // AllocateSourceArrays is optional; if not provided, the
///   // ArrayHandleDecorator will throw if its Allocate method is called. If
///   // an implementation is present and doesn't throw, the
///   // ArrayHandleDecorator's internal state is updated to show `size` as the
///   // number of values.
///   template <typename Array1Type, typename Array2Type>
///   VISKORES_CONT
///   void AllocateSourceArrays(viskores::Id size,
///                             viskores::CopyFlag preserve,
///                             viskores::cont::Token& token,
///                             Array1Type array1,
///                             Array2Type array2) const;
///
/// };
/// ```
///
/// There are several example DecoratorImpl classes provided in the
/// UnitTestArrayHandleDecorator test file.
///
template <typename DecoratorImplT, typename... ArrayTs>
class ArrayHandleDecorator
  : public internal::DecoratorHandleTraits<typename std::decay<DecoratorImplT>::type,
                                           typename std::decay<ArrayTs>::type...>::Superclass
{
private:
  using Traits = internal::DecoratorHandleTraits<typename std::decay<DecoratorImplT>::type,
                                                 typename std::decay<ArrayTs>::type...>;

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleDecorator,
                                 (ArrayHandleDecorator<typename std::decay<DecoratorImplT>::type,
                                                       typename std::decay<ArrayTs>::type...>),
                                 (typename Traits::Superclass));

  VISKORES_CONT
  ArrayHandleDecorator(viskores::Id numValues,
                       const typename std::decay<DecoratorImplT>::type& impl,
                       const typename std::decay<ArrayTs>::type&... arrays)
    : Superclass{ StorageType::CreateBuffers(impl, numValues, arrays...) }
  {
  }
};

/// Create an ArrayHandleDecorator with the specified number of values that
/// uses the provided DecoratorImplT and source ArrayHandles.
///
template <typename DecoratorImplT, typename... ArrayTs>
VISKORES_CONT ArrayHandleDecorator<typename std::decay<DecoratorImplT>::type,
                                   typename std::decay<ArrayTs>::type...>
make_ArrayHandleDecorator(viskores::Id numValues, DecoratorImplT&& f, ArrayTs&&... arrays)
{
  using AHList = viskores::List<typename std::decay<ArrayTs>::type...>;
  VISKORES_STATIC_ASSERT_MSG(sizeof...(ArrayTs) > 0,
                             "Must specify at least one source array handle for "
                             "ArrayHandleDecorator. Consider using "
                             "ArrayHandleImplicit instead.");
  VISKORES_STATIC_ASSERT_MSG(internal::decor::AllAreArrayHandles<AHList>::value,
                             "Trailing template parameters for "
                             "ArrayHandleDecorator must be a list of ArrayHandle "
                             "types.");

  return { numValues, std::forward<DecoratorImplT>(f), std::forward<ArrayTs>(arrays)... };
}
}
} // namespace viskores::cont

#ifdef VISKORES_USE_TAO_SEQ
#undef VISKORES_USE_TAO_SEQ
#endif

#endif //viskores_ArrayHandleDecorator_h
