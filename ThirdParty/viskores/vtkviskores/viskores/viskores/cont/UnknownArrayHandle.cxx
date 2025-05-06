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

#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleReverse.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorInternal.h>
#include <viskores/cont/UncertainArrayHandle.h>

#include <viskores/cont/internal/ArrayCopyUnknown.h>

#include <cstring>
#include <sstream>

namespace
{

template <viskores::IdComponent N, typename ScalarList>
struct AllVecImpl;
template <viskores::IdComponent N, typename... Scalars>
struct AllVecImpl<N, viskores::List<Scalars...>>
{
  using type = viskores::List<viskores::Vec<Scalars, N>...>;
};

// Normally I would implement this with viskores::ListTransform, but that is causing an ICE in GCC 4.8.
// This implementation is not much different.
template <viskores::IdComponent N>
using AllVec = typename AllVecImpl<N, viskores::TypeListBaseC>::type;

template <typename T>
using IsBasicStorage = std::is_same<viskores::cont::StorageTagBasic, T>;
template <typename List>
using RemoveBasicStorage = viskores::ListRemoveIf<List, IsBasicStorage>;

using UnknownSerializationTypes =
  viskores::ListAppend<viskores::TypeListBaseC, AllVec<2>, AllVec<3>, AllVec<4>>;
using UnknownSerializationSpecializedStorage = viskores::ListAppend<
  RemoveBasicStorage<VISKORES_DEFAULT_STORAGE_LIST>,
  viskores::List<viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                                            viskores::cont::StorageTagBasic,
                                                            viskores::cont::StorageTagBasic>,
                 viskores::cont::StorageTagConstant,
                 viskores::cont::StorageTagCounting,
                 viskores::cont::StorageTagIndex,
                 viskores::cont::StorageTagGroupVec<viskores::cont::StorageTagBasic, 2>,
                 viskores::cont::StorageTagGroupVec<viskores::cont::StorageTagBasic, 3>,
                 viskores::cont::StorageTagGroupVec<viskores::cont::StorageTagBasic, 4>,
                 viskores::cont::StorageTagPermutation<viskores::cont::StorageTagBasic,
                                                       viskores::cont::StorageTagBasic>,
                 viskores::cont::StorageTagReverse<viskores::cont::StorageTagBasic>,
                 viskores::cont::StorageTagSOA,
                 viskores::cont::StorageTagUniformPoints>>;

} // anonymous namespace

