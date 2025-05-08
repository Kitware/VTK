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
#ifndef viskores_cont_ArrayHandleGroupVecVariable_h
#define viskores_cont_ArrayHandleGroupVecVariable_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayPortal.h>
#include <viskores/cont/ErrorBadValue.h>

#include <viskores/Assert.h>
#include <viskores/VecFromPortal.h>

namespace viskores
{
namespace internal
{

template <typename ComponentsPortalType, typename OffsetsPortalType>
class VISKORES_ALWAYS_EXPORT ArrayPortalGroupVecVariable
{
public:
  using ComponentType = typename std::remove_const<typename ComponentsPortalType::ValueType>::type;
  using ValueType = viskores::VecFromPortal<ComponentsPortalType>;

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalGroupVecVariable()
    : ComponentsPortal()
    , OffsetsPortal()
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalGroupVecVariable(const ComponentsPortalType& componentsPortal,
                              const OffsetsPortalType& offsetsPortal)
    : ComponentsPortal(componentsPortal)
    , OffsetsPortal(offsetsPortal)
  {
  }

  /// Copy constructor for any other ArrayPortalGroupVecVariable with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentsPortalType, typename OtherOffsetsPortalType>
  VISKORES_EXEC_CONT ArrayPortalGroupVecVariable(
    const ArrayPortalGroupVecVariable<OtherComponentsPortalType, OtherOffsetsPortalType>& src)
    : ComponentsPortal(src.GetComponentsPortal())
    , OffsetsPortal(src.GetOffsetsPortal())
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->OffsetsPortal.GetNumberOfValues() - 1; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const
  {
    viskores::Id offsetIndex = this->OffsetsPortal.Get(index);
    viskores::Id nextOffsetIndex = this->OffsetsPortal.Get(index + 1);

    return ValueType(this->ComponentsPortal,
                     static_cast<viskores::IdComponent>(nextOffsetIndex - offsetIndex),
                     offsetIndex);
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  void Set(viskores::Id index, const ValueType& value) const
  {
    if ((&value.GetPortal() == &this->ComponentsPortal) &&
        (value.GetOffset() == this->OffsetsPortal.Get(index)))
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

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const ComponentsPortalType& GetComponentsPortal() const { return this->ComponentsPortal; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  const OffsetsPortalType& GetOffsetsPortal() const { return this->OffsetsPortal; }

private:
  ComponentsPortalType ComponentsPortal;
  OffsetsPortalType OffsetsPortal;
};

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

template <typename ComponentsStorageTag, typename OffsetsStorageTag>
struct VISKORES_ALWAYS_EXPORT StorageTagGroupVecVariable
{
};

namespace internal
{

template <typename ComponentsPortal, typename ComponentsStorageTag, typename OffsetsStorageTag>
class Storage<viskores::VecFromPortal<ComponentsPortal>,
              viskores::cont::StorageTagGroupVecVariable<ComponentsStorageTag, OffsetsStorageTag>>
{
  using ComponentType = typename ComponentsPortal::ValueType;
  using ComponentsStorage = viskores::cont::internal::Storage<ComponentType, ComponentsStorageTag>;
  using OffsetsStorage = viskores::cont::internal::Storage<viskores::Id, OffsetsStorageTag>;

  using ComponentsArray = viskores::cont::ArrayHandle<ComponentType, ComponentsStorageTag>;
  using OffsetsArray = viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageTag>;

  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<ComponentsPortal, typename ComponentsStorage::WritePortalType>::value),
    "Used invalid ComponentsPortal type with expected ComponentsStorageTag.");

  struct Info
  {
    std::size_t OffsetsBuffersOffset;
  };

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> ComponentsBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(
      buffers.begin() + 1, buffers.begin() + info.OffsetsBuffersOffset);
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> OffsetsBuffers(
    const std::vector<viskores::cont::internal::Buffer> buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<viskores::cont::internal::Buffer>(
      buffers.begin() + info.OffsetsBuffersOffset, buffers.end());
  }

public:
  VISKORES_STORAGE_NO_RESIZE;

  using ReadPortalType =
    viskores::internal::ArrayPortalGroupVecVariable<typename ComponentsStorage::ReadPortalType,
                                                    typename OffsetsStorage::ReadPortalType>;
  using WritePortalType =
    viskores::internal::ArrayPortalGroupVecVariable<typename ComponentsStorage::WritePortalType,
                                                    typename OffsetsStorage::ReadPortalType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    // Number of components can be variable.
    return 0;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return OffsetsStorage::GetNumberOfValues(OffsetsBuffers(buffers)) - 1;
  }

  VISKORES_CONT static void Fill(const std::vector<viskores::cont::internal::Buffer>&,
                                 const viskores::VecFromPortal<ComponentsPortal>&,
                                 viskores::Id,
                                 viskores::Id,
                                 viskores::cont::Token&)
  {
    throw viskores::cont::ErrorBadType("Fill not supported for ArrayHandleGroupVecVariable.");
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return ReadPortalType(
      ComponentsStorage::CreateReadPortal(ComponentsBuffers(buffers), device, token),
      OffsetsStorage::CreateReadPortal(OffsetsBuffers(buffers), device, token));
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(
      ComponentsStorage::CreateWritePortal(ComponentsBuffers(buffers), device, token),
      OffsetsStorage::CreateReadPortal(OffsetsBuffers(buffers), device, token));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const ComponentsArray& componentsArray = ComponentsArray{},
    const OffsetsArray& offsetsArray = OffsetsArray{})
  {
    Info info;
    info.OffsetsBuffersOffset = 1 + componentsArray.GetBuffers().size();
    return viskores::cont::internal::CreateBuffers(info, componentsArray, offsetsArray);
  }

  VISKORES_CONT static ComponentsArray GetComponentsArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return ComponentsArray(ComponentsBuffers(buffers));
  }

