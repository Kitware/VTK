// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkImplicitArray_h
#define vtkImplicitArray_h

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkGenericDataArray.h"
#include "vtkImplicitArrayTraits.h" // for traits

#include <memory>
#include <type_traits>

/**
 * \class vtkImplicitArray
 * \brief A read only array class that wraps an implicit function from integers to any value type
 * supported by VTK
 *
 * This templated array class allows one to mimic the vtkDataArray interface using an implicit map
 * behind the scenes. The `BackendT` type can be a class or a struct that implements a const map
 * method that takes integers to any VTK value type. It can also be any type of Closure/Functor that
 * implements a const operator() method from integers to the value type of the array. If a void
 * mapTuple(vtkIdType, TupleType*) const method is also present, the array will use this method to
 * to populate the tuple instead of the map method. If a
 * ValueType mapComponent(vtkIdType, int) const method is also present, the array will use this
 * method to populate the GetTypedComponent function instead of the map method.
 *
 * The ordering of the array for tuples and components is implicitly AOS.
 *
 * The backend type can be trivially constructible, in which case the array gets initialized with a
 * default constructed instance of the BackendT, or not default constructible, in which case the
 * backend is initially a nullptr and must be set using the `SetBackend` method.
 *
 * Being a "read_only" array, any attempt to set a value in the array will result in a warning
 * message with no change to the backend itself. This may evolve in future versions of the class as
 * the needs of users become clearer.
 *
 * The `GetVoidPointer` method will create an internal vtkAOSDataArrayTemplate and populate it with
 * the values from the implicit array and can thus be very memory intensive. The `Squeeze` method
 * will destroy this internal memory array. Both deep and shallow copies to other types of arrays
 * will populate the other array with the implicit values contained in the implicit one. Deep and
 * shallow copies from other array into this one do not make sense and will result in undefined
 * behavior. Deep and shallow copies from implicit arrays of the same type will act the same way and
 * will transfer a shared backend object pointer. Deep and shallow copies from one type of implicit
 * array to a different type should result in a compile time error.
 *
 * Constraints on the backend type are enforced through `implicit_array_traits` found in the
 * vtkImplicitArrayTraits header file. These traits use metaprogramming to check the proposed
 * backend type at compile time. The decision to use this type of structure was taken for the
 * following reasons:
 * - static dispatch of the calls to the backend (when combined with the CRTP structure of
 * vtkGenericDataArray)
 * - flexibility regarding the complexity/simplicity/nature/type... of the backend one would like to
 * use
 *
 * Example for array that always returns 42:
 * @code
 * struct Const42
 * {
 *   int operator()(vtkIdType idx) const { return 42; }
 * };
 * vtkNew<vtkImplicitArray<Const42>> arr42;
 * @endcode
 *
 * Example for array that implements map and mapTuple
 * @code
 * struct ConstTupleStruct
 * {
 * int Tuple[3] = { 0, 0, 0 };
 * // constructor
 * ConstTupleStruct(int tuple[3])
 * {
 *  this->Tuple[0] = tuple[0];
 *  this->Tuple[1] = tuple[1];
 *  this->Tuple[2] = tuple[2];
 * }
 *
 * // used for GetValue
 * int map(int idx) const
 * {
 *   int tuple[3];
 *   this->mapTuple(idx / 3, tuple);
 *   return tuple[idx % 3];
 * }
 * // used for GetTypedTuple
 * void mapTuple(int vtkNotUsed(idx), int* tuple) const
 * {
 *   tuple[0] = this->tuple[0];
 *   tuple[1] = this->tuple[1];
 *   tuple[2] = this->tuple[2];
 * }
 * };
 * @endcode
 *
 * example for array that implements map and mapComponent
 * @code
 * struct ConstComponentStruct
 * {
 *   int Tuple[3] = { 0, 0, 0 };
 *
 * ConstComponentStruct(int tuple[3])
 * {
 *   this->Tuple[0] = tuple[0];
 *   this->Tuple[1] = tuple[1];
 *   this->Tuple[2] = tuple[2];
 * }
 *
 * // used for GetValue
 * int map(int idx) const { return this->mapComponent(idx / 3, idx % 3); }
 * // used for GetTypedComponent
 * int mapComponent(int vtkNotUsed(idx), int comp) const { return this->Tuple[comp]; }
 * };
 * @endcode
 *
 * A peculiarity of `vtkImplicitArray`s is that their `NewInstance` method no longer gives
 * an instance of the exact same array type. A `NewInstance` call on a `vtkImplicitArray`
 * will return a `vtkAOSDataArrayTemplate<ValueTypeT>` with the same value type as the
 * original implicit array. This is so that the following workflow (used extensively
 * throughout VTK) can work without issues:
 * @code
 * struct Const42
 * {
 *   int operator()(vtkIdType idx) const { return 42; }
 * };
 * vtkNew<vtkImplicitArray<Const42>> arr42;
 * arr42->SetNumberOfTuples(11);
 * vtkSmartPointer<vtkDataArray> arr43 = vtk::TakeSmartPointer(arr42->NewInstance());
 * arr43->SetNumberOfComponents(arr42->GetNumberOfComponents());
 * arr43->SetNumberOfTuples(arr42->GetNumberOfTuples());
 * arr43->Fill(43);
 * @endcode
 *
 * Optionally, `vtkImplicitArray`s backends can return their memory usage in KiB by defining
 * the function `getMemorySize` returning `unsigned long`. `vtkImplicitArray` then exposes this
 * function through the `GetActualMemorySize` function. If the backend does not define it,
 * `GetActualMemorySize` always returns 1.
 *
 * @sa
 * vtkGenericDataArray vtkImplicitArrayTraits vtkDataArray
 */

