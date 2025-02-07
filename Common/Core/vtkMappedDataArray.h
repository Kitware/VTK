// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMappedDataArray
 * @brief   Map non-contiguous data structures into the
 * vtkDataArray API.
 *
 *
 * vtkMappedDataArray is a superclass for vtkDataArrays that do not use
 * the standard memory layout, and allows VTK to interface with
 * simulation codes for in-situ analysis without repacking simulation data.
 *
 * vtkMappedDataArrayNewInstanceMacro is used by subclasses to implement
 * NewInstanceInternal such that a non-mapped vtkDataArray is returned by
 * NewInstance(). This prevents the mapped array type from propagating
 * through the pipeline.
 *
 * @attention
 * Subclasses that hold vtkIdType elements must also
 * reimplement `int GetDataType() const` (see Caveat in vtkTypedDataArray).
 */

#ifndef vtkMappedDataArray_h
#define vtkMappedDataArray_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkTypedDataArray.h"

VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
class VTK_DEPRECATED_IN_9_5_0("Please use the SetArray functionality of `vtkAOSDataArrayTemplate` "
                              "for 1 component or `vtkSOADataArrayTemplate` for more "
                              "instead.") vtkMappedDataArray : public vtkTypedDataArray<Scalar>
{
public:
  vtkTemplateTypeMacro(vtkMappedDataArray<Scalar>, vtkTypedDataArray<Scalar>);
  typedef typename Superclass::ValueType ValueType;

  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkMappedDataArray.
   * This method checks if:
   * - source->GetArrayType() is appropriate, and
   * - source->GetDataType() matches the Scalar template argument
   * if these conditions are met, the method performs a static_cast to return
   * source as a vtkMappedDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkMappedDataArray<Scalar>* FastDownCast(vtkAbstractArray* source);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  // vtkAbstractArray virtual method that must be reimplemented.
  void DeepCopy(vtkAbstractArray* aa) override = 0;
  vtkVariant GetVariantValue(vtkIdType idx) override = 0;
  void SetVariantValue(vtkIdType idx, vtkVariant value) override = 0;
  void GetTuples(vtkIdList* ptIds, vtkAbstractArray* output) override = 0;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output) override = 0;
  void InterpolateTuple(
    vtkIdType i, vtkIdList* ptIndices, vtkAbstractArray* source, double* weights) override = 0;
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray* source1, vtkIdType id2,
    vtkAbstractArray* source2, double t) override = 0;

  // vtkDataArray virtual method that must be reimplemented.
  void DeepCopy(vtkDataArray* da) override = 0;

  /**
   * Print an error and create an internal, long-lived temporary array. This
   * method should not be used on vtkMappedDataArray subclasses. See
   * vtkArrayDispatch for a better way.
   */
  void* GetVoidPointer(vtkIdType id) override;

  /**
   * Copy the internal data to the void pointer. The pointer is cast to this
   * array's Scalar type and vtkTypedDataArrayIterator is used to populate
   * the input array.
   */
  void ExportToVoidPointer(void* ptr) override;

  /**
   * Read the data from the internal temporary array (created by GetVoidPointer)
   * back into the mapped array. If GetVoidPointer has not been called (and the
   * internal array therefore does not exist), print an error and return. The
   * default implementation uses vtkTypedDataArrayIterator to extract the mapped
   * data.
   */
  void DataChanged() override;

  ///@{
  /**
   * These methods don't make sense for mapped data array. Prints an error and
   * returns.
   */
  void SetVoidArray(void*, vtkIdType, int) override;
  void SetVoidArray(void*, vtkIdType, int, int) override;
  ///@}

  ///@{
  /**
   * Not implemented. Print error and return nullptr.
   */
  void* WriteVoidPointer(vtkIdType /*id*/, vtkIdType /*number*/) override
  {
    vtkErrorMacro(<< "WriteVoidPointer: Method not implemented.");
    return nullptr;
  }
  ///@}

  /**
   * Invalidate the internal temporary array and call superclass method.
   */
  void Modified() override;

  // vtkAbstractArray override:
  bool HasStandardMemoryLayout() const override { return false; }

protected:
  vtkMappedDataArray();
  ~vtkMappedDataArray() override;

  int GetArrayType() const override { return vtkAbstractArray::MappedDataArray; }

private:
  vtkMappedDataArray(const vtkMappedDataArray&) = delete;
  void operator=(const vtkMappedDataArray&) = delete;

  ///@{
  /**
   * GetVoidPointer.
   */
  ValueType* TemporaryScalarPointer;
  size_t TemporaryScalarPointerSize;
  ///@}
};

// Declare vtkArrayDownCast implementations for mapped containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkMappedDataArray);

VTK_ABI_NAMESPACE_END
#include "vtkMappedDataArray.txx"

// Adds an implementation of NewInstanceInternal() that returns an AoS
// (unmapped) VTK array, if possible. Use this in combination with
// vtkAbstractTemplateTypeMacro when your subclass is a template class.
// Otherwise, use vtkMappedDataArrayTypeMacro.
#define vtkMappedDataArrayNewInstanceMacro(thisClass)                                              \
protected:                                                                                         \
  vtkObjectBase* NewInstanceInternal() const override                                              \
  {                                                                                                \
    if (vtkDataArray* da = vtkDataArray::CreateDataArray(thisClass::VTK_DATA_TYPE))                \
    {                                                                                              \
      return da;                                                                                   \
    }                                                                                              \
    return thisClass::New();                                                                       \
  }                                                                                                \
                                                                                                   \
public:

// Same as vtkTypeMacro, but adds an implementation of NewInstanceInternal()
// that returns a standard (unmapped) VTK array, if possible.
#define vtkMappedDataArrayTypeMacro(thisClass, superClass)                                         \
  vtkAbstractTypeMacroWithNewInstanceType(thisClass, superClass, vtkDataArray);                    \
  vtkMappedDataArrayNewInstanceMacro(thisClass)

#endif // vtkMappedDataArray_h

// VTK-HeaderTest-Exclude: vtkMappedDataArray.h
