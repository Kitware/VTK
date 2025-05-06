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
#ifndef viskores_cont_ArrayHandleRuntimeVec_h
#define viskores_cont_ArrayHandleRuntimeVec_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayPortal.h>
#include <viskores/cont/ErrorBadType.h>

#include <viskores/Assert.h>
#include <viskores/StaticAssert.h>
#include <viskores/VecFromPortal.h>
#include <viskores/VecTraits.h>

namespace viskores
{
namespace internal
{

namespace detail
{

template <typename T>
struct UnrollVecImpl
{
  using type = viskores::Vec<T, 1>;
};

template <typename T, viskores::IdComponent N>
struct UnrollVecImpl<viskores::Vec<T, N>>
{
  using subtype = typename UnrollVecImpl<T>::type;
  using type = viskores::Vec<typename subtype::ComponentType, subtype::NUM_COMPONENTS * N>;
};

} // namespace detail

// A helper class that unrolls a nested `Vec` to a single layer `Vec`. This is similar
// to `viskores::VecFlat`, except that this only flattens `viskores::Vec<T,N>` objects, and not
// any other `Vec`-like objects. The reason is that a `viskores::Vec<T,N>` is the same as N
// consecutive `T` objects whereas the same may not be said about other `Vec`-like objects.
template <typename T>
using UnrollVec = typename detail::UnrollVecImpl<T>::type;

template <typename ComponentsPortalType>
class VISKORES_ALWAYS_EXPORT ArrayPortalRuntimeVec
{
public:
  using ComponentType = typename std::remove_const<typename ComponentsPortalType::ValueType>::type;
  using ValueType = viskores::VecFromPortal<ComponentsPortalType>;

  ArrayPortalRuntimeVec() = default;

  VISKORES_EXEC_CONT ArrayPortalRuntimeVec(const ComponentsPortalType& componentsPortal,
                                           viskores::IdComponent numComponents)
    : ComponentsPortal(componentsPortal)
    , NumberOfComponents(numComponents)
  {
  }

  /// Copy constructor for any other ArrayPortalRuntimeVec with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  template <typename OtherComponentsPortalType>
  VISKORES_EXEC_CONT ArrayPortalRuntimeVec(
    const ArrayPortalRuntimeVec<OtherComponentsPortalType>& src)
    : ComponentsPortal(src.GetComponentsPortal())
    , NumberOfComponents(src.GetNumberOfComponents())
  {
  }

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const
  {
    return this->ComponentsPortal.GetNumberOfValues() / this->NumberOfComponents;
  }

  VISKORES_EXEC_CONT ValueType Get(viskores::Id index) const
  {
    return ValueType(
      this->ComponentsPortal, this->NumberOfComponents, index * this->NumberOfComponents);
  }

  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    if ((&value.GetPortal() == &this->ComponentsPortal) &&
        (value.GetOffset() == (index * this->NumberOfComponents)))
    {
      // The ValueType (VecFromPortal) operates on demand. Thus, if you set
      // something in the value, it has already been passed to the array.
    }
    else
    {
      // The value comes from somewhere else. Copy data in.
      this->Get(index) = value;
    }
  }

  VISKORES_EXEC_CONT const ComponentsPortalType& GetComponentsPortal() const
  {
    return this->ComponentsPortal;
  }

  VISKORES_EXEC_CONT viskores::IdComponent GetNumberOfComponents() const
  {
    return this->NumberOfComponents;
  }

private:
  ComponentsPortalType ComponentsPortal;
  viskores::IdComponent NumberOfComponents = 0;
};

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

struct VISKORES_ALWAYS_EXPORT StorageTagRuntimeVec
{
};

namespace internal
{

struct RuntimeVecMetaData
{
  viskores::IdComponent NumberOfComponents;
};

template <typename ComponentsPortal>
class Storage<viskores::VecFromPortal<ComponentsPortal>, viskores::cont::StorageTagRuntimeVec>
{
  using ComponentType = typename ComponentsPortal::ValueType;
  using ComponentsStorage =
    viskores::cont::internal::Storage<ComponentType, viskores::cont::StorageTagBasic>;

  VISKORES_STATIC_ASSERT_MSG(
    viskores::VecTraits<ComponentType>::NUM_COMPONENTS == 1,
    "ArrayHandleRuntimeVec only supports scalars grouped into a single Vec. Nested Vecs can "
    "still be used with ArrayHandleRuntimeVec. The values are treated as flattened (like "
    "with VecFlat).");

  using ComponentsArray = viskores::cont::ArrayHandle<ComponentType, StorageTagBasic>;

  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<ComponentsPortal, typename ComponentsStorage::WritePortalType>::value),
    "Used invalid ComponentsPortal type with expected ComponentsStorageTag.");