  VISKORES_CONT static OffsetsArray GetOffsetsArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return OffsetsArray(OffsetsBuffers(buffers));
  }
};

} // namespace internal

/// @brief Fancy array handle that groups values into vectors of different sizes.
///
/// It is sometimes the case that you need to run a worklet with an input or
/// output that has a different number of values per instance. For example, the
/// cells of a `viskores::cont::CellCetExplicit` can have different numbers of points in each
/// cell. If inputting or outputting cells of this type, each instance of the
/// worklet might need a `viskores::Vec` of a different length. This fancy array handle
/// takes an array of values and an array of offsets and groups the consecutive
/// values in Vec-like objects. The values are treated as tightly packed, so
/// that each Vec contains the values from one offset to the next. The last
/// value contains values from the last offset to the end of the array.
///
/// For example, if you have an array handle with the 9 values
/// 0,1,2,3,4,5,6,7,8 an offsets array handle with the 4 values 0,4,6,9 and give
/// them to an `ArrayHandleGroupVecVariable`, you get an array that looks like
/// it contains three values of Vec-like objects with the data [0,1,2,3],
/// [4,5], and [6,7,8].
///
/// Note that caution should be used with `ArrayHandleRuntimeVec` because the
/// size of the `viskores::Vec` values is not known at compile time. Thus, the value
/// type of this array is forced to a special `viskores::VecFromPortal` class that can cause
/// surprises if treated as a `viskores::Vec`. In particular, the static `NUM_COMPONENTS`
/// expression does not exist. Furthermore, new variables of type `viskores::VecFromPortal`
/// cannot be created. This means that simple operators like `+` will not work
/// because they require an intermediate object to be created. (Equal operators
/// like `+=` do work because they are given an existing variable to place the
/// output.)
///
/// The offsets array is often derived from a list of sizes for each of the
/// entries. You can use the convenience function
/// `viskores::cont::ConvertNumComponentsToOffsets` to take an array of sizes
/// (i.e. the number of components for each entry) and get an array of offsets
/// needed for `ArrayHandleGroupVecVariable`.
///
template <typename ComponentsArrayHandleType, typename OffsetsArrayHandleType>
class ArrayHandleGroupVecVariable
  : public viskores::cont::ArrayHandle<
      viskores::VecFromPortal<typename ComponentsArrayHandleType::WritePortalType>,
      viskores::cont::StorageTagGroupVecVariable<typename ComponentsArrayHandleType::StorageTag,
                                                 typename OffsetsArrayHandleType::StorageTag>>
{
  VISKORES_IS_ARRAY_HANDLE(ComponentsArrayHandleType);
  VISKORES_IS_ARRAY_HANDLE(OffsetsArrayHandleType);

  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<viskores::Id, typename OffsetsArrayHandleType::ValueType>::value),
    "ArrayHandleGroupVecVariable's offsets array must contain viskores::Id values.");

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVecVariable,
    (ArrayHandleGroupVecVariable<ComponentsArrayHandleType, OffsetsArrayHandleType>),
    (viskores::cont::ArrayHandle<
      viskores::VecFromPortal<typename ComponentsArrayHandleType::WritePortalType>,
      viskores::cont::StorageTagGroupVecVariable<typename ComponentsArrayHandleType::StorageTag,
                                                 typename OffsetsArrayHandleType::StorageTag>>));

  using ComponentType = typename ComponentsArrayHandleType::ValueType;

  /// Construct an `ArrayHandleGroupVecVariable`
  VISKORES_CONT
  ArrayHandleGroupVecVariable(const ComponentsArrayHandleType& componentsArray,
                              const OffsetsArrayHandleType& offsetsArray)
    : Superclass(StorageType::CreateBuffers(componentsArray, offsetsArray))
  {
  }

  /// Return the components array providing the data for the grouped vec array.
  VISKORES_CONT ComponentsArrayHandleType GetComponentsArray() const
  {
    return StorageType::GetComponentsArray(this->GetBuffers());
  }

  /// Return the offsets array defining the locations and sizes of each value.
  VISKORES_CONT OffsetsArrayHandleType GetOffsetsArray() const
  {
    return StorageType::GetOffsetsArray(this->GetBuffers());
  }
};