//-------------------------------------------------------------------------------------------------
// Special macro for implicit array types modifying the behavior of NewInstance to provide writable
// AOS arrays instead of empty implicit arrays
#define vtkImplicitArrayTypeMacro(thisClass, superclass)                                           \
  vtkAbstractTypeMacroWithNewInstanceType(thisClass, superclass,                                   \
    vtkAOSDataArrayTemplate<typename thisClass::ValueType>, typeid(thisClass).name());             \
                                                                                                   \
protected:                                                                                         \
  vtkObjectBase* NewInstanceInternal() const override                                              \
  {                                                                                                \
    return vtkAOSDataArrayTemplate<typename thisClass::ValueType>::New();                          \
  }                                                                                                \
                                                                                                   \
public:
//-------------------------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
template <class BackendT>
class vtkImplicitArray
  : public vtkGenericDataArray<vtkImplicitArray<BackendT>,
      typename vtk::detail::implicit_array_traits<BackendT>::rtype>
{
  using trait = vtk::detail::implicit_array_traits<BackendT>;
  static_assert(trait::can_read,
    "Supplied backend type does not have mandatory read trait. Must implement either map() const "
    "or operator() const.");
  using ValueTypeT = typename trait::rtype;
  using GenericDataArrayType = vtkGenericDataArray<vtkImplicitArray<BackendT>, ValueTypeT>;

public:
  using SelfType = vtkImplicitArray<BackendT>;
  vtkImplicitArrayTypeMacro(SelfType, GenericDataArrayType);
  using ValueType = typename GenericDataArrayType::ValueType;
  using BackendType = BackendT;

  static vtkImplicitArray* New();

  ///@{
  /**
   * Implementation of vtkGDAConceptMethods
   */
  /**
   * Get the value at @a idx. @a idx assumes AOS ordering.
   */
  ValueType GetValue(vtkIdType idx) const { return this->GetValueImpl<BackendT>(idx); }

  /**
   * Will not do anything for these read only arrays!
   */
  void SetValue(vtkIdType idx, ValueType value);

  /**
   * Copy the tuple at @a idx into @a tuple.
   */
  void GetTypedTuple(vtkIdType idx, ValueType* tuple) const
  {
    this->GetTypedTupleImpl<BackendT>(idx, tuple);
  }

  /**
   * Will not do anything for these read only arrays!
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple);

  /**
   * Get component @a comp of the tuple at @a idx.
   */
  ValueType GetTypedComponent(vtkIdType idx, int comp) const
  {
    return this->GetTypedComponentImpl<BackendT>(idx, comp);
  }

  /**
   * Will not do anything for these read only arrays!
   */
  void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value);
  ///@}

  ///@{
  /**
   * Setter/Getter for Backend
   */
  void SetBackend(std::shared_ptr<BackendT> newBackend)
  {
    this->Backend = newBackend;
    this->Modified();
  }
  std::shared_ptr<BackendT> GetBackend() { return this->Backend; }
  ///@}

  /**
   * Utility method for setting backend parameterization directly
   */
  template <typename... Params>
  void ConstructBackend(Params&&... params)
  {
    this->SetBackend(std::make_shared<BackendT>(std::forward<Params>(params)...));
  }

  /**
   * Use of this method is discouraged, it creates a memory copy of the data into
   * a contiguous AoS-ordered buffer internally.
   *
   * Implicit array aims to limit memory consumption. Calling this method breaks
   * this paradigm and can cause unexpected memory consumption,
   * specially when called indirectly by some implementation details.
   * E.g. when using the numpy wrapping, see #19304.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;

  /**
   * Release all extraneous internal memory including the void pointer used by `GetVoidPointer`
   */
  void Squeeze() override;

  /**
   * Get the type of array this is when down casting
   */
  int GetArrayType() const override { return vtkAbstractArray::ImplicitArray; }

  /**
   * Reset the array to default construction
   */
  void Initialize() override
  {
    this->Initialize<BackendT>();
    this->Squeeze();
  }

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this implicit data array.
   *
   * The value returned is guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object.
   *
   * Implicit array backends can implement the `getMemorySize` function to override the default
   * implementation, which always returns 1.
   */
  unsigned long GetActualMemorySize() const override
  {
    return this->GetActualMemorySizeImpl<BackendT>();
  }

  /**
   * Specific DeepCopy for implicit arrays
   *
   * This method should be preferred for two implicit arrays having the same backend. We cannot call
   * the method `DeepCopy` since that conflicts with the virtual function of the same name that
   * cannot be templated. The non-interopability of templates and virtual functions is a language
   * limitation at the time of writing this documentation.  We can call this from the dispatched
   * version of the `DeepCopy` in `vtkDataArray`. However, the implicit array needs to be
   * dispatchable in order to to not enter into the Generic implementation of the deep copy. This
   * dispatch is not always the case for all implicit arrays.
   */
  template <typename OtherBackend>
  void ImplicitDeepCopy(vtkImplicitArray<OtherBackend>* other)
  {
    static_assert(std::is_same<BackendT, OtherBackend>::value,
      "Cannot copy implicit array with one type of backend to an implicit array with a different "
      "type of backend");
    this->SetNumberOfComponents(other->GetNumberOfComponents());
    this->SetNumberOfTuples(other->GetNumberOfTuples());
    this->SetBackend(other->GetBackend());
  }

  ///@{
  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
   * This method checks if source->GetArrayType() returns DataArray
   * or a more derived type, and performs a static_cast to return
   * source as a vtkDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkImplicitArray<BackendT>* FastDownCast(vtkAbstractArray* source);
  ///@}

