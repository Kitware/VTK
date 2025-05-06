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
#ifndef viskores_cont_UnknownArrayHandle_h
#define viskores_cont_UnknownArrayHandle_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/ArrayHandleRecombineVec.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/cont/StorageList.h>

#include <viskores/Deprecated.h>
#include <viskores/TypeList.h>
#include <viskores/VecTraits.h>

#include <memory>
#include <typeindex>

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename T, typename S>
void UnknownAHDelete(void* mem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  delete arrayHandle;
}

template <typename T, typename S>
const std::vector<viskores::cont::internal::Buffer>& UnknownAHBuffers(void* mem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  return arrayHandle->GetBuffers();
}

template <typename T, typename S>
void* UnknownAHNewInstance()
{
  return new viskores::cont::ArrayHandle<T, S>;
}

template <typename T, typename S>
viskores::Id UnknownAHNumberOfValues(void* mem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  return arrayHandle->GetNumberOfValues();
}

template <typename T, typename S>
viskores::IdComponent UnknownAHNumberOfComponentsFlat(void* mem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  return arrayHandle->GetNumberOfComponentsFlat();
}

// Uses SFINAE to use Storage<>::GetNumberOfComponents if it exists, or the VecTraits otherwise
template <typename T, typename S>
inline auto UnknownAHNumberOfComponentsImpl(void* mem)
  -> decltype(viskores::cont::internal::Storage<T, S>::GetNumberOfComponents(
    std::vector<viskores::cont::internal::Buffer>()))
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  return viskores::cont::internal::Storage<T, S>::GetNumberOfComponents(arrayHandle->GetBuffers());
}

// Uses static vec size.
template <typename T, typename S>
inline viskores::IdComponent UnknownAHNumberOfComponentsImpl(void*,
                                                             viskores::VecTraitsTagSizeStatic)
{
  return viskores::VecTraits<T>::NUM_COMPONENTS;
}

// The size of the vecs are not defined at compile time. Assume that the components are not
// nested and use the flat components query.
template <typename T, typename S>
inline viskores::IdComponent UnknownAHNumberOfComponentsImpl(void* mem,
                                                             viskores::VecTraitsTagSizeVariable)
{
  return UnknownAHNumberOfComponentsFlat<T, S>(mem);
}

template <typename T, typename S>
viskores::IdComponent UnknownAHNumberOfComponents(void* mem)
{
  return UnknownAHNumberOfComponentsImpl<T, S>(mem,
                                               typename viskores::VecTraits<T>::IsSizeStatic{});
}

template <typename T, typename S>
void UnknownAHAllocate(void* mem,
                       viskores::Id numValues,
                       viskores::CopyFlag preserve,
                       viskores::cont::Token& token)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  arrayHandle->Allocate(numValues, preserve, token);
}

template <typename T, typename S>
void UnknownAHShallowCopy(const void* sourceMem, void* destinationMem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  const AH* source = reinterpret_cast<const AH*>(sourceMem);
  AH* destination = reinterpret_cast<AH*>(destinationMem);
  *destination = *source;
}

template <typename T, typename S>
void UnknownAHDeepCopy(const void* sourceMem, void* destinationMem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  const AH* source = reinterpret_cast<const AH*>(sourceMem);
  AH* destination = reinterpret_cast<AH*>(destinationMem);
  destination->DeepCopyFrom(*source);
}

template <typename T, typename S>
std::vector<viskores::cont::internal::Buffer> UnknownAHExtractComponent(
  void* mem,
  viskores::IdComponent componentIndex,
  viskores::CopyFlag allowCopy)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  auto componentArray =
    viskores::cont::ArrayExtractComponent(*arrayHandle, componentIndex, allowCopy);
  return componentArray.GetBuffers();
}

template <typename T, typename S>
void UnknownAHReleaseResources(void* mem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  arrayHandle->ReleaseResources();
}

template <typename T, typename S>
void UnknownAHReleaseResourcesExecution(void* mem)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  arrayHandle->ReleaseResourcesExecution();
}

template <typename T, typename S>
void UnknownAHPrintSummary(void* mem, std::ostream& out, bool full)
{
  using AH = viskores::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  viskores::cont::printSummary_ArrayHandle(*arrayHandle, out, full);
}

struct VISKORES_CONT_EXPORT UnknownAHContainer;

struct MakeUnknownAHContainerFunctor
{
  template <typename T, typename S>
  std::shared_ptr<UnknownAHContainer> operator()(
    const viskores::cont::ArrayHandle<T, S>& array) const;
};

struct VISKORES_CONT_EXPORT UnknownAHComponentInfo
{
  std::type_index Type;
  bool IsIntegral;
  bool IsFloat;
  bool IsSigned;
  std::size_t Size;

  UnknownAHComponentInfo() = delete;

  bool operator==(const UnknownAHComponentInfo& rhs);

  template <typename T>
  static UnknownAHComponentInfo Make()
  {
    return UnknownAHComponentInfo{ typeid(T),
                                   std::is_integral<T>::value,
                                   std::is_floating_point<T>::value,
                                   std::is_signed<T>::value,
                                   sizeof(T) };
  }

private:
  UnknownAHComponentInfo(std::type_index&& type,
                         bool isIntegral,
                         bool isFloat,
                         bool isSigned,
                         std::size_t size)
    : Type(std::move(type))
    , IsIntegral(isIntegral)
    , IsFloat(isFloat)
    , IsSigned(isSigned)
    , Size(size)
  {
  }
};

struct VISKORES_CONT_EXPORT UnknownAHContainer
{
  void* ArrayHandlePointer;

  std::type_index ValueType;
  std::type_index StorageType;
  UnknownAHComponentInfo BaseComponentType;

  using DeleteType = void(void*);
  DeleteType* DeleteFunction;

  using BuffersType = const std::vector<viskores::cont::internal::Buffer>&(void*);
  BuffersType* Buffers;

  using NewInstanceType = void*();
  NewInstanceType* NewInstance;

  using NewInstanceBasicType = std::shared_ptr<UnknownAHContainer>(void*);
  NewInstanceBasicType* NewInstanceBasic;
  NewInstanceBasicType* NewInstanceFloatBasic;

  using NumberOfValuesType = viskores::Id(void*);
  NumberOfValuesType* NumberOfValues;

  using NumberOfComponentsType = viskores::IdComponent(void*);
  NumberOfComponentsType* NumberOfComponents;
  NumberOfComponentsType* NumberOfComponentsFlat;

  using AllocateType = void(void*, viskores::Id, viskores::CopyFlag, viskores::cont::Token&);
  AllocateType* Allocate;

