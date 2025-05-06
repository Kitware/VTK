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
#ifndef viskores_cont_ArrayHandleMultiplexer_h
#define viskores_cont_ArrayHandleMultiplexer_h

#include <viskores/Assert.h>
#include <viskores/TypeTraits.h>

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <viskores/cont/Variant.h>
#include <viskores/exec/Variant.h>

namespace viskores
{

namespace internal
{

namespace detail
{

struct ArrayPortalMultiplexerGetNumberOfValuesFunctor
{
  template <typename PortalType>
  VISKORES_EXEC_CONT viskores::Id operator()(const PortalType& portal) const noexcept
  {
    return portal.GetNumberOfValues();
  }
};

struct ArrayPortalMultiplexerGetFunctor
{
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VISKORES_EXEC_CONT typename PortalType::ValueType operator()(const PortalType& portal,
                                                               viskores::Id index) const noexcept
  {
    return portal.Get(index);
  }
};

struct ArrayPortalMultiplexerSetFunctor
{
  template <typename PortalType>
  VISKORES_EXEC_CONT void operator()(const PortalType& portal,
                                     viskores::Id index,
                                     const typename PortalType::ValueType& value) const noexcept
  {
    this->DoSet(
      portal, index, value, typename viskores::internal::PortalSupportsSets<PortalType>::type{});
  }

private:
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VISKORES_EXEC_CONT void DoSet(const PortalType& portal,
                                viskores::Id index,
                                const typename PortalType::ValueType& value,
                                std::true_type) const noexcept
  {
    portal.Set(index, value);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VISKORES_EXEC_CONT void DoSet(const PortalType&,
                                viskores::Id,
                                const typename PortalType::ValueType&,
                                std::false_type) const noexcept
  {
    // This is an error but whatever.
    VISKORES_ASSERT(false && "Calling Set on a portal that does not support it.");
  }
};

} // namespace detail

template <typename... PortalTypes>
struct ArrayPortalMultiplexer
{
  using PortalVariantType = viskores::exec::Variant<PortalTypes...>;
  PortalVariantType PortalVariant;

  using ValueType = typename PortalVariantType::template TypeAt<0>::ValueType;

  ArrayPortalMultiplexer() = default;
  ~ArrayPortalMultiplexer() = default;
  ArrayPortalMultiplexer(ArrayPortalMultiplexer&&) = default;
  ArrayPortalMultiplexer(const ArrayPortalMultiplexer&) = default;
  ArrayPortalMultiplexer& operator=(ArrayPortalMultiplexer&&) = default;
  ArrayPortalMultiplexer& operator=(const ArrayPortalMultiplexer&) = default;

  template <typename Portal>
  VISKORES_EXEC_CONT ArrayPortalMultiplexer(const Portal& src) noexcept
    : PortalVariant(src)
  {
  }

  template <typename Portal>
  VISKORES_EXEC_CONT ArrayPortalMultiplexer& operator=(const Portal& src) noexcept
  {
    this->PortalVariant = src;
    return *this;
  }

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const noexcept
  {
    return this->PortalVariant.CastAndCall(
      detail::ArrayPortalMultiplexerGetNumberOfValuesFunctor{});
  }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const noexcept
  {
    return this->PortalVariant.CastAndCall(detail::ArrayPortalMultiplexerGetFunctor{}, index);
  }

  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const noexcept
  {
    this->PortalVariant.CastAndCall(detail::ArrayPortalMultiplexerSetFunctor{}, index, value);
  }
};

} // namespace internal

namespace cont
{

template <typename... StorageTags>
struct StorageTagMultiplexer
{
};

namespace internal
{

namespace detail
{

struct MultiplexerGetNumberOfComponentsFlatFunctor
{
  template <typename StorageType>
  VISKORES_CONT viskores::IdComponent operator()(
    StorageType,
    const std::vector<viskores::cont::internal::Buffer>& buffers) const
  {
    return StorageType::GetNumberOfComponentsFlat(buffers);
  }
};

struct MultiplexerGetNumberOfValuesFunctor
{
  template <typename StorageType>
  VISKORES_CONT viskores::Id operator()(
    StorageType,
    const std::vector<viskores::cont::internal::Buffer>& buffers) const
  {
    return StorageType::GetNumberOfValues(buffers);
  }
};

struct MultiplexerResizeBuffersFunctor
{
  template <typename StorageType>
  VISKORES_CONT void operator()(StorageType,
                                viskores::Id numValues,
                                const std::vector<viskores::cont::internal::Buffer>& buffers,
                                viskores::CopyFlag preserve,
                                viskores::cont::Token& token) const
  {
    StorageType::ResizeBuffers(numValues, buffers, preserve, token);
  }
};

struct MultiplexerFillFunctor
{
  template <typename ValueType, typename StorageType>
  VISKORES_CONT void operator()(StorageType,
                                const std::vector<viskores::cont::internal::Buffer>& buffers,
                                const ValueType& fillValue,
                                viskores::Id startIndex,
                                viskores::Id endIndex,
                                viskores::cont::Token& token) const
  {
    StorageType::Fill(buffers, fillValue, startIndex, endIndex, token);
  }
};

template <typename ReadPortalType>
struct MultiplexerCreateReadPortalFunctor
{
  template <typename StorageType>
  VISKORES_CONT ReadPortalType
  operator()(StorageType,
             const std::vector<viskores::cont::internal::Buffer>& buffers,
             viskores::cont::DeviceAdapterId device,
             viskores::cont::Token& token) const
  {
    return ReadPortalType(StorageType::CreateReadPortal(buffers, device, token));
  }
};

template <typename WritePortalType>
struct MultiplexerCreateWritePortalFunctor
{
  template <typename StorageType>
  VISKORES_CONT WritePortalType
  operator()(StorageType,
             const std::vector<viskores::cont::internal::Buffer>& buffers,
             viskores::cont::DeviceAdapterId device,
             viskores::cont::Token& token) const
  {
    return WritePortalType(StorageType::CreateWritePortal(buffers, device, token));
  }
};

template <typename T, typename... Ss>
struct MultiplexerArrayHandleVariantFunctor
{
  using VariantType = viskores::cont::Variant<viskores::cont::ArrayHandle<T, Ss>...>;