namespace viskores
{
namespace cont
{

namespace detail
{

std::shared_ptr<UnknownAHContainer> UnknownAHContainer::MakeNewInstance() const
{
  // Start by doing an invalid copy to create a new container, then swap out the pointer
  // to the array handle to make sure that each object will delete its own ArrayHandle
  // when they get destroyed.
  std::shared_ptr<UnknownAHContainer> newContainer(new UnknownAHContainer(*this));
  newContainer->ArrayHandlePointer = this->NewInstance();
  return newContainer;
}

bool UnknownAHComponentInfo::operator==(const UnknownAHComponentInfo& rhs)
{
  if (this->IsIntegral || this->IsFloat)
  {
    return ((this->IsIntegral == rhs.IsIntegral) && (this->IsFloat == rhs.IsFloat) &&
            (this->IsSigned == rhs.IsSigned) && (this->Size == rhs.Size));
  }
  else
  {
    // Needs optimization based on platform. OSX cannot compare typeid across translation units?
    bool typesEqual = (this->Type == rhs.Type);

    // Could use optimization based on platform. OSX cannot compare typeid across translation
    // units, so we have to also check the names. (Why doesn't the == operator above do that?)
    // Are there other platforms that behave similarly?
    if (!typesEqual)
    {
      typesEqual = (std::strcmp(this->Type.name(), rhs.Type.name()) == 0);
    }

    return typesEqual;
  }
}

} // namespace detail

VISKORES_CONT bool UnknownArrayHandle::IsValueTypeImpl(std::type_index type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Needs optimization based on platform. OSX cannot compare typeid across translation units?
  bool typesEqual = (this->Container->ValueType == type);

  // Could use optimization based on platform. OSX cannot compare typeid across translation
  // units, so we have to also check the names. (Why doesn't the == operator above do that?)
  // Are there other platforms that behave similarly?
  if (!typesEqual)
  {
    typesEqual = (std::strcmp(this->Container->ValueType.name(), type.name()) == 0);
  }

  return typesEqual;
}

VISKORES_CONT bool UnknownArrayHandle::IsStorageTypeImpl(std::type_index type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Needs optimization based on platform. OSX cannot compare typeid across translation units?
  bool typesEqual = (this->Container->StorageType == type);

  // Could use optimization based on platform. OSX cannot compare typeid across translation
  // units, so we have to also check the names. (Why doesn't the == operator above do that?)
  // Are there other platforms that behave similarly?
  if (!typesEqual)
  {
    typesEqual = (std::strcmp(this->Container->StorageType.name(), type.name()) == 0);
  }

  return typesEqual;
}

VISKORES_CONT bool UnknownArrayHandle::IsBaseComponentTypeImpl(
  const detail::UnknownAHComponentInfo& type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Note that detail::UnknownAHComponentInfo has a custom operator==
  return this->Container->BaseComponentType == type;
}

VISKORES_CONT bool UnknownArrayHandle::IsValid() const
{
  return static_cast<bool>(this->Container);
}

VISKORES_CONT UnknownArrayHandle UnknownArrayHandle::NewInstance() const
{
  if (this->IsStorageType<viskores::cont::StorageTagRuntimeVec>())
  {
    // Special case for `ArrayHandleRuntimeVec`, which (1) can be used in place of
    // a basic array in `UnknownArrayHandle` and (2) needs a special construction to
    // capture the correct number of components. Also note that we are allowing this
    // special case to be implemented in `NewInstanceBasic` because it has a better
    // fallback (throw an exception rather than create a potentially incompatible
    // with the wrong number of components).
    return this->NewInstanceBasic();
  }
  UnknownArrayHandle newArray;
  if (this->Container)
  {
    newArray.Container = this->Container->MakeNewInstance();
  }
  return newArray;
}

VISKORES_CONT UnknownArrayHandle UnknownArrayHandle::NewInstanceBasic() const
{
  UnknownArrayHandle newArray;
  if (this->IsStorageType<viskores::cont::StorageTagRuntimeVec>())
  {
    // Special case for `ArrayHandleRuntimeVec`, which (1) can be used in place of
    // a basic array in `UnknownArrayHandle` and (2) needs a special construction to
    // capture the correct number of components.
    auto runtimeVecArrayCreator = [&](auto exampleComponent)
    {
      using ComponentType = decltype(exampleComponent);
      if (!newArray.IsValid() && this->IsBaseComponentType<ComponentType>())
      {
        newArray = viskores::cont::make_ArrayHandleRuntimeVec<ComponentType>(
          this->GetNumberOfComponentsFlat());
      }
    };
    viskores::ListForEach(runtimeVecArrayCreator, viskores::TypeListBaseC{});
    if (newArray.IsValid())
    {
      return newArray;
    }
  }
  if (this->Container)
  {
    newArray.Container = this->Container->NewInstanceBasic(this->Container->ArrayHandlePointer);
  }
  return newArray;
}

VISKORES_CONT UnknownArrayHandle UnknownArrayHandle::NewInstanceFloatBasic() const
{
  if (this->IsStorageType<viskores::cont::StorageTagRuntimeVec>())
  {
    // Special case for `ArrayHandleRuntimeVec`, which (1) can be used in place of
    // a basic array in `UnknownArrayHandle` and (2) needs a special construction to
    // capture the correct number of components.
    return viskores::cont::make_ArrayHandleRuntimeVec<viskores::FloatDefault>(
      this->GetNumberOfComponentsFlat());
  }
  UnknownArrayHandle newArray;
  if (this->Container)
  {
    newArray.Container =
      this->Container->NewInstanceFloatBasic(this->Container->ArrayHandlePointer);
  }
  return newArray;
}

VISKORES_CONT std::string UnknownArrayHandle::GetValueTypeName() const
{
  if (this->Container)
  {
    return viskores::cont::TypeToString(this->Container->ValueType);
  }
  else
  {
    return "";
  }
}

VISKORES_CONT std::string UnknownArrayHandle::GetBaseComponentTypeName() const
{
  if (this->Container)
  {
    return viskores::cont::TypeToString(this->Container->BaseComponentType);
  }
  else
  {
    return "";
  }
}

VISKORES_CONT std::string UnknownArrayHandle::GetStorageTypeName() const
{
  if (this->Container)
  {
    return viskores::cont::TypeToString(this->Container->StorageType);
  }
  else
  {
    return "";
  }
}

VISKORES_CONT std::string UnknownArrayHandle::GetArrayTypeName() const
{
  if (this->Container)
  {
    return "viskores::cont::ArrayHandle<" + this->GetValueTypeName() + ", " +
      this->GetStorageTypeName() + ">";
  }
  else
  {
    return "";
  }
}

VISKORES_CONT viskores::Id UnknownArrayHandle::GetNumberOfValues() const
{
  if (this->Container)
  {
    return this->Container->NumberOfValues(this->Container->ArrayHandlePointer);
  }
  else
  {
    return 0;
  }
}

viskores::IdComponent UnknownArrayHandle::GetNumberOfComponents() const
{
  if (this->Container)
  {
    return this->Container->NumberOfComponents(this->Container->ArrayHandlePointer);
  }
  else
  {
    return 0;
  }
}

VISKORES_CONT viskores::IdComponent UnknownArrayHandle::GetNumberOfComponentsFlat() const
{
  if (this->Container)
  {
    return this->Container->NumberOfComponentsFlat(this->Container->ArrayHandlePointer);
  }
  else
  {
    return 0;
  }
}

VISKORES_CONT void UnknownArrayHandle::Allocate(viskores::Id numValues,
                                                viskores::CopyFlag preserve,
                                                viskores::cont::Token& token) const
{
  if (this->Container)
  {
    this->Container->Allocate(this->Container->ArrayHandlePointer, numValues, preserve, token);
  }
  else
  {
    throw viskores::cont::ErrorBadAllocation(
      "Cannot allocate UnknownArrayHandle that does not contain an array.");
  }
}

VISKORES_CONT void UnknownArrayHandle::Allocate(viskores::Id numValues,
                                                viskores::CopyFlag preserve) const
{
  viskores::cont::Token token;
  this->Allocate(numValues, preserve, token);
}

VISKORES_CONT void UnknownArrayHandle::DeepCopyFrom(
  const viskores::cont::UnknownArrayHandle& source)
{
  if (!this->IsValid())
  {
    *this = source.NewInstance();
  }

  const_cast<const UnknownArrayHandle*>(this)->DeepCopyFrom(source);
}

VISKORES_CONT void UnknownArrayHandle::DeepCopyFrom(
  const viskores::cont::UnknownArrayHandle& source) const
{
  if (!this->IsValid())
  {
    throw viskores::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  if (source.IsValueTypeImpl(this->Container->ValueType) &&
      source.IsStorageTypeImpl(this->Container->StorageType))
  {
    this->Container->DeepCopy(source.Container->ArrayHandlePointer,
                              this->Container->ArrayHandlePointer);
  }
  else
  {
    viskores::cont::internal::ArrayCopyUnknown(source, *this);
  }
}

VISKORES_CONT
void UnknownArrayHandle::CopyShallowIfPossible(const viskores::cont::UnknownArrayHandle& source)
{
  if (!this->IsValid())
  {
    *this = source;
  }
  else
  {
    const_cast<const UnknownArrayHandle*>(this)->CopyShallowIfPossible(source);
  }
}

VISKORES_CONT
void UnknownArrayHandle::CopyShallowIfPossible(
  const viskores::cont::UnknownArrayHandle& source) const
{
  if (!this->IsValid())
  {
    throw viskores::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  if (source.IsValueTypeImpl(this->Container->ValueType) &&
      source.IsStorageTypeImpl(this->Container->StorageType))
  {
    this->Container->ShallowCopy(source.Container->ArrayHandlePointer,
                                 this->Container->ArrayHandlePointer);
  }
  else
  {
    viskores::cont::internal::ArrayCopyUnknown(source, *this);
  }
}

VISKORES_CONT void UnknownArrayHandle::ReleaseResourcesExecution() const
{
  if (this->Container)
  {
    this->Container->ReleaseResourcesExecution(this->Container->ArrayHandlePointer);
  }
}

VISKORES_CONT void UnknownArrayHandle::ReleaseResources() const
{
  if (this->Container)
  {
    this->Container->ReleaseResources(this->Container->ArrayHandlePointer);
  }
}

VISKORES_CONT void UnknownArrayHandle::PrintSummary(std::ostream& out, bool full) const
{
  if (this->Container)
  {
    this->Container->PrintSummary(this->Container->ArrayHandlePointer, out, full);
  }
  else
  {
    out << "null UnknownArrayHandle" << std::endl;
  }
}

namespace internal
{

VISKORES_CONT_EXPORT void ThrowCastAndCallException(const viskores::cont::UnknownArrayHandle& ref,
                                                    const std::type_info& type)
{
  std::ostringstream out;
  out << "Could not find appropriate cast for array in CastAndCall.\n"
         "Array: ";
  ref.PrintSummary(out);
  out << "TypeList: " << viskores::cont::TypeToString(type) << "\n";
  throw viskores::cont::ErrorBadType(out.str());
}

} // namespace internal
}
} // namespace viskores::cont


//=============================================================================
// Specializations of serialization related classes

namespace viskores
{
namespace cont
{

std::string SerializableTypeString<viskores::cont::UnknownArrayHandle>::Get()
{
  return "UnknownAH";
}
}
} // namespace viskores::cont

namespace
{

enum struct SerializedArrayType : viskores::UInt8
{
  BasicArray = 0,
  SpecializedStorage
};

struct SaveBasicArray
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType,
                                mangled_diy_namespace::BinaryBuffer& bb,
                                const viskores::cont::UnknownArrayHandle& obj,
                                bool& saved)
  {
    // Basic arrays and arrays with compatible layouts can be loaed/saved as an
    // ArrayHandleRuntimeVec. Thus, we can load/save them all with one routine.
    using ArrayType = viskores::cont::ArrayHandleRuntimeVec<ComponentType>;
    if (!saved && obj.CanConvert<ArrayType>())
    {
      ArrayType array = obj.AsArrayHandle<ArrayType>();
      viskoresdiy::save(bb, SerializedArrayType::BasicArray);
      viskoresdiy::save(bb, viskores::cont::TypeToString<ComponentType>());
      viskoresdiy::save(bb, array);
      saved = true;
    }
  }
};

struct LoadBasicArray
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType,
                                mangled_diy_namespace::BinaryBuffer& bb,
                                viskores::cont::UnknownArrayHandle& obj,
                                const std::string& componentTypeString,
                                bool& loaded)
  {
    if (!loaded && (componentTypeString == viskores::cont::TypeToString<ComponentType>()))
    {
      viskores::cont::ArrayHandleRuntimeVec<ComponentType> array;
      viskoresdiy::load(bb, array);
      obj = array;
      loaded = true;
    }
  }
};

