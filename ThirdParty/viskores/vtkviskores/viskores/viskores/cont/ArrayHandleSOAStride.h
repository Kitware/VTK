//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_ArrayHandleSOAStride_h
#define viskores_cont_ArrayHandleSOAStride_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleStride.h>

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

template <typename T>
class ArrayPortalSOAStrideRead
{
  const T* Array = nullptr;
  viskores::Id NumberOfValues = 0;
  viskores::IdComponent Stride = 1;

public:
  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    return detail::ArrayPortalBasicReadGet(this->Array + (index * this->Stride));
  }

  ArrayPortalSOAStrideRead() = default;
  ArrayPortalSOAStrideRead(ArrayPortalSOAStrideRead&&) = default;
  ArrayPortalSOAStrideRead(const ArrayPortalSOAStrideRead&) = default;
  ArrayPortalSOAStrideRead& operator=(ArrayPortalSOAStrideRead&&) = default;
  ArrayPortalSOAStrideRead& operator=(const ArrayPortalSOAStrideRead&) = default;

  VISKORES_CONT ArrayPortalSOAStrideRead(const T* array,
                                         viskores::Id numberOfValues,
                                         viskores::IdComponent stride,
                                         viskores::IdComponent offset)
    : Array(array + offset)
    , NumberOfValues(numberOfValues)
    , Stride(stride)
  {
  }
};

template <typename T>
class ArrayPortalSOAStrideWrite
{
  T* Array = nullptr;
  viskores::Id NumberOfValues = 0;
  viskores::IdComponent Stride = 1;

public:
  using ValueType = T;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfValues);

    return detail::ArrayPortalBasicWriteGet(this->Array + (index * this->Stride));
  }

  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    VISKORES_ASSERT(index >= 0);
    VISKORES_ASSERT(index < this->NumberOfValues);

    detail::ArrayPortalBasicWriteSet(this->Array + (index * this->Stride), value);
  }

  ArrayPortalSOAStrideWrite() = default;
  ArrayPortalSOAStrideWrite(ArrayPortalSOAStrideWrite&&) = default;
  ArrayPortalSOAStrideWrite(const ArrayPortalSOAStrideWrite&) = default;
  ArrayPortalSOAStrideWrite& operator=(ArrayPortalSOAStrideWrite&&) = default;
  ArrayPortalSOAStrideWrite& operator=(const ArrayPortalSOAStrideWrite&) = default;

  VISKORES_CONT ArrayPortalSOAStrideWrite(T* array,
                                          viskores::Id numberOfValues,
                                          viskores::IdComponent stride,
                                          viskores::IdComponent offset)
    : Array(array + offset)
    , NumberOfValues(numberOfValues)
    , Stride(stride)
  {
  }
};

} // namespace internal

namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagSOAStride
{
};

namespace internal
{

template <typename ValueType>
class VISKORES_ALWAYS_EXPORT Storage<ValueType, viskores::cont::StorageTagSOAStride>
{
  using VTraits = viskores::VecTraits<ValueType>;
  using ComponentType = typename VTraits::ComponentType;
  static constexpr viskores::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename VTraits::BaseComponentType>::value),
    "ArrayHandleSOAStride currently only supports flat Vec types.");

  static constexpr viskores::IdComponent NUM_BUFFERS_PER_COMPONENT = 2;

  using ComponentStorage =
    viskores::cont::internal::Storage<ComponentType, viskores::cont::StorageTagStride>;