  using ShallowCopyType = void(const void*, void*);
  ShallowCopyType* ShallowCopy;

  using DeepCopyType = void(const void*, void*);
  DeepCopyType* DeepCopy;

  using ExtractComponentType = std::vector<viskores::cont::internal::Buffer>(void*,
                                                                             viskores::IdComponent,
                                                                             viskores::CopyFlag);
  ExtractComponentType* ExtractComponent;

  using ReleaseResourcesType = void(void*);
  ReleaseResourcesType* ReleaseResources;
  ReleaseResourcesType* ReleaseResourcesExecution;

  using PrintSummaryType = void(void*, std::ostream&, bool);
  PrintSummaryType* PrintSummary;

  void operator=(const UnknownAHContainer&) = delete;

  ~UnknownAHContainer() { this->DeleteFunction(this->ArrayHandlePointer); }

  std::shared_ptr<UnknownAHContainer> MakeNewInstance() const;

  template <typename T, typename S>
  static std::shared_ptr<UnknownAHContainer> Make(const viskores::cont::ArrayHandle<T, S>& array)
  {
    return std::shared_ptr<UnknownAHContainer>(new UnknownAHContainer(array));
  }

  template <typename TargetT, typename SourceT, typename SourceS>
  static std::shared_ptr<UnknownAHContainer> Make(
    const viskores::cont::ArrayHandle<TargetT, viskores::cont::StorageTagCast<SourceT, SourceS>>&
      array)
  {
    viskores::cont::ArrayHandleCast<TargetT, viskores::cont::ArrayHandle<SourceT, SourceS>>
      castArray = array;
    return Make(castArray.GetSourceArray());
  }

  template <typename T, typename... Ss>
  static std::shared_ptr<UnknownAHContainer> Make(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<Ss...>>& array)
  {
    auto&& variant =
      viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandle<T, Ss>...>(array)
        .GetArrayHandleVariant();
    if (variant.IsValid())
    {
      return variant.CastAndCall(MakeUnknownAHContainerFunctor{});
    }
    else
    {
      return std::shared_ptr<UnknownAHContainer>{};
    }
  }

private:
  UnknownAHContainer(const UnknownAHContainer&) = default;

  template <typename T, typename S>
  explicit UnknownAHContainer(const viskores::cont::ArrayHandle<T, S>& array);
};

template <typename T, typename S>
std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceBasic(void*,
                                                              viskores::VecTraitsTagSizeStatic)
{
  return UnknownAHContainer::Make(viskores::cont::ArrayHandleBasic<T>{});
}
template <typename T, typename S>
std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceBasic(void* mem,
                                                              viskores::VecTraitsTagSizeVariable)
{
  viskores::IdComponent numComponents = UnknownAHNumberOfComponentsFlat<T, S>(mem);
  if (numComponents < 1)
  {
    // Array can have an inconsistent number of components. Cannot be represented by basic array.
    throw viskores::cont::ErrorBadType("Cannot create a basic array from array with ValueType of " +
                                       viskores::cont::TypeToString<T>());
  }
  using ComponentType = typename viskores::VecTraits<T>::BaseComponentType;
  return UnknownAHContainer::Make(
    viskores::cont::ArrayHandleRuntimeVec<ComponentType>(numComponents));
}
template <typename T, typename S>
std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceBasic(void* mem)
{
  return UnknownAHNewInstanceBasic<T, S>(mem, typename viskores::VecTraits<T>::IsSizeStatic{});
}

template <typename T, typename S>
std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceFloatBasic(void*,
                                                                   viskores::VecTraitsTagSizeStatic)
{
  using FloatT =
    typename viskores::VecTraits<T>::template ReplaceBaseComponentType<viskores::FloatDefault>;
  return UnknownAHContainer::Make(viskores::cont::ArrayHandleBasic<FloatT>{});
}
template <typename T, typename S>
std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceFloatBasic(
  void* mem,
  viskores::VecTraitsTagSizeVariable)
{
  viskores::IdComponent numComponents = UnknownAHNumberOfComponentsFlat<T, S>(mem);
  if (numComponents < 1)
  {
    // Array can have an inconsistent number of components. Cannot be represented by basic array.
    throw viskores::cont::ErrorBadType("Cannot create a basic array from array with ValueType of " +
                                       viskores::cont::TypeToString<T>());
  }
  return UnknownAHContainer::Make(
    viskores::cont::ArrayHandleRuntimeVec<viskores::FloatDefault>(numComponents));
}
template <typename T, typename S>
std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceFloatBasic(void* mem)
{
  return UnknownAHNewInstanceFloatBasic<T, S>(mem, typename viskores::VecTraits<T>::IsSizeStatic{});
}

template <typename T, typename S>
inline UnknownAHContainer::UnknownAHContainer(const viskores::cont::ArrayHandle<T, S>& array)
  : ArrayHandlePointer(new viskores::cont::ArrayHandle<T, S>(array))
  , ValueType(typeid(T))
  , StorageType(typeid(S))
  , BaseComponentType(
      UnknownAHComponentInfo::Make<typename viskores::VecTraits<T>::BaseComponentType>())
  , DeleteFunction(detail::UnknownAHDelete<T, S>)
  , Buffers(detail::UnknownAHBuffers<T, S>)
  , NewInstance(detail::UnknownAHNewInstance<T, S>)
  , NewInstanceBasic(detail::UnknownAHNewInstanceBasic<T, S>)
  , NewInstanceFloatBasic(detail::UnknownAHNewInstanceFloatBasic<T, S>)
  , NumberOfValues(detail::UnknownAHNumberOfValues<T, S>)
  , NumberOfComponents(detail::UnknownAHNumberOfComponents<T, S>)
  , NumberOfComponentsFlat(detail::UnknownAHNumberOfComponentsFlat<T, S>)
  , Allocate(detail::UnknownAHAllocate<T, S>)
  , ShallowCopy(detail::UnknownAHShallowCopy<T, S>)
  , DeepCopy(detail::UnknownAHDeepCopy<T, S>)
  , ExtractComponent(detail::UnknownAHExtractComponent<T, S>)
  , ReleaseResources(detail::UnknownAHReleaseResources<T, S>)
  , ReleaseResourcesExecution(detail::UnknownAHReleaseResourcesExecution<T, S>)
  , PrintSummary(detail::UnknownAHPrintSummary<T, S>)
{
}