VISKORES_CONT void SaveSpecializedArray(mangled_diy_namespace::BinaryBuffer& bb,
                                        const viskores::cont::UnknownArrayHandle& obj)
{
  viskores::IdComponent numComponents = obj.GetNumberOfComponents();
  switch (numComponents)
  {
    case 1:
      viskoresdiy::save(bb, SerializedArrayType::SpecializedStorage);
      viskoresdiy::save(bb, numComponents);
      viskoresdiy::save(
        bb, obj.ResetTypes<viskores::TypeListBaseC, UnknownSerializationSpecializedStorage>());
      break;
    case 2:
      viskoresdiy::save(bb, SerializedArrayType::SpecializedStorage);
      viskoresdiy::save(bb, numComponents);
      viskoresdiy::save(bb, obj.ResetTypes<AllVec<2>, UnknownSerializationSpecializedStorage>());
      break;
    case 3:
      viskoresdiy::save(bb, SerializedArrayType::SpecializedStorage);
      viskoresdiy::save(bb, numComponents);
      viskoresdiy::save(bb, obj.ResetTypes<AllVec<3>, UnknownSerializationSpecializedStorage>());
      break;
    case 4:
      viskoresdiy::save(bb, SerializedArrayType::SpecializedStorage);
      viskoresdiy::save(bb, numComponents);
      viskoresdiy::save(bb, obj.ResetTypes<AllVec<4>, UnknownSerializationSpecializedStorage>());
      break;
    default:
      throw viskores::cont::ErrorBadType(
        "Vectors of size " + std::to_string(numComponents) +
        " are not supported for serialization from UnknownArrayHandle. "
        "Try narrowing down possible types with UncertainArrayHandle.");
  }
}