/// `make_ArrayHandleGroupVecVariable` is convenience function to generate an
/// ArrayHandleGroupVecVariable. It takes in an ArrayHandle of values and an
/// array handle of offsets and returns an array handle with consecutive
/// entries grouped in a Vec.
///
template <typename ComponentsArrayHandleType, typename OffsetsArrayHandleType>
VISKORES_CONT
  viskores::cont::ArrayHandleGroupVecVariable<ComponentsArrayHandleType, OffsetsArrayHandleType>
  make_ArrayHandleGroupVecVariable(const ComponentsArrayHandleType& componentsArray,
                                   const OffsetsArrayHandleType& offsetsArray)
{
  return viskores::cont::ArrayHandleGroupVecVariable<ComponentsArrayHandleType,
                                                     OffsetsArrayHandleType>(componentsArray,
                                                                             offsetsArray);
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

template <typename SAH, typename OAH>
struct SerializableTypeString<viskores::cont::ArrayHandleGroupVecVariable<SAH, OAH>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_GroupVecVariable<" + SerializableTypeString<SAH>::Get() + "," +
      SerializableTypeString<OAH>::Get() + ">";
    return name;
  }
};

template <typename SP, typename SST, typename OST>
struct SerializableTypeString<
  viskores::cont::ArrayHandle<viskores::VecFromPortal<SP>,
                              viskores::cont::StorageTagGroupVecVariable<SST, OST>>>
  : SerializableTypeString<viskores::cont::ArrayHandleGroupVecVariable<
      viskores::cont::ArrayHandle<typename SP::ValueType, SST>,
      viskores::cont::ArrayHandle<viskores::Id, OST>>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename SAH, typename OAH>
struct Serialization<viskores::cont::ArrayHandleGroupVecVariable<SAH, OAH>>
{
private:
  using Type = viskores::cont::ArrayHandleGroupVecVariable<SAH, OAH>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, Type(obj).GetComponentsArray());
    viskoresdiy::save(bb, Type(obj).GetOffsetsArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    SAH src;
    OAH off;

    viskoresdiy::load(bb, src);
    viskoresdiy::load(bb, off);

    obj = viskores::cont::make_ArrayHandleGroupVecVariable(src, off);
  }
};

template <typename SP, typename SST, typename OST>
struct Serialization<
  viskores::cont::ArrayHandle<viskores::VecFromPortal<SP>,
                              viskores::cont::StorageTagGroupVecVariable<SST, OST>>>
  : Serialization<viskores::cont::ArrayHandleGroupVecVariable<
      viskores::cont::ArrayHandle<typename SP::ValueType, SST>,
      viskores::cont::ArrayHandle<viskores::Id, OST>>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleGroupVecVariable_h