protected:
  vtkImplicitArray();
  ~vtkImplicitArray() override;

  ///@{
  /**
   * No allocation necessary
   */
  bool AllocateTuples(vtkIdType vtkNotUsed(numTuples)) { return true; }
  bool ReallocateTuples(vtkIdType vtkNotUsed(numTuples)) { return true; }
  ///@}

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  /**
   * The backend object actually mapping the indexes
   */
  std::shared_ptr<BackendT> Backend;

private:
  vtkImplicitArray(const vtkImplicitArray&) = delete;
  void operator=(const vtkImplicitArray&) = delete;

  ///@{
  /**
   * Methods for static dispatch towards map trait
   */
  template <typename U>
  typename std::enable_if<vtk::detail::has_map_trait<U>::value, ValueType>::type GetValueImpl(
    vtkIdType idx) const
  {
    return this->Backend->map(idx);
  }
  ///@}

  ///@{
  /**
   * Methods for static dispatch towards closure trait
   */
  template <typename U>
  typename std::enable_if<vtk::detail::is_closure_trait<U>::value, ValueType>::type GetValueImpl(
    vtkIdType idx) const
  {
    return (*this->Backend)(idx);
  }
  ///@}

  ///@{
  /**
   * Static dispatch Initialize for default constructible things
   */
  template <typename U>
  typename std::enable_if<vtk::detail::implicit_array_traits<U>::default_constructible, void>::type
  Initialize()
  {
    this->Backend = std::make_shared<BackendT>();
  }
  ///@}

  ///@{
  /**
   * Static dispatch Initialize for non-default constructible things
   */
  template <typename U>
  typename std::enable_if<!vtk::detail::implicit_array_traits<U>::default_constructible, void>::type
  Initialize()
  {
    this->Backend = nullptr;
  }
  ///@}

  ///@{
  /**
   * Static dispatch tuple mapping for compatible backends
   */
  template <typename U>
  typename std::enable_if<vtk::detail::implicit_array_traits<U>::can_direct_read_tuple, void>::type
  GetTypedTupleImpl(vtkIdType idx, ValueType* tuple) const
  {
    static_assert(
      std::is_same<typename vtk::detail::can_map_tuple_trait<U>::rtype, ValueType>::value,
      "Tuple type should be the same as the return type of the mapTuple");
    this->Backend->mapTuple(idx, tuple);
  }
  ///@}

  ///@{
  /**
   * Static dispatch tuple mapping using component mapping for compatible backends
   */
  template <typename U>
  typename std::enable_if<!vtk::detail::implicit_array_traits<U>::can_direct_read_tuple &&
      vtk::detail::implicit_array_traits<U>::can_direct_read_component,
    void>::type
  GetTypedTupleImpl(vtkIdType idx, ValueType* tuple) const
  {
    for (vtkIdType comp = 0; comp < this->NumberOfComponents; comp++)
    {
      tuple[comp] = this->GetTypedComponent(idx, comp);
    }
  }

  /**
   * Static dispatch tuple mapping for incompatible backends
   */
  template <typename U>
  typename std::enable_if<!vtk::detail::implicit_array_traits<U>::can_direct_read_tuple &&
      !vtk::detail::implicit_array_traits<U>::can_direct_read_component,
    void>::type
  GetTypedTupleImpl(vtkIdType idx, ValueType* tuple) const
  {
    const vtkIdType tupIdx = idx * this->NumberOfComponents;
    for (vtkIdType comp = 0; comp < this->NumberOfComponents; comp++)
    {
      tuple[comp] = this->GetValue(tupIdx + comp);
    }
  }
  ///@}

  ///@{
  /**
   * Static dispatch component mapping for compatible backends
   */
  template <typename U>
  typename std::enable_if<vtk::detail::implicit_array_traits<U>::can_direct_read_component,
    ValueType>::type
  GetTypedComponentImpl(vtkIdType idx, int comp) const
  {
    static_assert(
      std::is_same<typename vtk::detail::can_map_component_trait<U>::rtype, ValueType>::value,
      "Component return type should be the same as the return type of the mapComponent");
    return this->Backend->mapComponent(idx, comp);
  }
  ///@}

  ///@{
  /**
   * Static dispatch component mapping for incompatible backends
   */
  template <typename U>
  typename std::enable_if<!vtk::detail::implicit_array_traits<U>::can_direct_read_component,
    ValueType>::type
  GetTypedComponentImpl(vtkIdType idx, int comp) const
  {
    return this->GetValue(idx * this->NumberOfComponents + comp);
  }
  ///@}

  ///@{
  /**
   * Static call to get memory size for compatible backends
   */
  template <typename U>
  typename std::enable_if<vtk::detail::implicit_array_traits<U>::can_get_memory_size,
    unsigned long>::type
  GetActualMemorySizeImpl() const
  {
    return this->Backend->getMemorySize();
  }

  /**
   * Static call to get memory size for incompatible backends
   * For those backends, dhe default memory size is 1KiB.
   */
  template <typename U>
  typename std::enable_if<!vtk::detail::implicit_array_traits<U>::can_get_memory_size,
    unsigned long>::type
  GetActualMemorySizeImpl() const
  {
    return 1;
  }
  ///@}

  friend class vtkGenericDataArray<vtkImplicitArray<BackendT>, ValueTypeT>;
};

