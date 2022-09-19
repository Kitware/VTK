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

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkGenericDataArray.h"
#include "vtkImplicitArrayTraits.h" // for traits

#include <memory>
#include <type_traits>

template <class BackendT>
class VTKCOMMONCORE_EXPORT vtkImplicitArray
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
   * Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
   */
  inline ValueType GetValue(vtkIdType idx) const { return this->GetValue<BackendT>(idx); }

  /**
   * Will fail for these read only arrays!
   */
  void SetValue(vtkIdType idx, ValueType value);

  /**
   * Copy the tuple at @a tupleIdx into @a tuple.
   */
  void GetTypedTuple(vtkIdType idx, ValueType* tuple)
  {
    const vtkIdType tupIdx = idx * this->NumberOfComponents;
    for (vtkIdType comp = 0; comp < this->NumberOfComponents; comp++)
    {
      tuple[comp] = this->GetValue(tupIdx + comp);
    }
  }

  /**
   * Will fail for these read only arrays!
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple);

  /**
   * Get component @a comp of the tuple at @a tupleIdx.
   */
  inline ValueType GetTypedComponent(vtkIdType idx, int comp) const
  {
    return this->GetValue(idx * this->GetNumberOfTuples() + comp);
  }

  /**
   * Will fail for these read only arrays!
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
  };
  std::shared_ptr<BackendT> GetBackend() { return this->Backend; };
  ///@}

  /**
   * Use of this method is discouraged, it creates a memory copy of the data into
   * a contiguous AoS-ordered buffer internally and prints a warning.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;

  /**
   * Release all extraneous internal memory
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
  };

#ifndef __VTK_WRAP__
  ///@{
  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
   * This method checks if source->GetArrayType() returns DataArray
   * or a more derived type, and performs a static_cast to return
   * source as a vtkDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkImplicitArray<BackendT>* FastDownCast(vtkAbstractArray* source);
  ///@}
#endif

protected:
  vtkImplicitArray();
  ~vtkImplicitArray() override;

  ///@{
  /**
   * No allocation necessary
   */
  bool AllocateTuples(vtkIdType numTuples) { return true; };
  bool ReallocateTuples(vtkIdType numTuples) { return true; };
  ///@}

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

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

#include "vtkImplicitArray.txx"

#endif // vtkImplicitArray_h