  using Info = RuntimeVecMetaData;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> ComponentsBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  using ReadPortalType =
    viskores::internal::ArrayPortalRuntimeVec<typename ComponentsStorage::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalRuntimeVec<typename ComponentsStorage::WritePortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponents(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<Info>().NumberOfComponents;
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    viskores::IdComponent numComponents = GetNumberOfComponents(buffers);
    viskores::IdComponent numSubComponents =
      ComponentsStorage::GetNumberOfComponentsFlat(ComponentsBuffers(buffers));
    return numComponents * numSubComponents;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ComponentsStorage::GetNumberOfValues(ComponentsBuffers(buffers)) /
      GetNumberOfComponents(buffers);
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    ComponentsStorage::ResizeBuffers(
      numValues * GetNumberOfComponents(buffers), ComponentsBuffers(buffers), preserve, token);
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const viskores::VecFromPortal<ComponentsPortal>&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadType("Fill not supported for ArrayHandleRuntimeVec.");
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(
      ComponentsStorage::CreateReadPortal(ComponentsBuffers(buffers), device, token),
      GetNumberOfComponents(buffers));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(
      ComponentsStorage::CreateWritePortal(ComponentsBuffers(buffers), device, token),
      GetNumberOfComponents(buffers));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    viskores::IdComponent numComponents = 1,
    const ComponentsArray& componentsArray = ComponentsArray{})
  {
    VISKORES_LOG_IF_S(viskores::cont::LogLevel::Warn,
                      (componentsArray.GetNumberOfValues() % numComponents) != 0,
                      "Array given to ArrayHandleRuntimeVec has size ("
                        << componentsArray.GetNumberOfValues()
                        << ") that is not divisible by the number of components selected ("
                        << numComponents << ").");
    Info info;
    info.NumberOfComponents = numComponents;
    return viskores::cont::internal::CreateBuffers(info, componentsArray);
  }

  VISKORES_CONT static ComponentsArray GetComponentsArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ComponentsArray(ComponentsBuffers(buffers));
  }

  VISKORES_CONT static void AsArrayHandleBasic(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagBasic>& dest)
  {
    if (GetNumberOfComponents(buffers) != 1)
    {
      throw viskores::cont::ErrorBadType(
        "Attempted to pull a scalar array from an ArrayHandleRuntime that does not hold scalars.");
    }
    dest = GetComponentsArray(buffers);
  }

  template <viskores::IdComponent N>
  VISKORES_CONT static void AsArrayHandleBasic(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::ArrayHandle<viskores::Vec<ComponentType, N>, viskores::cont::StorageTagBasic>&
      dest)
  {
    if (GetNumberOfComponents(buffers) != N)
    {
      throw viskores::cont::ErrorBadType(
        "Attempted to pull an array of Vecs of the wrong size from an ArrayHandleRuntime.");
    }
    dest =
      viskores::cont::ArrayHandle<viskores::Vec<ComponentType, N>, viskores::cont::StorageTagBasic>(
        ComponentsBuffers(buffers));
  }

  template <typename T, viskores::IdComponent NInner, viskores::IdComponent NOuter>
  VISKORES_CONT static void AsArrayHandleBasic(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec<T, NInner>, NOuter>,
                                viskores::cont::StorageTagBasic>& dest)
  {
    // Flatten the Vec by one level and attempt to get the array handle for that.
    viskores::cont::ArrayHandleBasic<viskores::Vec<T, NInner * NOuter>> squashedArray;
    AsArrayHandleBasic(buffers, squashedArray);
    // Now unsquash the array by stealling the buffers and creating an array of the right type
    dest = viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec<T, NInner>, NOuter>,
                                       viskores::cont::StorageTagBasic>(squashedArray.GetBuffers());
  }
};

} // namespace internal

/// @brief Fancy array handle for a basic array with runtime selected vec size.
///
/// It is sometimes the case that you need to create an array of `Vec`s where
/// the number of components is not known until runtime. This is problematic
/// for normal `ArrayHandle`s because you have to specify the size of the `Vec`s
/// as a template parameter at compile time. `ArrayHandleRuntimeVec` can be used
/// in this case.
///
/// Note that caution should be used with `ArrayHandleRuntimeVec` because the
/// size of the `Vec` values is not known at compile time. Thus, the value
/// type of this array is forced to a special `VecFromPortal` class that can cause
/// surprises if treated as a `Vec`. In particular, the static `NUM_COMPONENTS`
/// expression does not exist. Furthermore, new variables of type `VecFromPortal`
/// cannot be created. This means that simple operators like `+` will not work
/// because they require an intermediate object to be created. (Equal operators
/// like `+=` do work because they are given an existing variable to place the
/// output.)
///
/// It is possible to provide an `ArrayHandleBasic` of the same component
/// type as the underlying storage for this array. In this case, the array
/// will be accessed much in the same manner as `ArrayHandleGroupVec`.
///
/// `ArrayHandleRuntimeVec` also allows you to convert the array to an
/// `ArrayHandleBasic` of the appropriate `Vec` type (or `component` type).
/// A runtime check will be performed to make sure the number of components
/// matches.
///
template <typename ComponentType>
class ArrayHandleRuntimeVec
  : public viskores::cont::ArrayHandle<
      viskores::VecFromPortal<typename ArrayHandleBasic<ComponentType>::WritePortalType>,
      viskores::cont::StorageTagRuntimeVec>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleRuntimeVec,
    (ArrayHandleRuntimeVec<ComponentType>),
    (viskores::cont::ArrayHandle<
      viskores::VecFromPortal<typename ArrayHandleBasic<ComponentType>::WritePortalType>,
      viskores::cont::StorageTagRuntimeVec>));