template <typename T, typename S>
inline std::shared_ptr<UnknownAHContainer> MakeUnknownAHContainerFunctor::operator()(
  const viskores::cont::ArrayHandle<T, S>& array) const
{
  return UnknownAHContainer::Make(array);
};

} // namespace detail

// Forward declaration. Include UncertainArrayHandle.h if using this.
template <typename ValueTypeList, typename StorageTypeList>
class UncertainArrayHandle;

/// @brief An ArrayHandle of an unknown value type and storage.
///
/// `UnknownArrayHandle` holds an `ArrayHandle` object using runtime polymorphism
/// to manage different value and storage types rather than compile-time templates.
/// This adds a programming convenience that helps avoid a proliferation of
/// templates. It also provides the management necessary to interface Viskores with
/// data sources where types will not be known until runtime and is the storage
/// mechanism for classes like `DataSet` and `Field` that can hold numerous
/// types.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in Viskores, `UnknownArrayHandle` contains a method named `CastAndCallForTypes()`
/// that determines the correct type from some known list of value types and
/// storage. This mechanism is used internally by Viskores's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// If the `UnknownArrayHandle` is used in a context where the possible array
/// types can be whittled down to a finite list (or you have to), you can
/// specify lists of value types and storage using the `ResetTypesAndStorage()`
/// method. This will convert this object to an `UncertainArrayHandle` of the
/// given types. In cases where a finite set of types need to specified but
/// there is no known subset, `VISKORES_DEFAULT_TYPE_LIST` and
/// `VISKORES_DEFAULT_STORAGE_LIST` can be used.
///
/// `ArrayHandleCast` and `ArrayHandleMultiplexer` are treated special. If
/// the `UnknownArrayHandle` is set to an `ArrayHandle` of one of these
/// types, it will actually store the `ArrayHandle` contained. Likewise,
/// if the `ArrayHandle` is retrieved as one of these types, it will
/// automatically convert it if possible.
///
class VISKORES_CONT_EXPORT UnknownArrayHandle
{
  std::shared_ptr<detail::UnknownAHContainer> Container;

  VISKORES_CONT bool IsValueTypeImpl(std::type_index type) const;
  VISKORES_CONT bool IsStorageTypeImpl(std::type_index type) const;
  VISKORES_CONT bool IsBaseComponentTypeImpl(const detail::UnknownAHComponentInfo& type) const;

public:
  VISKORES_CONT UnknownArrayHandle() = default;

  template <typename T, typename S>
  VISKORES_CONT UnknownArrayHandle(const viskores::cont::ArrayHandle<T, S>& array)
    : Container(detail::UnknownAHContainer::Make(array))
  {
  }

  /// @brief Returns whether an array is stored in this `UnknownArrayHandle`.
  ///
  /// If the `UnknownArrayHandle` is constructed without an `ArrayHandle`, it
  /// will not have an underlying type, and therefore the operations will be
  /// invalid. It is still possible to set this `UnknownArrayHandle` to an
  /// `ArrayHandle`.
  VISKORES_CONT bool IsValid() const;

  /// @brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new `UnknownArrayHandle` for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VISKORES_CONT UnknownArrayHandle NewInstance() const;

  /// @brief Create a new `ArrayHandleBasic` with the same `ValueType` as this array.
  ///
  /// This method creates a new `ArrayHandleBasic` that has the same `ValueType` as the
  /// array held by this one and returns a new `UnknownArrayHandle` for it. This method
  /// is convenient when creating output arrays that should have the same types of values
  /// of the input, but the input might not be a writable array.
  ///
  VISKORES_CONT UnknownArrayHandle NewInstanceBasic() const;

  /// @brief Create a new `ArrayHandleBasic` with the base component of `viskores::FloatDefault`
  ///
  /// This method creates a new `ArrayHandleBasic` that has a `ValueType` that is similar
  /// to the array held by this one except that the base component type is replaced with
  /// `viskores::FloatDefault`. For example, if the contained array has `viskores::Int32` value types,
  /// the returned array will have `viskores::FloatDefault` value types. If the contained array
  /// has `viskores::Id3` value types, the returned array will have `viskores::Vec3f` value types.
  /// If the contained array already has `viskores::FloatDefault` as the base component (e.g.
  /// `viskores::FloatDefault`, `viskores::Vec3f`, `viskores::Vec<viskores::Vec2f, 3>`), then the value type
  /// will be preserved.
  ///
  /// The created array is returned in a new `UnknownArrayHandle`.
  ///
  /// This method is used to convert an array of an unknown type to an array of an almost
  /// known type.
  ///
  VISKORES_CONT UnknownArrayHandle NewInstanceFloatBasic() const;

  /// @brief Returns the name of the value type stored in the array.
  ///
  /// Returns an empty string if no array is stored.
  VISKORES_CONT std::string GetValueTypeName() const;

  /// @brief Returns the name of the base component of the value type stored in the array.
  ///
  /// Returns an empty string if no array is stored.
  VISKORES_CONT std::string GetBaseComponentTypeName() const;

  /// @brief Returns the name of the storage tag for the array.
  ///
  /// Returns an empty string if no array is stored.
  VISKORES_CONT std::string GetStorageTypeName() const;

  /// @brief Returns a string representation of the underlying data type.
  ///
  /// The returned string will be of the form `viskores::cont::ArrayHandle<T, S>` rather than the name
  /// of an actual subclass. If no array is stored, an empty string is returned.
  ///
  VISKORES_CONT std::string GetArrayTypeName() const;

  /// Returns true if this array matches the ValueType template argument.
  ///
  template <typename ValueType>
  VISKORES_CONT bool IsValueType() const
  {
    return this->IsValueTypeImpl(typeid(ValueType));
  }

  /// Returns true if this array matches the StorageType template argument.
  ///
  template <typename StorageType>
  VISKORES_CONT bool IsStorageType() const
  {
    return this->IsStorageTypeImpl(typeid(StorageType));
  }

  /// @brief Returns true if this array's `ValueType` has the provided base component type.
  ///
  /// The base component type is the recursive component type of any `Vec`-like object. So
  /// if the array's `ValueType` is `viskores::Vec<viskores::Float32, 3>`, then the base component
  /// type will be `viskores::Float32`. Likewise, if the `ValueType` is
  /// `viskores::Vec<viskores::Vec<viskores::Float32, 3>, 2>`, then the base component type is still
  /// `viskores::Float32`.
  ///
  /// If the `ValueType` is not `Vec`-like type, then the base component type is the same.
  /// So a `ValueType` of `viskores::Float32` has a base component type of `viskores::Float32`.
  ///
  template <typename BaseComponentType>
  VISKORES_CONT bool IsBaseComponentType() const
  {
    return this->IsBaseComponentTypeImpl(detail::UnknownAHComponentInfo::Make<BaseComponentType>());
  }