public:
  using ReadPortalType = viskores::internal::
    ArrayPortalSOARead<ValueType, viskores::internal::ArrayPortalSOAStrideRead<ComponentType>>;
  using WritePortalType = viskores::internal::
    ArrayPortalSOAWrite<ValueType, viskores::internal::ArrayPortalSOAStrideWrite<ComponentType>>;

  using ComponentArrayType = viskores::cont::ArrayHandleStride<ComponentType>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    std::vector<viskores::cont::internal::Buffer> buffers;
    buffers.reserve(static_cast<std::size_t>(NUM_COMPONENTS * NUM_BUFFERS_PER_COMPONENT));
    for (viskores::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      ComponentArrayType newArray;
      std::vector<viskores::cont::internal::Buffer> newBuffers = newArray.GetBuffers();
      VISKORES_ASSERT(newBuffers.size() == static_cast<std::size_t>(NUM_BUFFERS_PER_COMPONENT));
      buffers.insert(buffers.end(), newBuffers.begin(), newBuffers.end());
    }
    return buffers;
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> GetComponentBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::IdComponent componentIndex)
  {
    VISKORES_ASSERT(buffers.size() ==
                    static_cast<std::size_t>(NUM_COMPONENTS * NUM_BUFFERS_PER_COMPONENT));
    VISKORES_ASSERT(componentIndex >= 0);
    VISKORES_ASSERT(componentIndex < NUM_COMPONENTS);

    return { buffers.begin() + (NUM_BUFFERS_PER_COMPONENT * componentIndex),
             buffers.begin() + (NUM_BUFFERS_PER_COMPONENT * (componentIndex + 1)) };
  }

  VISKORES_CONT static ComponentArrayType GetComponentArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::IdComponent componentIndex)
  {
    return ComponentArrayType(GetComponentBuffers(buffers, componentIndex));
  }

  VISKORES_CONT static void SetComponentArray(
    std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::IdComponent componentIndex,
    const ComponentArrayType& componentArray)
  {
    VISKORES_ASSERT(buffers.size() ==
                    static_cast<std::size_t>(NUM_COMPONENTS * NUM_BUFFERS_PER_COMPONENT));
    VISKORES_ASSERT(componentIndex >= 0);
    VISKORES_ASSERT(componentIndex < NUM_COMPONENTS);

    if ((componentArray.GetModulo() > 0) &&
        (componentArray.GetModulo() < componentArray.GetNumberOfValues()))
    {
      throw viskores::cont::ErrorBadType("ArrayHandleSOAStride does not support modulo.");
    }
    if (componentArray.GetDivisor() > 1)
    {
      throw viskores::cont::ErrorBadType("ArrayHandleSOAStride does not support divisor.");
    }

    const std::vector<viskores::cont::internal::Buffer>& componentBuffers =
      componentArray.GetBuffers();
    VISKORES_ASSERT(componentBuffers.size() == static_cast<std::size_t>(NUM_BUFFERS_PER_COMPONENT));

    for (viskores::IdComponent sourceIndex = 0; sourceIndex < NUM_BUFFERS_PER_COMPONENT;
         ++sourceIndex)
    {
      buffers[static_cast<std::size_t>(componentIndex * NUM_BUFFERS_PER_COMPONENT + sourceIndex)] =
        componentBuffers[static_cast<std::size_t>(sourceIndex)];
    }
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return NUM_COMPONENTS;
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      ComponentStorage::ResizeBuffers(
        numValues, GetComponentBuffers(buffers, componentIndex), preserve, token);
    }
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    // Assume all buffers are the same size.
    return ComponentStorage::GetNumberOfValues(GetComponentBuffers(buffers, 0));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>& buffers,
                                 const ValueType& fillValue,
                                 viskores::Id startIndex,
                                 viskores::Id endIndex,
                                 viskores::cont::Token& token)
  {
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      ComponentStorage::Fill(GetComponentBuffers(buffers, componentIndex),
                             VTraits::GetComponent(fillValue, componentIndex),
                             startIndex,
                             endIndex,
                             token);
    }
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::Id numValues = GetNumberOfValues(buffers);
    ReadPortalType portal;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      // auto componentPortal = ComponentStorage::CreateReadPortal(
      //   GetComponentBuffers(buffers, componentIndex), device, token);
      ComponentArrayType componentArray = GetComponentArray(buffers, componentIndex);
      if ((componentArray.GetModulo() > 0) && (componentArray.GetModulo() < numValues))
      {
        throw viskores::cont::ErrorBadType("ArrayHandleSOAStride does not support modulo.");
      }
      if (componentArray.GetDivisor() > 1)
      {
        throw viskores::cont::ErrorBadType("ArrayHandleSOAStride does not support divisor.");
      }
      auto componentPortal = viskores::internal::ArrayPortalSOAStrideRead<ComponentType>(
        reinterpret_cast<const ComponentType*>(
          componentArray.GetBasicArray().GetBuffers()[0].ReadPointerDevice(device, token)),
        numValues,
        componentArray.GetStride(),
        componentArray.GetOffset());
      VISKORES_ASSERT(componentPortal.GetNumberOfValues() == numValues);
      portal.SetPortal(componentIndex, componentPortal);
    }
    return portal;
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    viskores::Id numValues = GetNumberOfValues(buffers);
    WritePortalType portal;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      // auto componentPortal = ComponentStorage::CreateWritePortal(
      //   GetComponentBuffers(buffers, componentIndex), device, token);
      ComponentArrayType componentArray = GetComponentArray(buffers, componentIndex);
      if ((componentArray.GetModulo() > 0) && (componentArray.GetModulo() < numValues))
      {
        throw viskores::cont::ErrorBadType("ArrayHandleSOAStride does not support modulo.");
      }
      if (componentArray.GetDivisor() > 1)
      {
        throw viskores::cont::ErrorBadType("ArrayHandleSOAStride does not support divisor.");
      }
      auto componentPortal = viskores::internal::ArrayPortalSOAStrideWrite<ComponentType>(
        reinterpret_cast<ComponentType*>(
          componentArray.GetBasicArray().GetBuffers()[0].WritePointerDevice(device, token)),
        numValues,
        componentArray.GetStride(),
        componentArray.GetOffset());
      VISKORES_ASSERT(componentPortal.GetNumberOfValues() == numValues);
      portal.SetPortal(componentIndex, componentPortal);
    }
    return portal;
  }
};

} // namespace internal