private:
  using ComponentsArrayType = viskores::cont::ArrayHandle<ComponentType, StorageTagBasic>;

public:
  /// @brief Construct an `ArrayHandleRuntimeVec` with a given number of components.
  ///
  /// @param  numComponents The size of the `Vec`s stored in the array. This must be
  /// specified at the time of construction.
  ///
  /// @param componentsArray This optional parameter allows you to supply a basic array
  /// that holds the components. This provides a mechanism to group consecutive values
  /// into vectors.
  VISKORES_CONT
  ArrayHandleRuntimeVec(viskores::IdComponent numComponents,
                        const ComponentsArrayType& componentsArray = ComponentsArrayType{})
    : Superclass(StorageType::CreateBuffers(numComponents, componentsArray))
  {
  }

  /// @brief Return the number of components in each vec value.
  VISKORES_CONT viskores::IdComponent GetNumberOfComponents() const
  {
    return StorageType::GetNumberOfComponents(this->GetBuffers());
  }

  /// @brief Return a basic array containing the components stored in this array.
  ///
  /// The returned array is shared with this object. Modifying the contents of one array
  /// will modify the other.
  VISKORES_CONT viskores::cont::ArrayHandleBasic<ComponentType> GetComponentsArray() const
  {
    return StorageType::GetComponentsArray(this->GetBuffers());
  }

  /// @brief Converts the array to that of a basic array handle.
  ///
  /// This method converts the `ArrayHandleRuntimeVec` to a simple `ArrayHandleBasic`.
  /// This is useful if the `ArrayHandleRuntimeVec` is passed to a routine that works
  /// on an array of a specific `Vec` size (or scalars). After a runtime check, the
  /// array can be converted to a typical array and used as such.
  template <typename ValueType>
  void AsArrayHandleBasic(viskores::cont::ArrayHandle<ValueType>& array) const
  {
    StorageType::AsArrayHandleBasic(this->GetBuffers(), array);
  }

  /// @copydoc AsArrayHandleBasic
  template <typename ArrayType>
  ArrayType AsArrayHandleBasic() const
  {
    ArrayType array;
    this->AsArrayHandleBasic(array);
    return array;
  }
};

/// `make_ArrayHandleRuntimeVec` is convenience function to generate an
/// `ArrayHandleRuntimeVec`. It takes the number of components stored in
/// each value's `Vec`, which must be specified on the construction of
/// the `ArrayHandleRuntimeVec`. If not specified, the number of components
/// is set to 1. `make_ArrayHandleRuntimeVec` can also optionally take an
/// existing array of components, which will be grouped into `Vec` values
/// based on the specified number of components.
///
template <typename T>
VISKORES_CONT auto make_ArrayHandleRuntimeVec(
  viskores::IdComponent numComponents,
  const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& componentsArray =
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>{})
{
  using UnrolledVec = viskores::internal::UnrollVec<T>;
  using ComponentType = typename UnrolledVec::ComponentType;

  // Use some dangerous magic to convert the basic array to its base component and create
  // an ArrayHandleRuntimeVec from that.
  viskores::cont::ArrayHandle<ComponentType, viskores::cont::StorageTagBasic> flatComponents(
    componentsArray.GetBuffers());

  return viskores::cont::ArrayHandleRuntimeVec<ComponentType>(
    numComponents * UnrolledVec::NUM_COMPONENTS, flatComponents);
}

/// Converts a basic array handle into an `ArrayHandleRuntimeVec` with 1 component. The
/// constructed array is essentially equivalent but of a different type.
template <typename T>
VISKORES_CONT auto make_ArrayHandleRuntimeVec(
  const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& componentsArray)
{
  return make_ArrayHandleRuntimeVec(1, componentsArray);
}