// Declare vtkArrayDownCast implementations for implicit containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkImplicitArray);
VTK_ABI_NAMESPACE_END

#include "vtkImplicitArray.txx"

#endif // vtkImplicitArray_h

// See vtkGenericDataArray for similar section
#ifdef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
struct vtkAffineImplicitBackend;
template <typename ValueType>
class vtkCompositeImplicitBackend;
template <typename ValueType>
struct vtkConstantImplicitBackend;
template <typename ValueType>
class vtkStructuredPointBackend;
template <typename ValueType>
class vtkIndexedImplicitBackend;
VTK_ABI_NAMESPACE_END
#include <functional>

// Needed to export for this module and not CommonCore
#define VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(ArrayType, ValueType)                                 \
  template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(                                         \
    ArrayType*, ValueType*, vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);  \
  template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(ArrayType*, ValueType*,                  \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);                       \
  template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],                \
    vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);                          \
  template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],                \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);

#define VTK_INSTANTIATE_VALUERANGE_VALUETYPE(ValueType)                                            \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkAffineImplicitBackend<ValueType>>, ValueType)                              \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>, ValueType)                           \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkConstantImplicitBackend<ValueType>>, ValueType)                            \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkStructuredPointBackend<ValueType>>, ValueType)                             \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkIndexedImplicitBackend<ValueType>>, ValueType)                             \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<ValueType(int)>>, ValueType)

