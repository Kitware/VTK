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
#ifndef viskores_cont_ArrayHandleGroupVec_h
#define viskores_cont_ArrayHandleGroupVec_h

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayPortal.h>
#include <viskores/cont/ErrorBadValue.h>

namespace viskores
{
namespace internal
{

template <typename PortalType, viskores::IdComponent N_COMPONENTS>
class VISKORES_ALWAYS_EXPORT ArrayPortalGroupVec
{
  using Writable = viskores::internal::PortalSupportsSets<PortalType>;

public:
  static constexpr viskores::IdComponent NUM_COMPONENTS = N_COMPONENTS;
  using ComponentsPortalType = PortalType;

  using ComponentType = typename std::remove_const<typename ComponentsPortalType::ValueType>::type;
  using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalGroupVec()
    : ComponentsPortal()
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalGroupVec(const ComponentsPortalType& componentsPortal)
    : ComponentsPortal(componentsPortal)
  {
  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentsPortalType>
  VISKORES_EXEC_CONT ArrayPortalGroupVec(
    const ArrayPortalGroupVec<OtherComponentsPortalType, NUM_COMPONENTS>& src)
    : ComponentsPortal(src.GetPortal())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const
  {
    return this->ComponentsPortal.GetNumberOfValues() / NUM_COMPONENTS;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    ValueType result;
    viskores::Id componentsIndex = index * NUM_COMPONENTS;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      result[componentIndex] = this->ComponentsPortal.Get(componentsIndex);
      componentsIndex++;
    }
    return result;
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    viskores::Id componentsIndex = index * NUM_COMPONENTS;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      this->ComponentsPortal.Set(componentsIndex, value[componentIndex]);
      componentsIndex++;
    }
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const ComponentsPortalType& GetPortal() const { return this->ComponentsPortal; }

private:
  ComponentsPortalType ComponentsPortal;
};
}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

template <typename ComponentsStorageTag, viskores::IdComponent NUM_COMPONENTS>
struct VISKORES_ALWAYS_EXPORT StorageTagGroupVec
{
};

namespace internal
{

template <typename ComponentType,
          viskores::IdComponent NUM_COMPONENTS,
          typename ComponentsStorageTag>
class Storage<viskores::Vec<ComponentType, NUM_COMPONENTS>,
              viskores::cont::StorageTagGroupVec<ComponentsStorageTag, NUM_COMPONENTS>>
{
  using ComponentsStorage = viskores::cont::internal::Storage<ComponentType, ComponentsStorageTag>;
  using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;

public:
  using ReadPortalType =
    viskores::internal::ArrayPortalGroupVec<typename ComponentsStorage::ReadPortalType,
                                            NUM_COMPONENTS>;
  using WritePortalType =
    viskores::internal::ArrayPortalGroupVec<typename ComponentsStorage::WritePortalType,
                                            NUM_COMPONENTS>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return ComponentsStorage::CreateBuffers();
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    ComponentsStorage::ResizeBuffers(NUM_COMPONENTS * numValues, buffers, preserve, token);
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ComponentsStorage::GetNumberOfComponentsFlat(buffers) * NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    viskores::Id componentsSize = ComponentsStorage::GetNumberOfValues(buffers);
    return componentsSize / NUM_COMPONENTS;
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const ValueType&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadType("Fill not supported for ArrayHandleGroupVec.");
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    if ((ComponentsStorage::GetNumberOfValues(buffers) % NUM_COMPONENTS) != 0)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                     "ArrayHandleGroupVec's components array does not divide evenly into Vecs.");
    }
    return ReadPortalType(ComponentsStorage::CreateReadPortal(buffers, device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    if ((ComponentsStorage::GetNumberOfValues(buffers) % NUM_COMPONENTS) != 0)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                     "ArrayHandleGroupVec's components array does not divide evenly into Vecs.");
    }
    return WritePortalType(ComponentsStorage::CreateWritePortal(buffers, device, token));
  }
};

} // namespace internal