  /// @brief Returns true if this array matches the ArrayHandleType template argument.
  ///
  /// Note that `UnknownArrayHandle` has some special handling for `ArrayHandleCast` and
  /// `ArrayHandleMultiplexer`. If you stored an array of one of these types into an
  /// `UnknownArrayHandle`, the type of the underlying array will change and `IsType()`
  /// will fail. However, you can still get the array back out as that type using
  /// `AsArrayHandle`.
  ///
  /// Use the `CanConvert()` method instead to determine if the `UnknownArrayHandle`
  /// contains an array that "matches" the array of a given type. Under most
  /// circumstances, you should prefer `CanConvert()` over `IsType()`.
  ///
  template <typename ArrayHandleType>
  VISKORES_CONT bool IsType() const
  {
    VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);
    return (this->IsValueType<typename ArrayHandleType::ValueType>() &&
            this->IsStorageType<typename ArrayHandleType::StorageTag>());
  }

  /// @brief Assigns potential value and storage types.
  ///
  /// Calling this method will return an `UncertainArrayHandle` with the provided
  /// value and storage type lists. The returned object will hold the same
  /// `ArrayHandle`, but `CastAndCall`s on the returned object will be constrained
  /// to the given types.
  ///
  // Defined in UncertainArrayHandle.h
  template <typename NewValueTypeList, typename NewStorageTypeList>
  VISKORES_CONT viskores::cont::UncertainArrayHandle<NewValueTypeList, NewStorageTypeList>
    ResetTypes(NewValueTypeList = NewValueTypeList{},
               NewStorageTypeList = NewStorageTypeList{}) const;

  /// @brief Returns the number of values in the array.
  ///
  VISKORES_CONT viskores::Id GetNumberOfValues() const;

  /// @brief Returns the number of components for each value in the array.
  ///
  /// If the array holds `viskores::Vec` objects, this will return the number of components
  /// in each value. If the array holds a basic C type (such as `float`), this will return 1.
  /// If the array holds `Vec`-like objects that have the number of components that can vary
  /// at runtime, this method will return 0 (because there is no consistent answer).
  ///
  VISKORES_CONT viskores::IdComponent GetNumberOfComponents() const;

  /// @brief Returns the total number of components for each value in the array.
  ///
  /// If the array holds `viskores::Vec` objects, this will return the total number of components
  /// in each value assuming the object is flattened out to one level of `Vec` objects.
  /// If the array holds a basic C type (such as `float`), this will return 1.
  /// If the array holds a simple `Vec` (such as `viskores::Vec3f`), this will return the number
  /// of components (in this case 3).
  /// If the array holds a hierarchy of `Vec`s (such as `viskores::Vec<viskores::Vec3f, 2>`), this will
  /// return the total number of vecs (in this case 6).
  ///
  /// If this object is holding an array where the number of components can be selected at
  /// runtime (for example, `viskores::cont::ArrayHandleRuntimeVec`), this method will still return
  /// the correct number of components. However, if each value in the array can be a `Vec` of
  /// a different size (such as `viskores::cont::ArrayHandleGroupVecVariable`),
  /// this method will return 0 (because there is no consistent answer).
  ///
  VISKORES_CONT viskores::IdComponent GetNumberOfComponentsFlat() const;

  /// @brief Reallocate the data in the array.
  ///
  /// The allocation works the same as the `Allocate()` method of `viskores::cont::ArrayHandle`.
  VISKORES_CONT void Allocate(viskores::Id numValues,
                              viskores::CopyFlag preserve,
                              viskores::cont::Token& token) const;
  /// @copydoc Allocate
  VISKORES_CONT void Allocate(viskores::Id numValues,
                              viskores::CopyFlag preserve = viskores::CopyFlag::Off) const;

  /// @brief Determine if the contained array can be passed to the given array type.
  ///
  /// This method will return true if calling `AsArrayHandle()` of the given type will
  /// succeed. The result is similar to `IsType()`, and if `IsType()` returns true, then
  /// this will return true. However, this method will also return true for other
  /// types such as an `ArrayHandleMultiplexer` that can contain the array.
  ///
  template <typename ArrayHandleType>
  VISKORES_CONT bool CanConvert() const;

  // MSVC will issue deprecation warnings here if this template is instantiated with
  // a deprecated class even if the template is used from a section of code where
  // deprecation warnings are suppressed. This is annoying behavior since this template
  // has no control over what class it is used with. To get around it, we have to
  // suppress all deprecation warnings here.
#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_BEGIN
#endif

private:
  template <typename T, typename S>
  VISKORES_CONT void BaseAsArrayHandle(viskores::cont::ArrayHandle<T, S>& array) const
  {
    using ArrayType = viskores::cont::ArrayHandle<T, S>;
    if (!this->IsType<ArrayType>())
    {
      VISKORES_LOG_CAST_FAIL(*this, decltype(array));
      throwFailedDynamicCast(this->GetArrayTypeName(), viskores::cont::TypeToString(array));
    }

    array = *reinterpret_cast<ArrayType*>(this->Container->ArrayHandlePointer);
  }

public:
  /// Returns this array cast appropriately and stored in the given `ArrayHandle` type.
  /// Throws a `viskores::cont::ErrorBadType` if the stored array cannot be stored in the given
  /// array type. Use the `CanConvert()` method to determine if the array can be returned
  /// with the given type.
  template <typename T, typename S>
  VISKORES_CONT void AsArrayHandle(viskores::cont::ArrayHandle<T, S>& array) const
  {
    this->BaseAsArrayHandle(array);
  }
  /// @copydoc AsArrayHandle
  template <typename T>
  VISKORES_CONT void AsArrayHandle(viskores::cont::ArrayHandle<T>& array) const;
  /// @copydoc AsArrayHandle
  template <typename T, typename... Ss>
  VISKORES_CONT void AsArrayHandle(
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<Ss...>>& array) const;
  /// @copydoc AsArrayHandle
  template <typename TargetT, typename SourceT, typename SourceS>
  VISKORES_CONT void AsArrayHandle(
    viskores::cont::ArrayHandle<TargetT, viskores::cont::StorageTagCast<SourceT, SourceS>>& array)
    const
  {
    using ContainedArrayType = viskores::cont::ArrayHandle<SourceT, SourceS>;
    array = viskores::cont::ArrayHandleCast<TargetT, ContainedArrayType>(
      this->AsArrayHandle<ContainedArrayType>());
  }
  /// @copydoc AsArrayHandle
  template <typename T>
  VISKORES_CONT void AsArrayHandle(
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagRuntimeVec>& array) const
  {
    using BaseT = typename T::ComponentType;
    if (this->IsStorageType<viskores::cont::StorageTagBasic>() &&
        this->IsBaseComponentType<BaseT>())
    {
      // Reinterpret the basic array as components, and then wrap that in a runtime vec
      // with the correct amount of components.
      viskores::cont::ArrayHandle<BaseT, viskores::cont::StorageTagBasic> basicArray(
        this->Container->Buffers(this->Container->ArrayHandlePointer));
      array =
        viskores::cont::ArrayHandleRuntimeVec<BaseT>(this->GetNumberOfComponentsFlat(), basicArray);
    }
    else
    {
      this->BaseAsArrayHandle(array);
    }
  }
  /// @copydoc AsArrayHandle
  template <typename ArrayType>
  VISKORES_CONT ArrayType AsArrayHandle() const
  {
    VISKORES_IS_ARRAY_HANDLE(ArrayType);
    ArrayType array;
    this->AsArrayHandle(array);
    return array;
  }