  template <typename StorageTag>
  VISKORES_CONT VariantType operator()(viskores::cont::internal::Storage<T, StorageTag>,
                                       const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return VariantType(viskores::cont::ArrayHandle<T, StorageTag>(buffers));
  }
};

} // namespace detail

template <typename ValueType, typename... StorageTags>
class Storage<ValueType, StorageTagMultiplexer<StorageTags...>>
{
  template <typename S>
  using StorageFor = viskores::cont::internal::Storage<ValueType, S>;

  using StorageVariant = viskores::cont::Variant<StorageFor<StorageTags>...>;

  VISKORES_CONT static StorageVariant Variant(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<StorageVariant>();
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> ArrayBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  using ReadPortalType =
    viskores::internal::ArrayPortalMultiplexer<typename StorageFor<StorageTags>::ReadPortalType...>;
  using WritePortalType = viskores::internal::ArrayPortalMultiplexer<
    typename StorageFor<StorageTags>::WritePortalType...>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).CastAndCall(detail::MultiplexerGetNumberOfComponentsFlatFunctor{},
                                        ArrayBuffers(buffers));
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).CastAndCall(detail::MultiplexerGetNumberOfValuesFunctor{},
                                        ArrayBuffers(buffers));
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    Variant(buffers).CastAndCall(
      detail::MultiplexerResizeBuffersFunctor{}, numValues, ArrayBuffers(buffers), preserve, token);
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const ValueType& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    Variant(buffers).CastAndCall(detail::MultiplexerFillFunctor{},
                                 ArrayBuffers(buffers),
                                 fillValue,
                                 startIndex,
                                 endIndex,
                                 token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return Variant(buffers).CastAndCall(
      detail::MultiplexerCreateReadPortalFunctor<ReadPortalType>{},
      ArrayBuffers(buffers),
      device,
      token);
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return Variant(buffers).CastAndCall(
      detail::MultiplexerCreateWritePortalFunctor<WritePortalType>{},
      ArrayBuffers(buffers),
      device,
      token);
  }

  VISKORES_CONT static bool IsValid(const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).IsValid();
  }

  template <typename ArrayType>
  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const ArrayType& array)
  {
    VISKORES_IS_ARRAY_HANDLE(ArrayType);
    return viskores::cont::internal::CreateBuffers(StorageVariant{ array.GetStorage() }, array);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return viskores::cont::internal::CreateBuffers(StorageVariant{});
  }

  VISKORES_CONT static
    typename detail::MultiplexerArrayHandleVariantFunctor<ValueType, StorageTags...>::VariantType
    GetArrayHandleVariant(const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return Variant(buffers).CastAndCall(
      detail::MultiplexerArrayHandleVariantFunctor<ValueType, StorageTags...>{},
      ArrayBuffers(buffers));
  }
};

} // namespace internal

namespace detail
{

template <typename... ArrayHandleTypes>
struct ArrayHandleMultiplexerTraits
{
  using ArrayHandleType0 = viskores::ListAt<viskores::List<ArrayHandleTypes...>, 0>;
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType0);
  using ValueType = typename ArrayHandleType0::ValueType;

  // If there is a compile error in this group of lines, then one of the array types given to
  // ArrayHandleMultiplexer must contain an invalid ArrayHandle. That could mean that it is not an
  // ArrayHandle type or it could mean that the value type does not match the appropriate value
  // type.
  template <typename ArrayHandle>
  struct CheckArrayHandleTransform
  {
    VISKORES_IS_ARRAY_HANDLE(ArrayHandle);
    VISKORES_STATIC_ASSERT((std::is_same<ValueType, typename ArrayHandle::ValueType>::value));
  };
  using CheckArrayHandle = viskores::List<CheckArrayHandleTransform<ArrayHandleTypes>...>;