/// @brief Fancy array handle that groups values into vectors.
///
/// It is sometimes the case that an array is stored such that consecutive
/// entries are meant to form a group. This fancy array handle takes an array
/// of values and a size of groups and then groups the consecutive values
/// stored in a `viskores::Vec`.
///
/// For example, if you have an array handle with the six values 0,1,2,3,4,5
/// and give it to a `ArrayHandleGroupVec` with the number of components set
/// to 3, you get an array that looks like it contains two values of `viskores::Vec`
/// of size 3 with the data [0,1,2], [3,4,5].
///
/// The array of components should have a number of values that divides evenly
/// with the size of the `viskores::Vec`. If the components array does not divide evenly
/// into `viskores::Vec`s, then a warning will be logged and the extra component values
/// will be ignored.
///
template <typename ComponentsArrayHandleType, viskores::IdComponent NUM_COMPONENTS>
class ArrayHandleGroupVec
  : public viskores::cont::ArrayHandle<
      viskores::Vec<typename ComponentsArrayHandleType::ValueType, NUM_COMPONENTS>,
      viskores::cont::StorageTagGroupVec<typename ComponentsArrayHandleType::StorageTag,
                                         NUM_COMPONENTS>>
{
  VISKORES_IS_ARRAY_HANDLE(ComponentsArrayHandleType);

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVec,
    (ArrayHandleGroupVec<ComponentsArrayHandleType, NUM_COMPONENTS>),
    (viskores::cont::ArrayHandle<
      viskores::Vec<typename ComponentsArrayHandleType::ValueType, NUM_COMPONENTS>,
      viskores::cont::StorageTagGroupVec<typename ComponentsArrayHandleType::StorageTag,
                                         NUM_COMPONENTS>>));

  using ComponentType = typename ComponentsArrayHandleType::ValueType;

  /// Construct an `ArrayHandleGroupVec` with a provided components array.
  VISKORES_CONT
  ArrayHandleGroupVec(const ComponentsArrayHandleType& componentsArray)
    : Superclass(componentsArray.GetBuffers())
  {
  }

  /// Retrieve the components array being grouped.
  VISKORES_CONT ComponentsArrayHandleType GetComponentsArray() const
  {
    return ComponentsArrayHandleType(this->GetBuffers());
  }
};

/// `make_ArrayHandleGroupVec` is convenience function to generate an
/// ArrayHandleGroupVec. It takes in an ArrayHandle and the number of components
/// (as a specified template parameter), and returns an array handle with
/// consecutive entries grouped in a Vec.
///
template <viskores::IdComponent NUM_COMPONENTS, typename ArrayHandleType>
VISKORES_CONT viskores::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS>
make_ArrayHandleGroupVec(const ArrayHandleType& array)
{
  return viskores::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS>(array);
}

//--------------------------------------------------------------------------------
// Specialization of ArrayExtractComponent
namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if
// the sub-storage is inefficient (thus making everything inefficient).
template <typename ComponentsStorageTag, viskores::IdComponent NUM_COMPONENTS>
struct ArrayExtractComponentImpl<
  viskores::cont::StorageTagGroupVec<ComponentsStorageTag, NUM_COMPONENTS>>
  : viskores::cont::internal::ArrayExtractComponentImpl<ComponentsStorageTag>
{
  template <typename T>
  viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> operator()(
    const viskores::cont::ArrayHandle<
      viskores::Vec<T, NUM_COMPONENTS>,
      viskores::cont::StorageTagGroupVec<ComponentsStorageTag, NUM_COMPONENTS>>& src,
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy) const
  {
    viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<T, ComponentsStorageTag>,
                                        NUM_COMPONENTS>
      srcArray(src);
    constexpr viskores::IdComponent NUM_SUB_COMPONENTS = viskores::VecFlat<T>::NUM_COMPONENTS;
    viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType> dest =
      ArrayExtractComponentImpl<ComponentsStorageTag>{}(
        srcArray.GetComponentsArray(), componentIndex % NUM_SUB_COMPONENTS, allowCopy);

    // Adjust stride and offset to expectations of grouped values
    return viskores::cont::ArrayHandleStride<typename viskores::VecTraits<T>::BaseComponentType>(
      dest.GetBasicArray(),
      dest.GetNumberOfValues() / NUM_COMPONENTS,
      dest.GetStride() * NUM_COMPONENTS,
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

template <typename AH, viskores::IdComponent NUM_COMPS>
struct SerializableTypeString<viskores::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name =
      "AH_GroupVec<" + SerializableTypeString<AH>::Get() + "," + std::to_string(NUM_COMPS) + ">";
    return name;
  }
};

template <typename T, viskores::IdComponent NUM_COMPS, typename ST>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::Vec<T, NUM_COMPS>,
                              viskores::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : SerializableTypeString<
      viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH, viskores::IdComponent NUM_COMPS>
struct Serialization<viskores::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
private:
  using Type = viskores::cont::ArrayHandleGroupVec<AH, NUM_COMPS>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, Type(obj).GetComponentsArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    viskoresdiy::load(bb, array);

    obj = viskores::cont::make_ArrayHandleGroupVec<NUM_COMPS>(array);
  }
};

template <typename T, viskores::IdComponent NUM_COMPS, typename ST>
struct Serialization<viskores::cont::ArrayHandle<viskores::Vec<T, NUM_COMPS>,
                                                 viskores::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : Serialization<
      viskores::cont::ArrayHandleGroupVec<viskores::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleGroupVec_h