#ifdef VISKORES_MSVC
  VISKORES_DEPRECATED_SUPPRESS_END
#endif

  /// @brief Deep copies data from another `UnknownArrayHandle`.
  ///
  /// This method takes an `UnknownArrayHandle` and deep copies data from it.
  ///
  /// If this object does not point to an existing `ArrayHandle`, a new `ArrayHandleBasic`
  /// with the same value type of the `source` is created.
  ///
  void DeepCopyFrom(const viskores::cont::UnknownArrayHandle& source);

  /// @brief Deep copies data from another `UnknownArrayHandle`.
  ///
  /// This method takes an `UnknownArrayHandle` and deep copies data from it.
  ///
  /// If this object does not point to an existing `ArrayHandle`, this const version
  /// of `DeepCopyFrom()` throws an exception.
  ///
  void DeepCopyFrom(const viskores::cont::UnknownArrayHandle& source) const;

  /// @brief Attempts a shallow copy of an array or a deep copy if that is not possible.
  ///
  /// This method takes an `UnknownArrayHandle` and attempts to perform a shallow copy.
  /// This shallow copy occurs if this object points to an `ArrayHandle` of the same type
  /// or does not point to any `ArrayHandle` at all. If this is not possible, then
  /// the array is deep copied.
  ///
  /// This method is roughly equivalent to the `viskores::cont::ArrayCopyShallowIfPossible()` function
  /// (defined in `viskores/cont/ArrayCopy.h`). This form allows you to copy into a type defined
  /// elsewhere (and hidden in the `UnknownArrayHandle`) whereas `ArrayCopyShallowIfPossible()`
  /// must be copied into an `ArrayHandle` of a known type.
  ///
  void CopyShallowIfPossible(const viskores::cont::UnknownArrayHandle& source);

  /// @brief Attempts a shallow copy of an array or a deep copy if that is not possible.
  ///
  /// This method takes an `UnknownArrayHandle` and attempts to perform a shallow copy.
  /// This shallow copy occurs if this object points to an `ArrayHandle` of the same type.
  /// If the types are incompatible, then the array is deep copied.
  ///
  /// If this object does not point to an existing `ArrayHandle`, this const version
  /// of `CopyShallowIfPossible()` throws an exception.
  ///
  /// This method is roughly equivalent to the `viskores::cont::ArrayCopyShallowIfPossible()` function
  /// (defined in `viskores/cont/ArrayCopy.h`). This form allows you to copy into a type defined
  /// elsewhere (and hidden in the `UnknownArrayHandle`) whereas `ArrayCopyShallowIfPossible()`
  /// must be copied into an `ArrayHandle` of a known type.
  ///
  void CopyShallowIfPossible(const viskores::cont::UnknownArrayHandle& source) const;

  /// @brief Extract a component of the array.
  ///
  /// This method returns an array that holds the data for a given flat component of the data.
  /// The `BaseComponentType` has to be specified and must match the contained array (i.e.
  /// the result of `IsBaseComponentType()` must succeed for the given type).
  ///
  /// This method treats each value in the array as a flat `viskores::Vec` even if it is a
  /// `viskores::Vec` of `Vec`s. For example, if the array actually holds values of type
  /// `viskores::Vec<viskores::Vec<T, 3>, 2>`, it is treated as if it holds a `Vec<T, 6>`. See
  /// `viskores::VecFlat` for details on how vectors are flattened.
  ///
  /// The point of using `ExtractComponent()` over `AsArrayHandle()` is that it drastically reduces
  /// the amount of types you have to try. Most of the time the base component type is one of
  /// the basic C types (i.e. `int`, `long`, `float`, etc.). You do not need to know what shape
  /// the containing `viskores::Vec` is in, nor do you need to know the actual storage of the array.
  ///
  /// Note that the type of the array returned is `ArrayHandleStride`. Using this type of
  /// array handle has a slight overhead over basic arrays like `ArrayHandleBasic` and
  /// `ArrayHandleSOA`.
  ///
  /// When extracting a component of an array, a shallow pointer to the data is returned
  /// whenever possible. However, in some circumstances it is impossible to conform the
  /// array. In these cases, the data are by default copied. If copying the data would
  /// cause problems (for example, you are writing into the array), you can select the
  /// optional `allowCopy` flag to `viskores::CopyFlag::Off`. In this case, an exception
  /// will be thrown if the result cannot be represented by a shallow copy.
  ///
  template <typename BaseComponentType>
  VISKORES_CONT viskores::cont::ArrayHandleStride<BaseComponentType> ExtractComponent(
    viskores::IdComponent componentIndex,
    viskores::CopyFlag allowCopy = viskores::CopyFlag::On) const
  {
    using ComponentArrayType = viskores::cont::ArrayHandleStride<BaseComponentType>;
    if (!this->IsBaseComponentType<BaseComponentType>())
    {
      VISKORES_LOG_CAST_FAIL(*this, ComponentArrayType);
      throwFailedDynamicCast("UnknownArrayHandle with " + this->GetArrayTypeName(),
                             "component array of " +
                               viskores::cont::TypeToString<BaseComponentType>());
    }

    auto buffers = this->Container->ExtractComponent(
      this->Container->ArrayHandlePointer, componentIndex, allowCopy);
    return ComponentArrayType(buffers);
  }

  /// @brief Extract the array knowing only the component type of the array.
  ///
  /// This method returns an `ArrayHandle` that points to the data in the array. This method
  /// differs from `AsArrayHandle()` because you do not need to know the exact `ValueType` and
  /// `StorageTag` of the array. Instead, you only need to know the base component type.
  ///
  /// `ExtractArrayFromComponents()` works by calling the `ExtractComponent()` method and then
  /// combining them together in a fancy `ArrayHandle`. This allows you to ignore the storage
  /// type of the underlying array as well as any `Vec` structure of the value type. However,
  /// it also places some limitations on how the data can be pulled from the data.
  ///
  /// First, you have to specify the base component type. This must match the data in the
  /// underlying array (as reported by `IsBaseComponentType()`).
  ///
  /// Second, the array returned will have the `Vec`s flattened. For example, if the underlying
  /// array has a `ValueType` of `viskores::Vec<viskores::Vec<T, 3>, 3>`, then this method will treat
  /// the data as if it was `viskores::Vec<T, 9>`. There is no way to get an array with `viskores::Vec`
  /// of `viskores::Vec` values.
  ///
  /// Third, because the `Vec` length of the values in the returned `ArrayHandle` must be
  /// determined at runtime, that can break many assumptions of using `viskores::Vec` objects. The
  /// type is not going to be a `viskores::Vec<T,N>` type but rather an internal class that is intended
  /// to behave like that. The type should behave mostly like a `viskores::Vec`, but will have some
  /// differences that can lead to unexpected behavior. For example, this `Vec`-like object
  /// will not have a `NUM_COMPONENTS` constant static expression because it is not known
  /// at compile time. (Use the `GetNumberOfComponents()` method instead.) And for the same
  /// reason you will not be able to pass these objects to classes overloaded or templated
  /// on the `Vec` type. Also, these `Vec`-like objects cannot be created as new instances.
  /// Thus, you will likely have to iterate over all components rather than do operations on
  /// the whole `Vec`.
  ///
  /// Fourth, because `ExtractArrayFromComponents()` uses `ExtractComponent()` to pull data from
  /// the array (which in turn uses `ArrayExtractComponent()`), there are some `ArrayHandle` types
  /// that will require copying data to a new array. This could be problematic in cases where
  /// you want to write to the array. To prevent data from being copied, set the optional
  ///  `allowCopy` to `viskores::CopyFlag::Off`. This will cause an exception to be thrown if
  /// the resulting array cannot reference the memory held in this `UnknownArrayHandle`.
  ///
  /// Fifth, component arrays are extracted using `ArrayHandleStride` as the representation
  /// for each component. This array adds a slight overhead for each lookup as it performs the
  /// arithmetic for finding the index of each component.
  ///
  template <typename BaseComponentType>
  VISKORES_CONT viskores::cont::ArrayHandleRecombineVec<BaseComponentType>
  ExtractArrayFromComponents(viskores::CopyFlag allowCopy = viskores::CopyFlag::On) const
  {
    viskores::cont::ArrayHandleRecombineVec<BaseComponentType> result;
    viskores::IdComponent numComponents = this->GetNumberOfComponentsFlat();
    for (viskores::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      result.AppendComponentArray(this->ExtractComponent<BaseComponentType>(cIndex, allowCopy));
    }
    return result;
  }

  /// @brief Call a functor using the underlying array type.
  ///
  /// `CastAndCallForTypes()` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array. You must specify
  /// the `TypeList` and `StorageList` as template arguments.
  ///
  /// After the functor argument you may add any number of arguments that will be
  /// passed to the functor after the converted `ArrayHandle`.
  ///
  template <typename TypeList, typename StorageList, typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallForTypes(Functor&& functor, Args&&... args) const;

  /// @brief Call a functor using the underlying array type with a float cast fallback.
  ///
  /// `CastAndCallForTypesWithFloatFallback()` attempts to cast the held array to a specific
  /// value type, and then calls the given functor with the cast array. You must specify
  /// the `TypeList` and `StorageList` as template arguments.
  ///
  /// After the functor argument you may add any number of arguments that will be
  /// passed to the functor after the converted `ArrayHandle`.
  ///
  /// If the underlying array does not match any of the requested array types, the
  /// array is copied to a new `ArrayHandleBasic` with `viskores::FloatDefault` components
  /// in its value and attempts to cast to those types.
  ///
  template <typename TypeList, typename StorageList, typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallForTypesWithFloatFallback(Functor&& functor, Args&&... args) const;

  /// @brief Call a functor on an array extracted from the components.
  ///
  /// `CastAndCallWithExtractedArray()` behaves similarly to `CastAndCallForTypes()`.
  /// It converts the contained data to an `ArrayHandle` and calls a functor with
  /// that `ArrayHandle` (and any number of optionally specified arguments).
  ///
  /// The advantage of `CastAndCallWithExtractedArray()` is that you do not need to
  /// specify any `TypeList` or `StorageList`. Instead, it internally uses
  /// `ExtractArrayFromComponents()` to work with most `ArrayHandle` types with only
  /// about 10 instances of the functor. In contrast, calling `CastAndCallForTypes()`
  /// with, for example, `VISKORES_DEFAULT_TYPE_LIST` and `VISKORES_DEFAULT_STORAGE_LIST`
  /// results in many more instances of the functor but handling many fewer types
  /// of `ArrayHandle`.
  ///
  /// There are, however, costs to using this method. Details of these costs are
  /// documented for the `ExtractArrayFromComponents()` method, but briefly they
  /// are that `Vec` types get flattened, the resulting array has a strange `Vec`-like
  /// value type that has many limitations on its use, there is an overhead for
  /// retrieving each value from the array, and there is a potential that data
  /// must be copied.
  ///
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallWithExtractedArray(Functor&& functor, Args&&... args) const;

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VISKORES_CONT void ReleaseResourcesExecution() const;

  /// Releases all resources in both the control and execution environments.
  ///
  VISKORES_CONT void ReleaseResources() const;

  /// Prints a summary of the array's type, size, and contents.
  VISKORES_CONT void PrintSummary(std::ostream& out, bool full = false) const;
};

