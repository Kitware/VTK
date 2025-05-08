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
#ifndef viskores_cont_ArrayHandleSOA_h
#define viskores_cont_ArrayHandleSOA_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/Math.h>
#include <viskores/VecTraits.h>

#include <viskores/internal/ArrayPortalBasic.h>
#include <viskores/internal/ArrayPortalHelpers.h>

#include <viskoresstd/integer_sequence.h>

#include <array>
#include <limits>
#include <type_traits>

namespace viskores
{

namespace internal
{

/// \brief An array portal that combines indices from multiple sources.
///
/// This will only work if \c VecTraits is defined for the type.
///
template <typename ValueType_, typename ComponentPortalType>
class ArrayPortalSOA
{
public:
  using ValueType = ValueType_;

private:
  using ComponentType = typename ComponentPortalType::ValueType;

  using VTraits = viskores::VecTraits<ValueType>;
  VISKORES_STATIC_ASSERT((std::is_same<typename VTraits::ComponentType, ComponentType>::value));
  static constexpr viskores::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

  ComponentPortalType Portals[NUM_COMPONENTS];
  viskores::Id NumberOfValues;

public:
  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT explicit ArrayPortalSOA(viskores::Id numValues = 0)
    : NumberOfValues(numValues)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT void SetPortal(viskores::IdComponent index, const ComponentPortalType& portal)
  {
    this->Portals[index] = portal;
  }

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  template <typename SPT = ComponentPortalType,
            typename Supported = typename viskores::internal::PortalSupportsGets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  VISKORES_EXEC_CONT ValueType Get(viskores::Id valueIndex) const
  {
    return this->Get(valueIndex, viskoresstd::make_index_sequence<NUM_COMPONENTS>());
  }

  template <typename SPT = ComponentPortalType,
            typename Supported = typename viskores::internal::PortalSupportsSets<SPT>::type,
            typename = typename std::enable_if<Supported::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id valueIndex, const ValueType& value) const
  {
    this->Set(valueIndex, value, viskoresstd::make_index_sequence<NUM_COMPONENTS>());
  }

private:
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <std::size_t I>
  VISKORES_EXEC_CONT ComponentType GetComponent(viskores::Id valueIndex) const
  {
    return this->Portals[I].Get(valueIndex);
  }

  template <std::size_t... I>
  VISKORES_EXEC_CONT ValueType Get(viskores::Id valueIndex, viskoresstd::index_sequence<I...>) const
  {
    return ValueType{ this->GetComponent<I>(valueIndex)... };
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <std::size_t I>
  VISKORES_EXEC_CONT bool SetComponent(viskores::Id valueIndex, const ValueType& value) const
  {
    this->Portals[I].Set(valueIndex,
                         VTraits::GetComponent(value, static_cast<viskores::IdComponent>(I)));
    return true;
  }

  template <std::size_t... I>
  VISKORES_EXEC_CONT void Set(viskores::Id valueIndex,
                              const ValueType& value,
                              viskoresstd::index_sequence<I...>) const
  {
    // Is there a better way to unpack an expression and execute them with no other side effects?
    (void)std::initializer_list<bool>{ this->SetComponent<I>(valueIndex, value)... };
  }
};

} // namespace internal

namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagSOA
{
};

namespace internal
{

template <typename ComponentType, viskores::IdComponent NUM_COMPONENTS>
class VISKORES_ALWAYS_EXPORT
  Storage<viskores::Vec<ComponentType, NUM_COMPONENTS>, viskores::cont::StorageTagSOA>
{
  using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

public:
  using ReadPortalType =
    viskores::internal::ArrayPortalSOA<ValueType,
                                       viskores::internal::ArrayPortalBasicRead<ComponentType>>;
  using WritePortalType =
    viskores::internal::ArrayPortalSOA<ValueType,
                                       viskores::internal::ArrayPortalBasicWrite<ComponentType>>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return std::vector<viskores::cont::internal::Buffer>(static_cast<std::size_t>(NUM_COMPONENTS));
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<ComponentType>::NUM_COMPONENTS * NUM_COMPONENTS;
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    viskores::BufferSizeType numBytes =
      viskores::internal::NumberOfValuesToNumberOfBytes<ComponentType>(numValues);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      buffers[componentIndex].SetNumberOfBytes(numBytes, preserve, token);
    }
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    // Assume all buffers are the same size.
    return static_cast<viskores::Id>(buffers[0].GetNumberOfBytes()) /
      static_cast<viskores::Id>(sizeof(ComponentType));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const ValueType& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    constexpr viskores::BufferSizeType sourceSize =
      static_cast<viskores::BufferSizeType>(sizeof(ComponentType));
    viskores::BufferSizeType startByte = startIndex * sourceSize;
    viskores::BufferSizeType endByte = endIndex * sourceSize;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      ComponentType source = fillValue[componentIndex];
      buffers[componentIndex].Fill(&source, sourceSize, startByte, endByte, token);
    }
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::Id numValues = GetNumberOfValues(buffers);
    ReadPortalType portal(numValues);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      VISKORES_ASSERT(buffers[0].GetNumberOfBytes() == buffers[componentIndex].GetNumberOfBytes());
      portal.SetPortal(componentIndex,
                       viskores::internal::ArrayPortalBasicRead<ComponentType>(
                         reinterpret_cast<const ComponentType*>(
                           buffers[componentIndex].ReadPointerDevice(device, token)),
                         numValues));
    }
    return portal;
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::Id numValues = GetNumberOfValues(buffers);
    WritePortalType portal(numValues);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      VISKORES_ASSERT(buffers[0].GetNumberOfBytes() == buffers[componentIndex].GetNumberOfBytes());
      portal.SetPortal(componentIndex,
                       viskores::internal::ArrayPortalBasicWrite<ComponentType>(
                         reinterpret_cast<ComponentType*>(
                           buffers[componentIndex].WritePointerDevice(device, token)),
                         numValues));
    }
    return portal;
  }
};

} // namespace internal