VISKORES_CONT void LoadSpecializedArray(mangled_diy_namespace::BinaryBuffer& bb,
                                        viskores::cont::UnknownArrayHandle& obj)
{
  viskores::IdComponent numComponents;
  viskoresdiy::load(bb, numComponents);

  viskores::cont::UncertainArrayHandle<viskores::TypeListBaseC,
                                       UnknownSerializationSpecializedStorage>
    array1;
  viskores::cont::UncertainArrayHandle<AllVec<2>, UnknownSerializationSpecializedStorage> array2;
  viskores::cont::UncertainArrayHandle<AllVec<3>, UnknownSerializationSpecializedStorage> array3;
  viskores::cont::UncertainArrayHandle<AllVec<4>, UnknownSerializationSpecializedStorage> array4;

  switch (numComponents)
  {
    case 1:
      viskoresdiy::load(bb, array1);
      obj = array1;
      break;
    case 2:
      viskoresdiy::load(bb, array2);
      obj = array2;
      break;
    case 3:
      viskoresdiy::load(bb, array3);
      obj = array3;
      break;
    case 4:
      viskoresdiy::load(bb, array4);
      obj = array4;
      break;
    default:
      throw viskores::cont::ErrorInternal(
        "Unexpected component size when loading UnknownArrayHandle.");
  }
}

} // anonymous namespace