//=============================================================================
// Out of class implementations

namespace detail
{

template <typename T, typename S>
struct UnknownArrayHandleCanConvert
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle& array) const
  {
    return array.IsType<viskores::cont::ArrayHandle<T, S>>();
  }
};

template <typename T>
struct UnknownArrayHandleCanConvert<T, viskores::cont::StorageTagBasic>
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle& array) const
  {
    using UnrolledVec = viskores::internal::UnrollVec<T>;
    return (array.IsType<viskores::cont::ArrayHandleBasic<T>>() ||
            (array.IsStorageType<viskores::cont::StorageTagRuntimeVec>() &&
             array.IsBaseComponentType<typename UnrolledVec::ComponentType>() &&
             UnrolledVec::NUM_COMPONENTS == array.GetNumberOfComponentsFlat()));
  }
};

template <typename TargetT, typename SourceT, typename SourceS>
struct UnknownArrayHandleCanConvert<TargetT, viskores::cont::StorageTagCast<SourceT, SourceS>>
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle& array) const
  {
    return UnknownArrayHandleCanConvert<SourceT, SourceS>{}(array);
  }
};

template <typename T>
struct UnknownArrayHandleCanConvertTry
{
  template <typename S>
  VISKORES_CONT void operator()(S,
                                const viskores::cont::UnknownArrayHandle& array,
                                bool& canConvert) const
  {
    canConvert |= UnknownArrayHandleCanConvert<T, S>{}(array);
  }
};