/// @brief An `ArrayHandle` that for Vecs stores each component in a separate physical array.
///
/// `ArrayHandleSOA` behaves like a regular `ArrayHandle` (with a basic storage) except that
/// if you specify a `ValueType` of a `Vec` or a `Vec-like`, it will actually store each
/// component in a separate physical array. When data are retrieved from the array, they are
/// reconstructed into `Vec` objects as expected.
///
/// The intention of this array type is to help cover the most common ways data is lain out in
/// memory. Typically, arrays of data are either an "array of structures" like the basic storage
/// where you have a single array of structures (like `Vec`) or a "structure of arrays" where
/// you have an array of a basic type (like `float`) for each component of the data being
/// represented. The `ArrayHandleSOA` makes it easy to cover this second case without creating
/// special types.
///
/// `ArrayHandleSOA` can be constructed from a collection of `ArrayHandle` with basic storage.
/// This allows you to construct `Vec` arrays from components without deep copies.
///
template <typename T>
class ArrayHandleSOA : public ArrayHandle<T, viskores::cont::StorageTagSOA>
{
  using ComponentType = typename viskores::VecTraits<T>::ComponentType;
  static constexpr viskores::IdComponent NUM_COMPONENTS = viskores::VecTraits<T>::NUM_COMPONENTS;

