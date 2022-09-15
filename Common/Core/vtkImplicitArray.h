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

#include "vtkAOSDataArrayTemplate.h" // for copies
#include "vtkCommonCoreModule.h"     // for export macro
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
  vtkGetMacro(Backend, std::shared_ptr<BackendT>);
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

protected:
  vtkImplicitArray();
  ~vtkImplicitArray() override;

  bool AllocateTuples(vtkIdType numTuples) { return true; };
  bool ReallocateTuples(vtkIdType numTuples) { return true; };

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

  friend class vtkGenericDataArray<vtkImplicitArray<BackendT>, ValueTypeT>;
};

#include "vtkImplicitArray.txx"

#endif // vtkImplicitArray_h