/// @brief An `ArrayHandle` that stores each component in a separate physical array with striding.
///
/// `ArrayHandleSOAStride` behaves much like an `ArrayHandleSOA` in that each component is
/// (potentially) stored in a separate array. However, whereas `ArrayHandleSOA` specifically
/// stores each component in a basic array, `ArrayHandleSOAStride` represents each component
/// as an `ArrayHandleStride`. This gives flexibility in the representation because the values
/// do not have to be tightly packed. The spacing between values can be determined at runtime.
/// This allows `ArrayHandleSOAStride` to represent most memory array layouts. For example,
/// although it behaves like an SOA array, it can point to an AOS array by having each component
/// point to the same physical array with different offsets.
///
/// `ArrayHandleSOAStride` is also similar to `ArrayHandleRecombineVec`. It can be used to
/// represent unknown arrays by extracting each component. The difference is that
/// `ArrayHandleSOAStride` requires a fix sized value where the number of components is known
/// at compile time. In contrast, `ArrayHandleRecombineVec` can work with any size vector
/// defined at runtime. However, `ArrayHandleRecombineVec` requires a dynamically sized `Vec`-like
/// object that has limited use. When `ArrayHandleSOAStride` can be used, it uses the same value
/// type as the array it is mimicking.
template <typename T>
class ArrayHandleSOAStride : public ArrayHandle<T, viskores::cont::StorageTagSOAStride>
{
  using ComponentType = typename viskores::VecTraits<T>::ComponentType;
  static constexpr viskores::IdComponent NUM_COMPONENTS = viskores::VecTraits<T>::NUM_COMPONENTS;

public:
  /// @brief The type of each component array.
  using ComponentArrayType =
    viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagStride>;

  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleSOAStride,
                                 (ArrayHandleSOAStride<T>),
                                 (ArrayHandle<T, viskores::cont::StorageTagSOAStride>));

  /// @brief Construct an `ArrayHandleSOAStride` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// viskores::cont::ArrayHandleStride<T> components1;
  /// viskores::cont::ArrayHandleStride<T> components2;
  /// viskores::cont::ArrayHandleStride<T> components3;
  /// // Fill arrays...
  ///
  /// std::array<viskores::cont::ArrayHandleStride<T>, 3>
  ///   allComponents{ components1, components2, components3 };
  /// viskores::cont::ArrayHandleSOAStride<viskores::Vec<T, 3>> vecarray(allComponents);
  /// @endcode
  ArrayHandleSOAStride(const std::array<ComponentArrayType, NUM_COMPONENTS>& componentArrays)
  {
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      this->SetArray(componentIndex, componentArrays[componentIndex]);
    }
  }

  /// @brief Construct an `ArrayHandleSOAStride` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// viskores::cont::ArrayHandleStride<T> components1;
  /// viskores::cont::ArrayHandleStride<T> components2;
  /// viskores::cont::ArrayHandleStride<T> components3;
  /// // Fill arrays...
  ///
  /// std::vector<viskores::cont::ArrayHandleStride<T>>
  ///   allComponents{ components1, components2, components3 };
  /// viskores::cont::ArrayHandleSOAStride<viskores::Vec<T, 3>> vecarray(allComponents);
  /// @endcode
  ArrayHandleSOAStride(const std::vector<ComponentArrayType>& componentArrays)
  {
    VISKORES_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      this->SetArray(componentIndex, componentArrays[componentIndex]);
    }
  }

  /// @brief Construct an `ArrayHandleSOAStride` from a collection of component arrays.
  ///
  /// @code{.cpp}
  /// viskores::cont::ArrayHandle<T> components1;
  /// viskores::cont::ArrayHandle<T> components2;
  /// viskores::cont::ArrayHandle<T> components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOAStride<viskores::Vec<T, 3> vecarray(
  ///   { components1, components2, components3 });
  /// @endcode
  ArrayHandleSOAStride(std::initializer_list<ComponentArrayType>&& componentArrays)
  {
    VISKORES_ASSERT(componentArrays.size() == NUM_COMPONENTS);
    viskores::IdComponent componentIndex = 0;
    for (auto&& array : componentArrays)
    {
      this->SetArray(componentIndex, array);
      ++componentIndex;
    }
  }

  /// @brief Construct an `ArrayHandleSOAStride` from a collection of component arrays.
  ///
  /// The data is copied from the `std::vector`s to the array handle.
  ///
  /// @code{.cpp}
  /// std::vector<T> components1;
  /// std::vector<T> components2;
  /// std::vector<T> components3;
  /// // Fill arrays...
  ///
  /// viskores::cont::ArrayHandleSOAStride<viskores::Vec<T, 3>> vecarray(
  ///   { components1, components2, components3 });
  /// @endcode
  ArrayHandleSOAStride(std::initializer_list<std::vector<ComponentType>>&& componentVectors)
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

  /// @brief Get a basic array representing the component for the given index.
  VISKORES_CONT ComponentArrayType GetArray(viskores::IdComponent index) const
  {
    return StorageType::GetComponentArray(this->GetBuffers(), index);
  }

  /// @brief Replace a component array.
  VISKORES_CONT void SetArray(viskores::IdComponent index, const ComponentArrayType& array)
  {
    StorageType::SetComponentArray(this->GetBuffers(), index, array);
  }
};