  using ComponentArrayType =
    viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagBasic>;

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleSOA,
                                 (ArrayHandleSOA<T>),
                                 (ArrayHandle<T, viskores::cont::StorageTagSOA>));

  ArrayHandleSOA(std::initializer_list<viskores::cont::internal::Buffer>&& componentBuffers)
    : Superclass(componentBuffers)
  {
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// viskores::cont::ArrayHandle<T> components1;
  /// viskores::cont::ArrayHandle<T> components2;
  /// viskores::cont::ArrayHandle<T> components3;
  /// // Fill arrays...
  ///
  /// std::array<T, 3> allComponents{ components1, components2, components3 };
  /// viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>vecarray(allComponents);
  /// @endcode
  ArrayHandleSOA(const std::array<ComponentArrayType, NUM_COMPONENTS>& componentArrays)
  {
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      this->SetArray(componentIndex, componentArrays[componentIndex]);
    }
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// viskores::cont::ArrayHandle<T> components1;
  /// viskores::cont::ArrayHandle<T> components2;
  /// viskores::cont::ArrayHandle<T> components3;
  /// // Fill arrays...
  ///
  /// std::vector<T> allComponents{ components1, components2, components3 };
  /// viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>vecarray(allComponents);
  /// @endcode
  ArrayHandleSOA(const std::vector<ComponentArrayType>& componentArrays)
  {
    VISKORES_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      this->SetArray(componentIndex, componentArrays[componentIndex]);
    }
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// viskores::cont::ArrayHandle<T> components1;
  /// viskores::cont::ArrayHandle<T> components2;
  /// viskores::cont::ArrayHandle<T> components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3> vecarray(
  ///   { components1, components2, components3 });
  /// @endcode
  ArrayHandleSOA(std::initializer_list<ComponentArrayType>&& componentArrays)
  {
    VISKORES_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    viskores::IdComponent componentIndex = 0;
    for (auto&& array : componentArrays)
    {
      this->SetArray(componentIndex, array);
      ++componentIndex;
    }
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// The data is copied from the `std::vector`s to the array handle.
  ///
  /// @code{.cpp}
  /// std::vector<T> components1;
  /// std::vector<T> components2;
  /// std::vector<T> components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOA<viskores::Vec<T, 3>> vecarray(
  ///   { components1, components2, components3 });
  /// @endcode
  ArrayHandleSOA(std::initializer_list<std::vector<ComponentType>>&& componentVectors)
  {
    VISKORES_ASSERT(componentVectors.size() == NUM_COMPONENTS);
    viskores::IdComponent componentIndex = 0;
    for (auto&& vector : componentVectors)
    {
      // Note, std::vectors that come from std::initializer_list must be copied because the scope
      // of the objects in the initializer list disappears.
      this->SetArray(componentIndex,
                     viskores::cont::make_ArrayHandle(vector, viskores::CopyFlag::On));
      ++componentIndex;
    }
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// The first argument is a `viskores::CopyFlag` to determine whether the input arrays
  /// should be copied.
  /// The component arrays are listed as arguments.
  /// This only works if all the templated arguments are of type `std::vector<ComponentType>`.
  ///
  /// @code{.cpp}
  /// std::vector<T> components1;
  /// std::vector<T> components2;
  /// std::vector<T> components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOA<viskores::Vec<T, 3>> vecarray(
  ///   viskores::CopyFlag::On, components1, components2, components3);
  /// @endcode
  template <typename Allocator, typename... RemainingVectors>
  ArrayHandleSOA(viskores::CopyFlag copy,
                 const std::vector<ComponentType, Allocator>& vector0,
                 RemainingVectors&&... componentVectors)
    : Superclass(std::vector<viskores::cont::internal::Buffer>{
        viskores::cont::make_ArrayHandle(vector0, copy).GetBuffers()[0],
        viskores::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors), copy)
          .GetBuffers()[0]... })
  {
    VISKORES_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == NUM_COMPONENTS);
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// The first argument is a `viskores::CopyFlag` to determine whether the input arrays
  /// should be copied.
  /// The component arrays are listed as arguments.
  /// This only works if all the templated arguments are rvalues of type
  /// `std::vector<ComponentType>`.
  ///
  /// @code{.cpp}
  /// std::vector<T> components1;
  /// std::vector<T> components2;
  /// std::vector<T> components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOA<viskores::Vec<T, N> vecarray(viskores::CopyFlag::Off,
  ///                                                     std::move(components1),
  ///                                                     std::move(components2),
  ///                                                     std::move(components3);
  /// @endcode
  template <typename... RemainingVectors>
  ArrayHandleSOA(viskores::CopyFlag copy,
                 std::vector<ComponentType>&& vector0,
                 RemainingVectors&&... componentVectors)
    : Superclass(std::vector<viskores::cont::internal::Buffer>{
        viskores::cont::make_ArrayHandle(std::move(vector0), copy),
        viskores::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors), copy)
          .GetBuffers()[0]... })
  {
    VISKORES_STATIC_ASSERT(sizeof...(RemainingVectors) + 1 == NUM_COMPONENTS);
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// T* components1;
  /// T* components2;
  /// T* components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOA<viskores::Vec<T, 3>>(
  ///   { components1, components2, components3 }, size, viskores::CopyFlag::On);
  /// @endcode
  ArrayHandleSOA(std::initializer_list<const ComponentType*> componentArrays,
                 viskores::Id length,
                 viskores::CopyFlag copy)
  {
    VISKORES_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    viskores::IdComponent componentIndex = 0;
    for (auto&& vectorIter = componentArrays.begin(); vectorIter != componentArrays.end();
         ++vectorIter)
    {
      this->SetArray(componentIndex, viskores::cont::make_ArrayHandle(*vectorIter, length, copy));
      ++componentIndex;
    }
  }

  /// @brief Construct an `ArrayHandleSOA` from a collection of component arrays.
  ///
  /// The component arrays are listed as arguments.
  /// This only works if all the templated arguments are of type `ComponentType*`.
  ///
  /// @code{.cpp}
  /// T* components1;
  /// T* components2;
  /// T* components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOA<viskores::Vec<T, 3>> vecarray(
  ///   size, viskores::CopyFlag::On, components1, components2, components3);
  /// @endcode
  template <typename... RemainingArrays>
  ArrayHandleSOA(viskores::Id length,
                 viskores::CopyFlag copy,
                 const ComponentType* array0,
                 const RemainingArrays&... componentArrays)
    : Superclass(std::vector<viskores::cont::internal::Buffer>{
        viskores::cont::make_ArrayHandle(array0, length, copy).GetBuffers()[0],
        viskores::cont::make_ArrayHandle(componentArrays, length, copy).GetBuffers()[0]... })
  {
    VISKORES_STATIC_ASSERT(sizeof...(RemainingArrays) + 1 == NUM_COMPONENTS);
  }

  /// @brief Get a basic array representing the component for the given index.
  VISKORES_CONT viskores::cont::ArrayHandleBasic<ComponentType> GetArray(
    viskores::IdComponent index) const
  {
    return ComponentArrayType({ this->GetBuffers()[index] });
  }

  /// @brief Replace a component array.
  VISKORES_CONT void SetArray(viskores::IdComponent index, const ComponentArrayType& array)
  {
    this->SetBuffer(index, array.GetBuffers()[0]);
  }
};