  // Note that this group of code could be simplified as the pair of lines:
  //   template <typename ArrayHandle>
  //   using ArrayHandleToStorageTag = typename ArrayHandle::StorageTag;
  // However, there are issues with older Visual Studio compilers that is not working
  // correctly with that form.
  template <typename ArrayHandle>
  struct ArrayHandleToStorageTagImpl
  {
    using Type = typename ArrayHandle::StorageTag;
  };
  template <typename ArrayHandle>
  using ArrayHandleToStorageTag = typename ArrayHandleToStorageTagImpl<ArrayHandle>::Type;

  using StorageTag =
    viskores::cont::StorageTagMultiplexer<ArrayHandleToStorageTag<ArrayHandleTypes>...>;
  using StorageType = viskores::cont::internal::Storage<ValueType, StorageTag>;
};
} // namespace detail

/// \brief An ArrayHandle that can behave like several other handles.
///
/// An \c ArrayHandleMultiplexer simply redirects its calls to another \c ArrayHandle. However
/// the type of that \c ArrayHandle does not need to be (completely) known at runtime. Rather,
/// \c ArrayHandleMultiplexer is defined over a set of possible \c ArrayHandle types. Any
/// one of these \c ArrayHandles may be assigned to the \c ArrayHandleMultiplexer.
///
/// When a value is retreived from the \c ArrayHandleMultiplexer, the multiplexer checks to
/// see which type of array is currently stored in it. It then redirects to the \c ArrayHandle
/// of the appropriate type.
///
/// The \c ArrayHandleMultiplexer template parameters are all the ArrayHandle types it
/// should support.
///
/// If only one template parameter is given, it is assumed to be the \c ValueType of the
/// array. A default list of supported arrays is supported (see
/// \c viskores::cont::internal::ArrayHandleMultiplexerDefaultArrays.) If multiple template
/// parameters are given, they are all considered possible \c ArrayHandle types.
///
template <typename... ArrayHandleTypes>
class ArrayHandleMultiplexer
  : public viskores::cont::ArrayHandle<
      typename detail::ArrayHandleMultiplexerTraits<ArrayHandleTypes...>::ValueType,
      typename detail::ArrayHandleMultiplexerTraits<ArrayHandleTypes...>::StorageTag>
{
  using Traits = detail::ArrayHandleMultiplexerTraits<ArrayHandleTypes...>;

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleMultiplexer,
    (ArrayHandleMultiplexer<ArrayHandleTypes...>),
    (viskores::cont::ArrayHandle<typename Traits::ValueType, typename Traits::StorageTag>));

  template <typename RealStorageTag>
  VISKORES_CONT ArrayHandleMultiplexer(
    const viskores::cont::ArrayHandle<ValueType, RealStorageTag>& src)
    : Superclass(StorageType::CreateBuffers(src))
  {
  }

  VISKORES_CONT bool IsValid() const { return StorageType::IsValid(this->GetBuffers()); }

  template <typename S>
  VISKORES_CONT void SetArray(const viskores::cont::ArrayHandle<ValueType, S>& src)
  {
    this->SetBuffers(StorageType::CreateBuffers(src));
  }

  VISKORES_CONT auto GetArrayHandleVariant() const
    -> decltype(StorageType::GetArrayHandleVariant(this->GetBuffers()))
  {
    return StorageType::GetArrayHandleVariant(this->GetBuffers());
  }
};

/// \brief Converts a`viskores::List` to an `ArrayHandleMultiplexer`
///
/// The argument of this template must be a `viskores::List` and furthermore all the types in
/// the list tag must be some type of \c ArrayHandle. The templated type gets aliased to
/// an \c ArrayHandleMultiplexer that can store any of these ArrayHandle types.
///
template <typename List>
using ArrayHandleMultiplexerFromList = viskores::ListApply<List, ArrayHandleMultiplexer>;

namespace internal
{

namespace detail
{

struct ArrayExtractComponentMultiplexerFunctor
{
  template <typename ArrayType>
  auto operator()(const ArrayType& array,
                  viskores::IdComponent componentIndex,
                  viskores::CopyFlag allowCopy) const
    -> decltype(viskores::cont::ArrayExtractComponent(array, componentIndex, allowCopy))
  {
    return viskores::cont::internal::ArrayExtractComponentImpl<typename ArrayType::StorageTag>{}(
      array, componentIndex, allowCopy);
  }
};

} // namespace detail

template <typename... Ss>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagMultiplexer<Ss...>>
{
  template <typename T>
  viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<Ss...>>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy)
  {
    viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandle<T, Ss>...> array(src);
    return array.GetArrayHandleVariant().CastAndCall(
      detail::ArrayExtractComponentMultiplexerFunctor{}, componentIndex, allowCopy);
  }
};

} // namespace internal

} // namespace cont

} // namespace viskores

#endif //viskores_cont_ArrayHandleMultiplexer_h