/// @brief Create a `viskores::cont::ArrayHandleSOAStride` with an initializer list of array handles.
///
/// @code{.cpp}
/// viskores::cont::ArrayHandle<T> components1;
/// viskores::cont::ArrayHandle<T> components2;
/// viskores::cont::ArrayHandle<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOAStride<viskores::Vec<T, 3>>(
///   { components1, components2, components3 });
/// @endcode
template <typename ValueType>
VISKORES_CONT ArrayHandleSOAStride<ValueType> make_ArrayHandleSOAStride(
  std::initializer_list<
    viskores::cont::ArrayHandle<typename viskores::VecTraits<ValueType>::ComponentType,
                                viskores::cont::StorageTagBasic>>&& componentArrays)
{
  return ArrayHandleSOAStride<ValueType>(std::move(componentArrays));
}

/// @brief Create a `viskores::cont::ArrayHandleSOAStride` with a number of array handles.
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
///   viskores::cont::make_ArrayHandleSOAStride(components1, components2, components3);
/// @endcode
template <typename ComponentType, typename... RemainingArrays>
VISKORES_CONT ArrayHandleSOAStride<
  viskores::Vec<ComponentType, internal::VecSizeFromRemaining<RemainingArrays...>::value>>
make_ArrayHandleSOAStride(
  const viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagBasic>&
    componentArray0,
  const RemainingArrays&... componentArrays)
{
  return { componentArray0, componentArrays... };
}

/// @brief Create a `viskores::cont::ArrayHandleSOAStride` with a `std::vector` of component arrays.
///
/// The data is copied from the `std::vector`s to the array handle.
///
/// @code{.cpp}
/// std::vector<T> components1;
/// std::vector<T> components2;
/// std::vector<T> components3;
/// // Fill arrays...
///
/// auto vecarray = viskores::cont::make_ArrayHandleSOAStride<viskores::Vec<T, 3>>(
///   { components1, components2, components3 });
/// @endcode
template <typename ValueType>
VISKORES_CONT ArrayHandleSOAStride<ValueType> make_ArrayHandleSOAStride(
  std::initializer_list<std::vector<typename viskores::VecTraits<ValueType>::ComponentType>>&&
    componentVectors)
{
  return ArrayHandleSOAStride<ValueType>(std::move(componentVectors));
}