namespace internal
{

template <typename... Remaining>
using VecSizeFromRemaining =
  std::integral_constant<viskores::IdComponent, viskores::IdComponent(sizeof...(Remaining) + 1)>;

} // namespace internal

/// @brief Create a `viskores::cont::ArrayHandleSOA` with an initializer list of array handles.
///
/// @code{.cpp}
/// viskores::cont::ArrayHandle<T> components1;
/// viskores::cont::ArrayHandle<T> components2;
/// viskores::cont::ArrayHandle<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>>(
///   { components1, components2, components3 });
/// @endcode
template <typename ValueType>
VISKORES_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<
    viskores::cont::ArrayHandle<typename viskores::VecTraits<ValueType>::ComponentType,
                                viskores::cont::StorageTagBasic>>&& componentArrays)
{
  return ArrayHandleSOA<ValueType>(std::move(componentArrays));
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with a number of array handles.
///
/// This only works if all the templated arguments are of type
/// `viskores::cont::ArrayHandle<ComponentType>`.
///
/// @code{.cpp}
/// viskores::cont::ArrayHandle<T> components1;
/// viskores::cont::ArrayHandle<T> components2;
/// viskores::cont::ArrayHandle<T> components3;
/// // Fill arrays...
///
/// auto vecarray =
///   viskores::cont::make_ArrayHandleSOA(components1, components2, components3);
/// @endcode
template <typename ComponentType, typename... RemainingArrays>
VISKORES_CONT ArrayHandleSOA<
  viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingArrays...>::value>>
make_ArrayHandleSOA(
  const viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagBasic>&
    componentArray0,
  const RemainingArrays&... componentArrays)
{
  return { componentArray0, componentArrays... };
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with an initializer list of `std::vector`.
///
/// The data is copied from the `std::vector`s to the array handle.
///
/// @code{.cpp}
/// std::vector<T> components1;
/// std::vector<T> components2;
/// std::vector<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>>(
///   { components1, components2, components3 });
/// @endcode
template <typename ValueType>
VISKORES_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<std::vector<typename viskores::VecTraits<ValueType>::ComponentType>>&&
    componentVectors)
{
  return ArrayHandleSOA<ValueType>(std::move(componentVectors));
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with a number of `std::vector`.
///
/// The first argument is a `viskores::CopyFlag` to determine whether the input arrays
/// should be copied.
/// The component arrays are listed as arguments.
/// This only works if all the templated arguments are of type `std::vector<ComponentType>`.
///
/// @code{.cpp}
/// std::vector<T> components1;
/// std::vector<T> components2;
/// std::vector<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOA(
///   viskores::CopyFlag::On, components1, components2, components3);
/// @endcode
template <typename ComponentType, typename... RemainingVectors>
VISKORES_CONT ArrayHandleSOA<
  viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingVectors...>::value>>
make_ArrayHandleSOA(viskores::CopyFlag copy,
                    const std::vector<ComponentType>& vector0,
                    RemainingVectors&&... componentVectors)
{
  // Convert std::vector to ArrayHandle first so that it correctly handles a mix of rvalue args.
  return { viskores::cont::make_ArrayHandle(vector0, copy),
           viskores::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors),
                                            copy)... };
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with a number of `std::vector`.
///
/// The first argument is a `viskores::CopyFlag` to determine whether the input arrays
/// should be copied.
/// The component arrays are listed as arguments.
/// This only works if all the templated arguments are rvalues of type
/// `std::vector<ComponentType>`.
///
/// @code{.cpp}
/// std::vector<T> components1;
/// std::vector<T> components2;
/// std::vector<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOA(viskores::CopyFlag::Off,
///                                                 std::move(components1),
///                                                 std::move(components2),
///                                                 std::move(components3);
/// @endcode
template <typename ComponentType, typename... RemainingVectors>
VISKORES_CONT ArrayHandleSOA<
  viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingVectors...>::value>>
make_ArrayHandleSOA(viskores::CopyFlag copy,
                    std::vector<ComponentType>&& vector0,
                    RemainingVectors&&... componentVectors)
{
  // Convert std::vector to ArrayHandle first so that it correctly handles a mix of rvalue args.
  return ArrayHandleSOA<
    viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingVectors...>::value>>(
    viskores::cont::make_ArrayHandle(std::move(vector0), copy),
    viskores::cont::make_ArrayHandle(std::forward<RemainingVectors>(componentVectors), copy)...);
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with a number of `std::vector`.
///
/// This only works if all the templated arguments are rvalues of type
/// `std::vector<ComponentType>`.
///
/// @code{.cpp}
/// std::vector<T> components1;
/// std::vector<T> components2;
/// std::vector<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOAMove(
///   std::move(components1), std::move(components2), std::move(components3));
/// @endcode
template <typename ComponentType, typename... RemainingVectors>
VISKORES_CONT ArrayHandleSOA<
  viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingVectors...>::value>>
make_ArrayHandleSOAMove(std::vector<ComponentType>&& vector0,
                        RemainingVectors&&... componentVectors)
{
  return { viskores::cont::make_ArrayHandleMove(std::move(vector0)),
           viskores::cont::make_ArrayHandleMove(
             std::forward<RemainingVectors>(componentVectors))... };
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with an initializer list of C arrays.
///
/// @code{.cpp}
/// T* components1;
/// T* components2;
/// T* components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>>(
///   { components1, components2, components3 }, size, viskores::CopyFlag::On);
/// @endcode
template <typename ValueType>
VISKORES_CONT ArrayHandleSOA<ValueType> make_ArrayHandleSOA(
  std::initializer_list<const typename viskores::VecTraits<ValueType>::ComponentType*>&&
    componentVectors,
  viskores::Id length,
  viskores::CopyFlag copy)
{
  return ArrayHandleSOA<ValueType>(std::move(componentVectors), length, copy);
}

/// @brief Create a `viskores::cont::ArrayHandleSOA` with a number of C arrays.
///
/// This only works if all the templated arguments are of type `ComponentType*`.
///
/// @code{.cpp}
/// T* components1;
/// T* components2;
/// T* components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOA(
///   size, viskores::CopyFlag::On, components1, components2, components3);
/// @endcode
template <typename ComponentType, typename... RemainingArrays>
VISKORES_CONT ArrayHandleSOA<
  viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingArrays...>::value>>
make_ArrayHandleSOA(viskores::Id length,
                    viskores::CopyFlag copy,
                    const ComponentType* array0,
                    const RemainingArrays*... componentArrays)
{
  return ArrayHandleSOA<
    viskores::Vec<ComponentType, viskores::IdComponent(sizeof...(RemainingArrays) + 1)>>(
    length, copy, array0, componentArrays...);
}

namespace internal
{

/// @cond

template <>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagSOA>
{
  template <typename T>
  auto operator()(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagSOA>& src,
                  viskores::IdComponent componentIndex,
                  viskores::CopyFlag allowCopy) const
    -> decltype(ArrayExtractComponentImpl<viskores::cont::StorageTagBasic>{}(
      viskores::cont::ArrayHandleBasic<T>{},
      componentIndex,
      allowCopy))
  {
    using FirstLevelComponentType = typename viskores::VecTraits<T>::ComponentType;
    viskores::cont::ArrayHandleSOA<T> array(src);
    constexpr viskores::IdComponent NUM_SUB_COMPONENTS =
      viskores::VecFlat<FirstLevelComponentType>::NUM_COMPONENTS;
    return ArrayExtractComponentImpl<viskores::cont::StorageTagBasic>{}(
      array.GetArray(componentIndex / NUM_SUB_COMPONENTS),
      componentIndex % NUM_SUB_COMPONENTS,
      allowCopy);
  }
};

/// @endcond

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

template <typename ValueType>
struct SerializableTypeString<viskores::cont::ArrayHandleSOA<ValueType>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_SOA<" + SerializableTypeString<ValueType>::Get() + ">";
    return name;
  }
};

template <typename ValueType>
struct SerializableTypeString<viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagSOA>>
  : SerializableTypeString<viskores::cont::ArrayHandleSOA<ValueType>>
{
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <typename ValueType>
struct Serialization<viskores::cont::ArrayHandleSOA<ValueType>>
{
  using BaseType = viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagSOA>;
  static constexpr viskores::IdComponent NUM_COMPONENTS =
    viskores::VecTraits<ValueType>::NUM_COMPONENTS;

  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      viskoresdiy::save(bb, obj.GetBuffers()[componentIndex]);
    }
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    std::vector<viskores::cont::internal::Buffer> buffers(NUM_COMPONENTS);
    for (std::size_t componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      viskoresdiy::load(bb, buffers[componentIndex]);
    }
    obj = BaseType(buffers);
  }
};

template <typename ValueType>
struct Serialization<viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagSOA>>
  : Serialization<viskores::cont::ArrayHandleSOA<ValueType>>
{
};

} // namespace mangled_diy_namespace
// @endcond SERIALIZATION

//=============================================================================
// Precompiled instances

#ifndef viskores_cont_ArrayHandleSOA_cxx

/// @cond

namespace viskores
{
namespace cont
{

#define VISKORES_ARRAYHANDLE_SOA_EXPORT(Type)           \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT   \
    ArrayHandle<viskores::Vec<Type, 2>, StorageTagSOA>; \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT   \
    ArrayHandle<viskores::Vec<Type, 3>, StorageTagSOA>; \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT   \
    ArrayHandle<viskores::Vec<Type, 4>, StorageTagSOA>;

VISKORES_ARRAYHANDLE_SOA_EXPORT(char)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::Int8)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::UInt8)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::Int16)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::UInt16)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::Int32)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::UInt32)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::Int64)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::UInt64)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::Float32)
VISKORES_ARRAYHANDLE_SOA_EXPORT(viskores::Float64)

#undef VISKORES_ARRAYHANDLE_SOA_EXPORT
}
} // namespace viskores::cont

/// @endcond

#endif // !viskores_cont_ArrayHandleSOA_cxx

#endif //viskores_cont_ArrayHandleSOA_h