template <typename T, typename... Ss>
struct UnknownArrayHandleCanConvert<T, viskores::cont::StorageTagMultiplexer<Ss...>>
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle& array) const
  {
    bool canConvert = false;
    viskores::ListForEach(
      UnknownArrayHandleCanConvertTry<T>{}, viskores::List<Ss...>{}, array, canConvert);
    return canConvert;
  }
};

template <typename T>
struct UnknownArrayHandleCanConvert<T, viskores::cont::StorageTagRuntimeVec>
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle& array) const
  {
    using BaseComponentType = typename T::ComponentType;
    return (array.IsType<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagRuntimeVec>>() ||
            (array.IsStorageType<viskores::cont::StorageTagBasic>() &&
             array.IsBaseComponentType<BaseComponentType>()));
  }
};

} // namespace detail

template <typename ArrayHandleType>
VISKORES_CONT inline bool UnknownArrayHandle::CanConvert() const
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

  return detail::UnknownArrayHandleCanConvert<typename ArrayHandleType::ValueType,
                                              typename ArrayHandleType::StorageTag>{}(*this);
}

namespace detail
{

template <typename T,
          viskores::IdComponent = viskores::VecTraits<
            typename viskores::internal::UnrollVec<T>::ComponentType>::NUM_COMPONENTS>
struct UnknownArrayHandleRuntimeVecAsBasic
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle*,
                                const detail::UnknownAHContainer*,
                                viskores::cont::ArrayHandle<T>&) const
  {
    // This version only gets called if T contains a `Vec`-like object that is not a strict `Vec`.
    // This is rare but could happen. In this case, the type cannot be stored in an
    // `ArrayHandleRuntimeVec` and therefore the load can never happen, so just ignore.
    return false;
  }
};

template <typename T>
struct UnknownArrayHandleRuntimeVecAsBasic<T, 1>
{
  VISKORES_CONT bool operator()(const viskores::cont::UnknownArrayHandle* self,
                                const detail::UnknownAHContainer* container,
                                viskores::cont::ArrayHandle<T>& array) const
  {
    using UnrolledVec = viskores::internal::UnrollVec<T>;
    using ComponentType = typename UnrolledVec::ComponentType;
    if (self->IsStorageType<viskores::cont::StorageTagRuntimeVec>() &&
        self->IsBaseComponentType<ComponentType>() &&
        UnrolledVec::NUM_COMPONENTS == self->GetNumberOfComponentsFlat())
    {
      // Pull out the components array out of the buffers. The array might not match exactly
      // the array put in, but the buffer should still be consistent with the array (which works
      // because the size of a basic array is based on the number of bytes in the buffer).
      using RuntimeVecType =
        typename viskores::cont::ArrayHandleRuntimeVec<ComponentType>::ValueType;
      using StorageRuntimeVec =
        viskores::cont::internal::Storage<RuntimeVecType, viskores::cont::StorageTagRuntimeVec>;
      StorageRuntimeVec::AsArrayHandleBasic(container->Buffers(container->ArrayHandlePointer),
                                            array);
      return true;
    }
    else
    {
      return false;
    }
  }
};

} // namespace detail

template <typename T>
VISKORES_CONT inline void UnknownArrayHandle::AsArrayHandle(
  viskores::cont::ArrayHandle<T>& array) const
{
  if (!detail::UnknownArrayHandleRuntimeVecAsBasic<T>{}(this, this->Container.get(), array))
  {
    this->BaseAsArrayHandle(array);
  }
}

namespace detail
{

struct UnknownArrayHandleMultiplexerCastTry
{
  template <typename T, typename S, typename... Ss>
  VISKORES_CONT void operator()(
    S,
    const viskores::cont::UnknownArrayHandle& unknownArray,
    viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<Ss...>>& outputArray,
    bool& converted) const
  {
    using ArrayType = viskores::cont::ArrayHandle<T, S>;
    if (unknownArray.CanConvert<ArrayType>())
    {
      if (converted && !unknownArray.IsType<ArrayType>())
      {
        // The array has already been converted and pushed in the multiplexer. It is
        // possible that multiple array types can be put in the ArrayHandleMultiplexer
        // (for example, and ArrayHandle or an ArrayHandle that has been cast). Exact
        // matches will override other matches (hence, the second part of the condition),
        // but at this point we have already found a better array to put inside.
        return;
      }
      outputArray = viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandle<T, Ss>...>(
        unknownArray.AsArrayHandle<ArrayType>());
      converted = true;
    }
  }
};

} // namespace detail

template <typename T, typename... Ss>
void UnknownArrayHandle::AsArrayHandle(
  viskores::cont::ArrayHandle<T, viskores::cont::StorageTagMultiplexer<Ss...>>& array) const
{
  bool converted = false;
  viskores::ListForEach(detail::UnknownArrayHandleMultiplexerCastTry{},
                        viskores::List<Ss...>{},
                        *this,
                        array,
                        converted);

  if (!converted)
  {
    VISKORES_LOG_CAST_FAIL(*this, decltype(array));
    throwFailedDynamicCast(viskores::cont::TypeToString(*this),
                           viskores::cont::TypeToString(array));
  }
}

