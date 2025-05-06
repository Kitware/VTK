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
#ifndef viskores_cont_ArrayHandleExtractComponent_h
#define viskores_cont_ArrayHandleExtractComponent_h

#include <viskores/StaticAssert.h>
#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace internal
{

template <typename PortalType>
class VISKORES_ALWAYS_EXPORT ArrayPortalExtractComponent
{
  using Writable = viskores::internal::PortalSupportsSets<PortalType>;

public:
  using VectorType = typename PortalType::ValueType;
  using Traits = viskores::VecTraits<VectorType>;
  using ValueType = typename Traits::ComponentType;

  VISKORES_EXEC_CONT
  ArrayPortalExtractComponent()
    : Portal()
    , Component(0)
  {
  }

  VISKORES_EXEC_CONT
  ArrayPortalExtractComponent(const PortalType& portal, viskores::IdComponent component)
    : Portal(portal)
    , Component(component)
  {
  }

  ArrayPortalExtractComponent(const ArrayPortalExtractComponent&) = default;
  ArrayPortalExtractComponent(ArrayPortalExtractComponent&&) = default;
  ArrayPortalExtractComponent& operator=(const ArrayPortalExtractComponent&) = default;
  ArrayPortalExtractComponent& operator=(ArrayPortalExtractComponent&&) = default;

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    return Traits::GetComponent(this->Portal.Get(index), this->Component);
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    VectorType vec = this->Portal.Get(index);
    Traits::SetComponent(vec, this->Component, value);
    this->Portal.Set(index, vec);
  }

  VISKORES_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
  viskores::IdComponent Component;
}; // class ArrayPortalExtractComponent

} // namespace internal

namespace cont
{

template <typename ArrayHandleType>
class StorageTagExtractComponent
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename viskores::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
              StorageTagExtractComponent<ArrayHandleType>>
{
  using SourceValueType = typename ArrayHandleType::ValueType;
  using ValueType = typename viskores::VecTraits<SourceValueType>::ComponentType;
  using SourceStorage = typename ArrayHandleType::StorageType;

public:
  VISKORES_CONT static viskores::IdComponent ComponentIndex(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<viskores::IdComponent>();
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> SourceBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

  using ReadPortalType =
    viskores::internal::ArrayPortalExtractComponent<typename SourceStorage::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalExtractComponent<typename SourceStorage::WritePortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<ValueType>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(SourceBuffers(buffers));
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const ValueType&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadType("Fill not supported for ArrayHandleExtractComponent.");
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    SourceStorage::ResizeBuffers(numValues, SourceBuffers(buffers), preserve, token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                          ComponentIndex(buffers));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(SourceStorage::CreateWritePortal(SourceBuffers(buffers), device, token),
                           ComponentIndex(buffers));
  }

  VISKORES_CONT static auto CreateBuffers(viskores::IdComponent componentIndex = 0,
                                          const ArrayHandleType& array = ArrayHandleType{})
    -> decltype(viskores::cont::internal::CreateBuffers())
  {
    return viskores::cont::internal::CreateBuffers(componentIndex, array);
  }
}; // class Storage

}
}
} // namespace viskores::cont::internal

namespace viskores
{
namespace cont
{

/// @brief A fancy `ArrayHandle` that turns a vector array into a scalar array by
/// slicing out a single component of each vector.
///
/// `ArrayHandleExtractComponent` is a specialization of `ArrayHandle`. It takes an
/// input `ArrayHandle` with a `viskores::Vec` `ValueType` and a component index
/// and uses this information to expose a scalar array consisting of the
/// specified component across all vectors in the input `ArrayHandle`. So for a
/// given index i, `ArrayHandleExtractComponent` looks up the i-th `viskores::Vec` in
/// the index array and reads or writes to the specified component, leave all
/// other components unmodified. This is done on the fly rather than creating a
/// copy of the array.
template <typename ArrayHandleType>
class ArrayHandleExtractComponent
  : public viskores::cont::ArrayHandle<
      typename viskores::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>
{
public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExtractComponent,
    (ArrayHandleExtractComponent<ArrayHandleType>),
    (viskores::cont::ArrayHandle<
      typename viskores::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>));

