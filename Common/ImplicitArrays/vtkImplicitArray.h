/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkImplicitArray_h
#define vtkImplicitArray_h

#include "vtkCommonImplicitArraysModule.h" // for export macro
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
 * implements a const operator() method from integers to the value type of the array.
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
 * - static dispatch of the calls to the backend (when combined with the CRTP strcture of
 * vtkGenericDataArray)
 * - flexibility regarding the complexity/simplicity/nature/type... of the backend one would like to
 * use
 *
 * Example for array that always returns 42:
 * @code
 * struct Const42
 * {
 *   int operator()(int idx) const { return 42; }
 * };
 * vtkNew<vtkImplicitArray<Const42>> arr42;
 * @endcode
 *
 * @sa
 * vtkGenericDataArray vtkImplicitArrayTraits vtkDataArray
 */
VTK_ABI_NAMESPACE_BEGIN
template <class BackendT>
class vtkImplicitArray
  : public vtkGenericDataArray<vtkImplicitArray<BackendT>,
      typename vtk::detail::implicit_array_traits<BackendT>::rtype>
{
  using trait = vtk::detail::implicit_array_traits<BackendT>;
  static_assert(trait::can_read,
    "Supplied backend type does not have mandatory read trait. Must implement either map() const "
    "or "
    "operator() const.");
  using ValueTypeT = typename trait::rtype;
  using GenericDataArrayType = vtkGenericDataArray<vtkImplicitArray<BackendT>, ValueTypeT>;

public:
  using SelfType = vtkImplicitArray<BackendT>;
  vtkTemplateTypeMacro(SelfType, GenericDataArrayType);
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
  inline ValueType GetValue(vtkIdType idx) const { return this->GetValue<BackendT>(idx); }

  /**
   * Will not do anything for these read only arrays!
   */
  void SetValue(vtkIdType idx, ValueType value);

  /**
   * Copy the tuple at @a idx into @a tuple.
   */
  void GetTypedTuple(vtkIdType idx, ValueType* tuple) const
  {
    const vtkIdType tupIdx = idx * this->NumberOfComponents;
    for (vtkIdType comp = 0; comp < this->NumberOfComponents; comp++)
    {
      tuple[comp] = this->GetValue(tupIdx + comp);
    }
  }

  /**
   * Will not do anything for these read only arrays!
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple);

  /**
   * Get component @a comp of the tuple at @a idx.
   */
  inline ValueType GetTypedComponent(vtkIdType idx, int comp) const
  {
    return this->GetValue(idx * this->NumberOfComponents + comp);
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
   * Use of this method is discouraged, it creates a memory copy of the data into
   * a contiguous AoS-ordered buffer internally.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;

  /**
   * Release all extraneous internal memory including the void pointer
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
   * Specific DeepCopy for implicit arrays
   *
   * This method should be prefered for two implicit arrays having the same backend. We cannot call
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
  typename std::enable_if<vtk::detail::has_map_trait<U>::value, ValueType>::type GetValue(
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
  typename std::enable_if<vtk::detail::is_closure_trait<U>::value, ValueType>::type GetValue(
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
   * Static dispatch Initialize for non-default constuctible things
   */
  template <typename U>
  typename std::enable_if<!vtk::detail::implicit_array_traits<U>::default_constructible, void>::type
  Initialize()
  {
    this->Backend = nullptr;
  }
  ///@}

  friend class vtkGenericDataArray<vtkImplicitArray<BackendT>, ValueTypeT>;
};

// Declare vtkArrayDownCast implementations for implicit containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkImplicitArray);
VTK_ABI_NAMESPACE_END

#include "vtkImplicitArray.txx"

#define vtkInstantiateSecondOrderTemplateMacro(decl0, decl1)                                       \
  decl0<decl1<float>>;                                                                             \
  decl0<decl1<double>>;                                                                            \
  decl0<decl1<char>>;                                                                              \
  decl0<decl1<signed char>>;                                                                       \
  decl0<decl1<unsigned char>>;                                                                     \
  decl0<decl1<short>>;                                                                             \
  decl0<decl1<unsigned short>>;                                                                    \
  decl0<decl1<int>>;                                                                               \
  decl0<decl1<unsigned int>>;                                                                      \
  decl0<decl1<long>>;                                                                              \
  decl0<decl1<unsigned long>>;                                                                     \
  decl0<decl1<long long>>;                                                                         \
  decl0<decl1<unsigned long long>>

#endif // vtkImplicitArray_h