#elif defined(VTK_USE_EXTERN_TEMPLATE) // VTK_IMPLICIT_VALUERANGE_INSTANTIATING

#ifndef VTK_IMPLICIT_TEMPLATE_EXTERN
#define VTK_IMPLICIT_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the following is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif

VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
struct vtkAffineImplicitBackend;
template <typename ValueType>
class vtkCompositeImplicitBackend;
template <typename ValueType>
struct vtkConstantImplicitBackend;
template <typename ValueType>
class vtkStructuredPointBackend;
template <typename ValueType>
class vtkIndexedImplicitBackend;
VTK_ABI_NAMESPACE_END
#include <functional>

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
template <typename A, typename R, typename T>
VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(
  A*, R*, T, const unsigned char* ghosts, unsigned char ghostsToSkip);
template <typename A, typename R>
VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(
  A*, R[2], AllValues, const unsigned char* ghosts, unsigned char ghostsToSkip);
template <typename A, typename R>
VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(
  A*, R[2], FiniteValues, const unsigned char* ghosts, unsigned char ghostsToSkip);
VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#define VTK_DECLARE_VALUERANGE_ARRAYTYPE(ArrayType, ValueType)                                     \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(                                  \
    ArrayType*, ValueType*, vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);  \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(ArrayType*, ValueType*,           \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);                       \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],         \
    vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);                          \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],         \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);

#define VTK_DECLARE_VALUERANGE_VALUETYPE(ValueType)                                                \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(                                                                \
    vtkImplicitArray<vtkAffineImplicitBackend<ValueType>>, ValueType)                              \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(                                                                \
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>, ValueType)                           \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(                                                                \
    vtkImplicitArray<vtkConstantImplicitBackend<ValueType>>, ValueType)                            \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(                                                                \
    vtkImplicitArray<vtkStructuredPointBackend<ValueType>>, ValueType)                             \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(                                                                \
    vtkImplicitArray<vtkIndexedImplicitBackend<ValueType>>, ValueType)                             \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<ValueType(int)>>, ValueType)

#define VTK_DECLARE_VALUERANGE_IMPLICIT_BACKENDTYPE(BackendT)                                      \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<float>>, double)                      \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<double>>, double)                     \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<char>>, double)                       \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<signed char>>, double)                \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<unsigned char>>, double)              \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<short>>, double)                      \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<unsigned short>>, double)             \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<int>>, double)                        \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<unsigned int>>, double)               \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<long>>, double)                       \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<unsigned long>>, double)              \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<long long>>, double)                  \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<BackendT<unsigned long long>>, double)

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
VTK_DECLARE_VALUERANGE_IMPLICIT_BACKENDTYPE(vtkAffineImplicitBackend)
VTK_DECLARE_VALUERANGE_IMPLICIT_BACKENDTYPE(vtkConstantImplicitBackend)
VTK_DECLARE_VALUERANGE_IMPLICIT_BACKENDTYPE(vtkCompositeImplicitBackend)
VTK_DECLARE_VALUERANGE_IMPLICIT_BACKENDTYPE(vtkStructuredPointBackend)
VTK_DECLARE_VALUERANGE_IMPLICIT_BACKENDTYPE(vtkIndexedImplicitBackend)

VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<float(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<double(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<char(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<signed char>(int)>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<unsigned char(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<short(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<unsigned short(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<int(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<unsigned int(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<long(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<unsigned long(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<long long(int)>>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<unsigned long long(int)>>, double)
VTK_ABI_NAMESPACE_END
}

#undef VTK_DECLARE_VALUERANGE_ARRAYTYPE
#undef VTK_DECLARE_VALUERANGE_VALUETYPE

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_IMPLICIT_TEMPLATE_EXTERN

#endif // VTK_IMPLICIT_VALUERANGE_INSTANTIATING