namespace internal
{

/// @cond

template <>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagSOAStride>
{
  template <typename T>
  auto operator()(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagSOAStride>& src,
                  viskores::IdComponent componentIndex,
                  viskores::CopyFlag viskoresNotUsed(allowCopy)) const
  {
    viskores::cont::ArrayHandleSOAStride<T> array(src);
    return array.GetArray(componentIndex);
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
struct SerializableTypeString<viskores::cont::ArrayHandleSOAStride<ValueType>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_SOAStride<" + SerializableTypeString<ValueType>::Get() + ">";
    return name;
  }
};

template <typename ValueType>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagSOAStride>>
  : SerializableTypeString<viskores::cont::ArrayHandleSOAStride<ValueType>>
{
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <typename ValueType>
struct Serialization<viskores::cont::ArrayHandleSOAStride<ValueType>>
{
  using BaseType = viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagSOAStride>;
  static constexpr viskores::IdComponent NUM_COMPONENTS =
    viskores::VecTraits<ValueType>::NUM_COMPONENTS;
  using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;

  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj_)
  {
    viskores::cont::ArrayHandleSOAStride<ValueType> obj = obj_;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      viskores::cont::ArrayHandleStride<ComponentType> componentArray =
        obj.GetArray(componentIndex);
      viskoresdiy::save(bb, componentArray.GetNumberOfValues());
      viskoresdiy::save(bb, componentArray.GetStride());
      viskoresdiy::save(bb, componentArray.GetOffset());
      viskoresdiy::save(bb, componentArray.GetModulo());
      viskoresdiy::save(bb, componentArray.GetDivisor());

      viskores::cont::internal::Buffer componentBuffer = componentArray.GetBuffers()[1];
      bool alreadyWritten = false;
      for (viskores::IdComponent checkIndex = 0; checkIndex < componentIndex; ++checkIndex)
      {
        viskores::cont::internal::Buffer checkBuffer = obj.GetArray(checkIndex).GetBuffers()[1];
        if (checkBuffer == componentBuffer)
        {
          viskoresdiy::save(bb, checkIndex);
          alreadyWritten = true;
          break;
        }
      }
      if (!alreadyWritten)
      {
        viskoresdiy::save(bb, viskores::IdComponent{ -1 });
        viskoresdiy::save(bb, componentBuffer);
      }
    }
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj_)
  {
    viskores::cont::ArrayHandleSOAStride<ValueType> obj;
    for (std::size_t componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      viskores::Id numValues;
      viskores::Id stride;
      viskores::Id offset;
      viskores::Id modulo;
      viskores::Id divisor;
      viskores::cont::internal::Buffer componentBuffer;

      viskoresdiy::load(bb, numValues);
      viskoresdiy::load(bb, stride);
      viskoresdiy::load(bb, offset);
      viskoresdiy::load(bb, modulo);
      viskoresdiy::load(bb, divisor);

      viskores::IdComponent bufferIndex;
      viskoresdiy::load(bb, bufferIndex);
      if (bufferIndex < 0)
      {
        viskoresdiy::load(bb, componentBuffer);
      }
      else
      {
        componentBuffer = obj.GetArray(bufferIndex).GetBuffers()[1];
      }

      viskores::cont::ArrayHandleStride<ComponentType> componentArray(
        componentBuffer, numValues, stride, offset, modulo, divisor);
      obj.SetArray(componentIndex, componentArray);
    }
    obj_ = obj;
  }
};

template <typename ValueType>
struct Serialization<viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagSOAStride>>
  : Serialization<viskores::cont::ArrayHandleSOAStride<ValueType>>
{
};

} // namespace mangled_diy_namespace
// @endcond SERIALIZATION

//=============================================================================
// Precompiled instances

#ifndef viskores_cont_ArrayHandleSOAStride_cxx

/// @cond

namespace viskores
{
namespace cont
{

#define VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(Type)                                          \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagSOAStride>; \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                         \
    ArrayHandle<viskores::Vec<Type, 2>, StorageTagSOAStride>;                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                         \
    ArrayHandle<viskores::Vec<Type, 3>, StorageTagSOAStride>;                                 \
  extern template class VISKORES_CONT_TEMPLATE_EXPORT                                         \
    ArrayHandle<viskores::Vec<Type, 4>, StorageTagSOAStride>;

VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(char)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::Int8)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::UInt8)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::Int16)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::UInt16)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::Int32)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::UInt32)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::Int64)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::UInt64)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::Float32)
VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT(viskores::Float64)

#undef VISKORES_ARRAYHANDLE_SOA_STRIDE_EXPORT
}
} // namespace viskores::cont

/// @endcond

#endif // !viskores_cont_ArrayHandleSOAStride_cxx

#endif //viskores_cont_ArrayHandleSOAStride_h