namespace mangled_diy_namespace
{

void Serialization<viskores::cont::UnknownArrayHandle>::save(
  BinaryBuffer& bb,
  const viskores::cont::UnknownArrayHandle& obj)
{
  bool saved = false;

  // First, try serializing basic arrays (which we can do for any Vec size).
  viskores::ListForEach(SaveBasicArray{}, viskores::TypeListBaseC{}, bb, obj, saved);

  // If that did not work, try one of the specialized arrays.
  if (!saved)
  {
    SaveSpecializedArray(bb, obj);
  }
}

void Serialization<viskores::cont::UnknownArrayHandle>::load(
  BinaryBuffer& bb,
  viskores::cont::UnknownArrayHandle& obj)
{
  SerializedArrayType arrayType;
  viskoresdiy::load(bb, arrayType);

  switch (arrayType)
  {
    case SerializedArrayType::BasicArray:
    {
      std::string componentTypeString;
      viskoresdiy::load(bb, componentTypeString);
      bool loaded = false;
      viskores::ListForEach(
        LoadBasicArray{}, viskores::TypeListBaseC{}, bb, obj, componentTypeString, loaded);
      if (!loaded)
      {
        throw viskores::cont::ErrorInternal(
          "Failed to load basic array. Unexpected buffer values.");
      }
      break;
    }
    case SerializedArrayType::SpecializedStorage:
      LoadSpecializedArray(bb, obj);
      break;
    default:
      throw viskores::cont::ErrorInternal("Got inappropriate enumeration value for loading array.");
  }
}

} // namespace mangled_diy_namespace