  /// Construct an `ArrayHandleExtractComponent` with a given array and component.
  VISKORES_CONT
  ArrayHandleExtractComponent(const ArrayHandleType& array, viskores::IdComponent component)
    : Superclass(StorageType::CreateBuffers(component, array))
  {
  }

  /// Get the component index being extracted from the source array.
  VISKORES_CONT viskores::IdComponent GetComponent() const
  {
    return StorageType::ComponentIndex(this->GetBuffers());
  }

  /// Get the source array of `Vec`s to get a component out of.
  VISKORES_CONT ArrayHandleType GetArray() const
  {
    using BaseArray = viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                  typename ArrayHandleType::StorageTag>;
    return ArrayHandleType(BaseArray(StorageType::SourceBuffers(this->GetBuffers())));
  }
};

/// `make_ArrayHandleExtractComponent` is convenience function to generate an
/// `ArrayHandleExtractComponent`.
template <typename ArrayHandleType>
VISKORES_CONT ArrayHandleExtractComponent<ArrayHandleType> make_ArrayHandleExtractComponent(
  const ArrayHandleType& array,
  viskores::IdComponent component)
{
  return ArrayHandleExtractComponent<ArrayHandleType>(array, component);
}

namespace internal
{

///@cond
// Doxygen has trouble parsing this, and it is not important to document.

template <typename ArrayHandleType>
struct ArrayExtractComponentImpl<viskores::cont::StorageTagExtractComponent<ArrayHandleType>>
{
  auto operator()(const viskores::cont::ArrayHandleExtractComponent<ArrayHandleType>& src,
                  viskores::IdComponent componentIndex,
                  viskores::CopyFlag allowCopy) const
    -> decltype(ArrayExtractComponentImpl<typename ArrayHandleType::StorageTag>{}(
      std::declval<ArrayHandleType>(),
      componentIndex,
      allowCopy))
  {
    using ValueType = typename ArrayHandleType::ValueType;
    using ComponentType = typename viskores::VecTraits<ValueType>::ComponentType;
    using FlatComponent = viskores::VecFlat<ComponentType>;
    constexpr viskores::IdComponent FLAT_SUB_COMPONENTS = FlatComponent::NUM_COMPONENTS;
    return ArrayExtractComponentImpl<typename ArrayHandleType::StorageTag>{}(
      src.GetArray(), (src.GetComponent() * FLAT_SUB_COMPONENTS) + componentIndex, allowCopy);
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

template <typename AH>
struct SerializableTypeString<viskores::cont::ArrayHandleExtractComponent<AH>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_ExtractComponent<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename AH>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<typename viskores::VecTraits<typename AH::ValueType>::ComponentType,
                              viskores::cont::StorageTagExtractComponent<AH>>>
  : SerializableTypeString<viskores::cont::ArrayHandleExtractComponent<AH>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<viskores::cont::ArrayHandleExtractComponent<AH>>
{
private:
  using Type = viskores::cont::ArrayHandleExtractComponent<AH>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, Type(obj).GetComponent());
    viskoresdiy::save(bb, Type(obj).GetArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::IdComponent component = 0;
    AH array;
    viskoresdiy::load(bb, component);
    viskoresdiy::load(bb, array);

    obj = viskores::cont::make_ArrayHandleExtractComponent(array, component);
  }
};

template <typename AH>
struct Serialization<
  viskores::cont::ArrayHandle<typename viskores::VecTraits<typename AH::ValueType>::ComponentType,
                              viskores::cont::StorageTagExtractComponent<AH>>>
  : Serialization<viskores::cont::ArrayHandleExtractComponent<AH>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif // viskores_cont_ArrayHandleExtractComponent_h