/// A convenience function for creating an `ArrayHandleRuntimeVec` from a standard C array.
///
template <typename T>
VISKORES_CONT auto make_ArrayHandleRuntimeVec(viskores::IdComponent numComponents,
                                              const T* array,
                                              viskores::Id numberOfValues,
                                              viskores::CopyFlag copy)
{
  return make_ArrayHandleRuntimeVec(numComponents,
                                    viskores::cont::make_ArrayHandle(array, numberOfValues, copy));
}

/// A convenience function to move a user-allocated array into an `ArrayHandleRuntimeVec`.
/// The provided array pointer will be reset to `nullptr`.
/// If the array was not allocated with the `new[]` operator, then deleter and reallocater
/// functions must be provided.
///
template <typename T>
VISKORES_CONT auto make_ArrayHandleRuntimeVecMove(
  viskores::IdComponent numComponents,
  T*& array,
  viskores::Id numberOfValues,
  viskores::cont::internal::BufferInfo::Deleter deleter = internal::SimpleArrayDeleter<T>,
  viskores::cont::internal::BufferInfo::Reallocater reallocater =
    internal::SimpleArrayReallocater<T>)
{
  return make_ArrayHandleRuntimeVec(
    numComponents,
    viskores::cont::make_ArrayHandleMove(array, numberOfValues, deleter, reallocater));
}

/// A convenience function for creating an `ArrayHandleRuntimeVec` from an `std::vector`.
///
template <typename T, typename Allocator>
VISKORES_CONT auto make_ArrayHandleRuntimeVec(viskores::IdComponent numComponents,
                                              const std::vector<T, Allocator>& array,
                                              viskores::CopyFlag copy)
{
  return make_ArrayHandleRuntimeVec(numComponents, viskores::cont::make_ArrayHandle(array, copy));
}

/// Move an `std::vector` into an `ArrayHandleRuntimeVec`.
///
template <typename T, typename Allocator>
VISKORES_CONT auto make_ArrayHandleRuntimeVecMove(viskores::IdComponent numComponents,
                                                  std::vector<T, Allocator>&& array)
{
  return make_ArrayHandleRuntimeVec(numComponents, make_ArrayHandleMove(std::move(array)));
}

template <typename T, typename Allocator>
VISKORES_CONT auto make_ArrayHandleRuntimeVec(viskores::IdComponent numComponents,
                                              std::vector<T, Allocator>&& array,
                                              viskores::CopyFlag viskoresNotUsed(copy))
{
  return make_ArrayHandleRuntimeVecMove(numComponents, std::move(array));
}

namespace internal
{

template <>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagRuntimeVec>
{
  template <typename T>
  auto operator()(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagRuntimeVec>& src,
                  viskores::IdComponent componentIndex,
                  viskores::CopyFlag allowCopy) const
  {
    using ComponentType = typename T::ComponentType;
    viskores::cont::ArrayHandleRuntimeVec<ComponentType> array{ src };
    constexpr viskores::IdComponent NUM_SUB_COMPONENTS =
      viskores::VecFlat<ComponentType>::NUM_COMPONENTS;
    viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> dest =
      ArrayExtractComponentImpl<viskores::cont::StorageTagBasic>{}(
        array.GetComponentsArray(), componentIndex % NUM_SUB_COMPONENTS, allowCopy);

    // Adjust stride and offset to expectations of grouped values
    const viskores::IdComponent numComponents = array.GetNumberOfComponents();
    return viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>(
      dest.GetBasicArray(),
      dest.GetNumberOfValues() / numComponents,
      dest.GetStride() * numComponents,
      dest.GetOffset() + (dest.GetStride() * (componentIndex / NUM_SUB_COMPONENTS)),
      dest.GetModulo(),
      dest.GetDivisor());
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

template <typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleRuntimeVec<T>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_RuntimeVec<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename VecType>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<VecType, viskores::cont::StorageTagRuntimeVec>>
  : SerializableTypeString<viskores::cont::ArrayHandleRuntimeVec<typename VecType::ComponentType>>
{
};

}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<viskores::cont::ArrayHandleRuntimeVec<T>>
{
private:
  using Type = viskores::cont::ArrayHandleRuntimeVec<T>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, Type(obj).GetNumberOfComponents());
    viskoresdiy::save(bb, Type(obj).GetComponentsArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::IdComponent numComponents;
    viskores::cont::ArrayHandleBasic<T> componentArray;

    viskoresdiy::load(bb, numComponents);
    viskoresdiy::load(bb, componentArray);

    obj = viskores::cont::make_ArrayHandleRuntimeVec(numComponents, componentArray);
  }
};

template <typename VecType>
struct Serialization<viskores::cont::ArrayHandle<VecType, viskores::cont::StorageTagRuntimeVec>>
  : Serialization<viskores::cont::ArrayHandleRuntimeVec<typename VecType::ComponentType>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleRuntimeVec_h