namespace detail
{

struct UnknownArrayHandleTry
{
  template <typename T, typename S, typename Functor, typename... Args>
  void operator()(viskores::List<T, S>,
                  Functor&& f,
                  bool& called,
                  const viskores::cont::UnknownArrayHandle& unknownArray,
                  Args&&... args) const
  {
    using DerivedArrayType = viskores::cont::ArrayHandle<T, S>;
    if (!called && unknownArray.CanConvert<DerivedArrayType>())
    {
      called = true;
      DerivedArrayType derivedArray;
      unknownArray.AsArrayHandle(derivedArray);
      VISKORES_LOG_CAST_SUCC(unknownArray, derivedArray);

      // If you get a compile error here, it means that you have called CastAndCall for a
      // viskores::cont::UnknownArrayHandle and the arguments of the functor do not match those
      // being passed. This is often because it is calling the functor with an ArrayHandle
      // type that was not expected. Either add overloads to the functor to accept all
      // possible array types or constrain the types tried for the CastAndCall. Note that
      // the functor will be called with an array of type viskores::cont::ArrayHandle<T, S>.
      // Directly using a subclass of ArrayHandle (e.g. viskores::cont::ArrayHandleConstant<T>)
      // might not work.
      f(derivedArray, std::forward<Args>(args)...);
    }
  }
};

} // namespace detail

namespace internal
{

namespace detail
{

template <typename T>
struct IsUndefinedArrayType
{
};
template <typename T, typename S>
struct IsUndefinedArrayType<viskores::List<T, S>>
  : viskores::cont::internal::IsInvalidArrayHandle<T, S>
{
};

} // namespace detail

template <typename ValueTypeList, typename StorageTypeList>
using ListAllArrayTypes =
  viskores::ListRemoveIf<viskores::ListCross<ValueTypeList, StorageTypeList>,
                         detail::IsUndefinedArrayType>;

VISKORES_CONT_EXPORT void ThrowCastAndCallException(const viskores::cont::UnknownArrayHandle&,
                                                    const std::type_info&);

} // namespace internal

template <typename TypeList, typename StorageTagList, typename Functor, typename... Args>
inline void UnknownArrayHandle::CastAndCallForTypes(Functor&& f, Args&&... args) const
{
  using crossProduct = internal::ListAllArrayTypes<TypeList, StorageTagList>;

  bool called = false;
  viskores::ListForEach(detail::UnknownArrayHandleTry{},
                        crossProduct{},
                        std::forward<Functor>(f),
                        called,
                        *this,
                        std::forward<Args>(args)...);
  if (!called)
  {
    // throw an exception
    VISKORES_LOG_CAST_FAIL(*this, TypeList);
    internal::ThrowCastAndCallException(*this, typeid(TypeList));
  }
}

template <typename TypeList, typename StorageTagList, typename Functor, typename... Args>
VISKORES_CONT void UnknownArrayHandle::CastAndCallForTypesWithFloatFallback(Functor&& functor,
                                                                            Args&&... args) const
{
  using crossProduct = internal::ListAllArrayTypes<TypeList, StorageTagList>;

  bool called = false;
  viskores::ListForEach(detail::UnknownArrayHandleTry{},
                        crossProduct{},
                        std::forward<Functor>(functor),
                        called,
                        *this,
                        std::forward<Args>(args)...);
  if (!called)
  {
    // Copy to a float array and try again
    VISKORES_LOG_F(viskores::cont::LogLevel::Info,
                   "Cast and call from %s failed. Copying to basic float array.",
                   this->GetArrayTypeName().c_str());
    viskores::cont::UnknownArrayHandle floatArray = this->NewInstanceFloatBasic();
    floatArray.DeepCopyFrom(*this);
    viskores::ListForEach(detail::UnknownArrayHandleTry{},
                          crossProduct{},
                          std::forward<Functor>(functor),
                          called,
                          floatArray,
                          std::forward<Args>(args)...);
  }
  if (!called)
  {
    // throw an exception
    VISKORES_LOG_CAST_FAIL(*this, TypeList);
    internal::ThrowCastAndCallException(*this, typeid(TypeList));
  }
}

//=============================================================================
// Free function casting helpers

/// Returns true if \c variant matches the type of ArrayHandleType.
///
template <typename ArrayHandleType>
VISKORES_CONT inline bool IsType(const viskores::cont::UnknownArrayHandle& array)
{
  return array.template IsType<ArrayHandleType>();
}

/// Returns \c variant cast to the given \c ArrayHandle type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename ArrayHandleType>
VISKORES_CONT inline ArrayHandleType Cast(const viskores::cont::UnknownArrayHandle& array)
{
  return array.template AsArrayHandle<ArrayHandleType>();
}

namespace detail
{

struct UnknownArrayHandleTryExtract
{
  template <typename T, typename Functor, typename... Args>
  void operator()(T,
                  Functor&& f,
                  bool& called,
                  const viskores::cont::UnknownArrayHandle& unknownArray,
                  Args&&... args) const
  {
    if (!called && unknownArray.IsBaseComponentType<T>())
    {
      called = true;
      auto extractedArray = unknownArray.ExtractArrayFromComponents<T>();
      VISKORES_LOG_CAST_SUCC(unknownArray, extractedArray);

      // If you get a compile error here, it means that you have called
      // CastAndCallWithExtractedArray for a viskores::cont::UnknownArrayHandle and the arguments of
      // the functor do not match those being passed. This is often because it is calling the
      // functor with an ArrayHandle type that was not expected. Add overloads to the functor to
      // accept all possible array types or constrain the types tried for the CastAndCall. Note
      // that the functor will be called with an array of type that is different than the actual
      // type of the `ArrayHandle` stored in the `UnknownArrayHandle`.
      f(extractedArray, std::forward<Args>(args)...);
    }
  }
};

} // namespace detail

template <typename Functor, typename... Args>
inline void UnknownArrayHandle::CastAndCallWithExtractedArray(Functor&& functor,
                                                              Args&&... args) const
{
  bool called = false;
  viskores::ListForEach(detail::UnknownArrayHandleTryExtract{},
                        viskores::TypeListScalarAll{},
                        std::forward<Functor>(functor),
                        called,
                        *this,
                        std::forward<Args>(args)...);
  if (!called)
  {
    // Throw an exception.
    // The message will be a little wonky because the types are just the value types, not the
    // full type to cast to.
    VISKORES_LOG_CAST_FAIL(*this, viskores::TypeListScalarAll);
    internal::ThrowCastAndCallException(*this, typeid(viskores::TypeListScalarAll));
  }
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

template <>
struct VISKORES_CONT_EXPORT SerializableTypeString<viskores::cont::UnknownArrayHandle>
{
  static VISKORES_CONT std::string Get();
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <>
struct VISKORES_CONT_EXPORT Serialization<viskores::cont::UnknownArrayHandle>
{
public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::cont::UnknownArrayHandle& obj);
  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::cont::UnknownArrayHandle& obj);
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

#endif //viskores_cont_UnknownArrayHandle_h
