// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBitArray
 * @brief   dynamic, self-adjusting array of bits
 *
 * vtkBitArray is an array of bits (0/1 data value). The array is packed
 * so that each byte stores eight bits. vtkBitArray provides methods
 * for insertion and retrieval of bits, and will automatically resize
 * itself to hold new data.
 *
 * > WARNING
 * > This class is not thread-safe during write access
 */

#ifndef vtkBitArray_h
#define vtkBitArray_h

#include "vtkBuffer.h"           // For vtkBuffer
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"

#include <cassert> // for assert
#include <vector>  // for vector

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArrayLookup;

class VTKCOMMONCORE_EXPORT vtkBitArray : public vtkDataArray
{
public:
  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE = vtkAbstractArray::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE = vtkAbstractArray::VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE = vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE,
    VTK_DATA_ARRAY_USER_DEFINED = vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED
  };
  using ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_BIT_ARRAY>;
  using DataTypeTag = std::integral_constant<int, VTK_BIT>;
  using ValueType = unsigned char;

  static vtkBitArray* New();
  vtkTypeMacro(vtkBitArray, vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkBitArray.
   * This method checks if source->GetArrayType() returns BitArray,
   * and performs a static_cast to return source as a vtkBitArray pointer.
   * Otherwise, nullptr is returned.
   */
  static vtkBitArray* FastDownCast(vtkAbstractArray* source);

  /**
   * Allocate memory for this array. Delete old storage only if necessary.
   * Note that ext is no longer used.
   */
  vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext = 1000) override;

  /**
   * Release storage and reset array to initial state.
   */
  void Initialize() override;

  // satisfy vtkDataArray API
  int GetArrayType() const override { return vtkBitArray::ArrayTypeTag::value; }
  int GetDataType() const override { return vtkBitArray::DataTypeTag::value; }
  int GetDataTypeSize() const override { return 0; }

  /**
   * Set the number of n-tuples in the array.
   */
  void SetNumberOfTuples(vtkIdType number) override;

  /**
   * In addition to setting the number of values, this method also sets the
   * unused bits of the last byte of the array.
   */
  bool SetNumberOfValues(vtkIdType number) override;

  /**
   * Set the tuple at the ith location using the jth tuple in the source array.
   * This method assumes that the two arrays have the same type
   * and structure. Note that range checking and memory allocation is not
   * performed; use in conjunction with SetNumberOfTuples() to allocate space.
   *
   * NOT THREAD-SAFE
   */
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;

  /**
   * Insert the jth tuple in the source array, at ith location in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   *
   * NOT THREAD-SAFE
   */
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;

  /**
   * Copy the tuples indexed in srcIds from the source array to the tuple
   * locations indexed by dstIds in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   *
   * NOT THREAD-SAFE
   */
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override;

  /**
   * Copy the tuples indexed in srcIds from the source array to the tuple
   * locations starting at index dstStart.
   * Note that memory allocation is performed as necessary to hold the data.
   *
   * NOT THREAD-SAFE
   */
  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override;

  /**
   * Copy n consecutive tuples starting at srcStart from the source array to
   * this array, starting at the dstStart location.
   * Note that memory allocation is performed as necessary to hold the data.
   *
   * NOT THREAD-SAFE
   */
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;

  /**
   * Insert the jth tuple in the source array, at the end in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   * Returns the location at which the data was inserted.
   *
   * NOT THREAD-SAFE
   */
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) override;

  /**
   * Get a pointer to a tuple at the ith location. This is a dangerous method
   * (it is not thread safe since a pointer is returned).
   */
  double* GetTuple(vtkIdType i) override;

  /**
   * Copy the tuple value into a user-provided array.
   */
  void GetTuple(vtkIdType i, double* tuple) override;

  ///@{
  /**
   * Set the tuple value at the ith location in the array.
   *
   * NOT THREAD-SAFE
   */
  void SetTuple(vtkIdType i, const float* tuple) override;
  void SetTuple(vtkIdType i, const double* tuple) override;
  ///@}

  ///@{
  /**
   * Insert (memory allocation performed) the tuple into the ith location
   * in the array.
   *
   * NOT THREAD-SAFE
   */
  void InsertTuple(vtkIdType i, const float* tuple) override;
  void InsertTuple(vtkIdType i, const double* tuple) override;
  ///@}

  ///@{
  /**
   * Insert (memory allocation performed) the tuple onto the end of the array.
   *
   * NOT THREAD-SAFE
   */
  vtkIdType InsertNextTuple(const float* tuple) override;
  vtkIdType InsertNextTuple(const double* tuple) override;
  ///@}

  ///@{
  /**
   * These methods remove tuples from the data array. They shift data and
   * resize array, so the data array is still valid after this operation. Note,
   * this operation is fairly slow.
   *
   * NOT THREAD-SAFE
   */
  void RemoveTuple(vtkIdType id) override;
  void RemoveFirstTuple() override;
  void RemoveLastTuple() override;
  ///@}

  ///@{
  /**
   * Set/Get the data component at the ith tuple and jth component location.
   * Note that i is less then NumberOfTuples and j is less then
   * NumberOfComponents. Make sure enough memory has been allocated (use
   * SetNumberOfTuples() and  SetNumberOfComponents()).
   *
   * NOT THREAD-SAFE
   */
  double GetComponent(vtkIdType tupleIdx, int compIdx) override;
  void SetComponent(vtkIdType i, int j, double c) override;
  ///@}

  /**
   * Free any unneeded memory.
   */
  void Squeeze() override { this->Resize(this->GetNumberOfTuples()); }

  /**
   * Resize the array while conserving the data.
   */
  vtkTypeBool Resize(vtkIdType numTuples) override;

  /**
   * Get component @a comp of the tuple at @a tupleIdx.
   */
  ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + comp < GetNumberOfValues())
      VTK_EXPECTS(0 <= comp && comp < GetNumberOfComponents());

  ///@{
  /**
   * Set component @a comp of the tuple at @a tupleIdx to @a value.
   */
  void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + comp < GetNumberOfValues())
      VTK_EXPECTS(0 <= comp && comp < GetNumberOfComponents());
  ///@}

  ///@{
  /**
   * Copy the tuple at @a tupleIdx into @a tuple.
   */
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  ///@}

  ///@{
  /**
   * Set this array's tuple at @a tupleIdx to the values in @a tuple.
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples());
  ///@}

  /**
   * Get the data at a particular index.
   */
  int GetValue(vtkIdType id) const;

  /**
   * Set the data at a particular index. Does not do range checking. Make sure
   * you use the method SetNumberOfValues() before inserting data.
   *
   * NOT THREAD-SAFE
   */
  void SetValue(vtkIdType id, int value);

  /**
   * Inserts values and checks to make sure there is enough memory
   *
   * NOT THREAD-SAFE
   */
  void InsertValue(vtkIdType id, int i);

  /**
   * Get a value in the array as a variant.
   */
  vtkVariant GetVariantValue(vtkIdType idx) override;

  /**
   * Set a value in the array from a variant.
   *
   * NOT THREAD-SAFE
   */
  void SetVariantValue(vtkIdType idx, vtkVariant value) override;

  /**
   * Inserts values from a variant and checks to ensure there is enough memory
   *
   * NOT THREAD-SAFE
   */
  void InsertVariantValue(vtkIdType idx, vtkVariant value) override;

  vtkIdType InsertNextValue(int i);

  /**
   * Insert the data component at ith tuple and jth component location.
   * Note that memory allocation is performed as necessary to hold the data.
   *
   * NOT THREAD-SAFE
   */
  void InsertComponent(vtkIdType i, int j, double c) override;

  /**
   * Direct manipulation of the underlying data.
   */
  ValueType* GetPointer(vtkIdType id) { return this->Buffer->GetBuffer() + id / 8; }

  /**
   * Get the address of a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  ValueType* WritePointer(vtkIdType id, vtkIdType number);

  void* WriteVoidPointer(vtkIdType id, vtkIdType number) override
  {
    return this->WritePointer(id, number);
  }

  void* GetVoidPointer(vtkIdType id) override { return static_cast<void*>(this->GetPointer(id)); }

  /**
   * Return the underlying buffer object. This can be used for zero-copy
   * access to the array data, particularly useful for Python buffer protocol
   * support.
   */
#ifdef __VTK_WRAP__
  vtkAbstractBuffer* GetBuffer() { return this->Buffer; }
#else
  vtkBuffer<ValueType>* GetBuffer() { return this->Buffer; }
#endif // __VTK_WRAP__

  /**
   * Deep copy of another bit array.
   */
  void DeepCopy(vtkDataArray* da) override;
  void DeepCopy(vtkAbstractArray* aa) override { this->Superclass::DeepCopy(aa); }

  /**
   * Shallow copy of another bit array.
   */
  void ShallowCopy(vtkDataArray* da) override;
  using vtkAbstractArray::ShallowCopy;

  ///@{
  /**
   * This method lets the user specify data to be held by the array.  The
   * array argument is a pointer to the data.  size is the size of
   * the array supplied by the user.  Set save to 1 to keep the class
   * from deleting the array when it cleans up or reallocates memory.
   * The class uses the actual array provided; it does not copy the data
   * from the supplied array.
   * If the delete method is VTK_DATA_ARRAY_USER_DEFINED
   * a custom free function can be assigned to be called using SetArrayFreeFunction,
   * if no custom function is assigned we will default to delete[].
   */
#ifndef __VTK_WRAP__
  void SetArray(
    ValueType* array, vtkIdType size, int save, int deleteMethod = VTK_DATA_ARRAY_DELETE);
#endif
  void SetVoidArray(void* array, vtkIdType size, int save) override
  {
    this->SetArray(static_cast<ValueType*>(array), size, save);
  }
  void SetVoidArray(void* array, vtkIdType size, int save, int deleteMethod) override
  {
    this->SetArray(static_cast<ValueType*>(array), size, save, deleteMethod);
  }
  ///@}

  /**
   * This method allows the user to specify a custom free function to be
   * called when the array is deallocated. Calling this method will implicitly
   * mean that the given free function will be called when the class
   * cleans up or reallocates memory.
   **/
  void SetArrayFreeFunction(void (*callback)(void*)) override;

  /**
   * Returns a new vtkBitArrayIterator instance.
   */
  VTK_DEPRECATED_IN_9_7_0("Use vtk::DataArrayValueRange, or the array directly")
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;

  ///@{
  /**
   * Return the indices where a specific value appears.
   */
  vtkIdType LookupValue(vtkVariant value) override;
  void LookupValue(vtkVariant value, vtkIdList* ids) override;
  vtkIdType LookupValue(int value);
  void LookupValue(int value, vtkIdList* ids);
  ///@}

  /**
   * Tell the array explicitly that the data has changed.
   * This is only necessary to call when you modify the array contents
   * without using the array's API (i.e. you retrieve a pointer to the
   * data and modify the array contents).  You need to call this so that
   * the fast lookup will know to rebuild itself.  Otherwise, the lookup
   * functions will give incorrect results.
   */
  void DataChanged() override;

  /**
   * Delete the associated fast lookup data structure on this array,
   * if it exists.  The lookup will be rebuilt on the next call to a lookup
   * function.
   */
  void ClearLookup() override;

protected:
  vtkBitArray();
  ~vtkBitArray() override;

  /**
   * This method should be called
   * whenever MaxId needs to be changed, as this method fills the unused bits of
   * the last byte to zero. If those bits are kept uninitialized, one can
   * trigger errors when reading the last byte.
   *
   * @note This method can be called with `this->MaxId < 0`. In this instance, nothing happens.
   *
   * @warning The buffer `this->Array` needs to already be allocated prior to calling this
   * method.
   */
  virtual void InitializeUnusedBitsInLastByte();

  /**
   * Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
   * all data is freed.
   */
  bool AllocateTuples(vtkIdType numTuples);

  /**
   * Allocate space for numTuples. Old data is preserved. If numTuples == 0, all data is freed.
   */
  bool ReallocateTuples(vtkIdType numTuples);

  vtkBuffer<ValueType>* Buffer; // pointer to data
  std::vector<double> LegacyTuple;

  /**
   * Function to resize data
   */
  VTK_DEPRECATED_IN_9_7_0("Use Resize")
  ValueType* ResizeAndExtend(vtkIdType size)
  {
    if (!this->Resize(size / this->NumberOfComponents + 1))
    {
      return nullptr;
    }
    return this->Buffer->GetBuffer();
  }

private:
  // hide superclass' DeepCopy() from the user and the compiler
  void DeepCopy(vtkDataArray& da) { this->vtkDataArray::DeepCopy(&da); }

  vtkBitArray(const vtkBitArray&) = delete;
  void operator=(const vtkBitArray&) = delete;

  vtkBitArrayLookup* Lookup;
  void UpdateLookup();
};

// Declare vtkArrayDownCast implementations for vtkBitArray:
vtkArrayDownCast_FastCastMacro(vtkBitArray);

inline void vtkBitArray::SetValue(vtkIdType id, int value)
{
  const auto bitsetDiv = std::div(id, static_cast<vtkIdType>(8));
  const vtkIdType &bitsetId = bitsetDiv.quot, &bitId = bitsetDiv.rem;
  ValueType mask = 0x80 >> bitId; // NOLINT(clang-analyzer-core.BitwiseShift)
  this->Buffer->GetBuffer()[bitsetId] =
    static_cast<ValueType>((value != 0) ? (this->Buffer->GetBuffer()[bitsetId] | mask)
                                        : (this->Buffer->GetBuffer()[bitsetId] & (~mask)));
  this->DataChanged();
}

inline void vtkBitArray::InsertValue(vtkIdType valueIdx, int value)
{
  if (valueIdx >= this->Size)
  {
    if (!this->Resize((valueIdx + 1) / this->NumberOfComponents + 1))
    {
      return;
    }
  }
  this->SetValue(valueIdx, value);
  if (valueIdx > this->MaxId)
  {
    this->MaxId = valueIdx;
    this->InitializeUnusedBitsInLastByte();
  }
}

inline vtkVariant vtkBitArray::GetVariantValue(vtkIdType id)
{
  return vtkVariant(this->GetValue(id));
}

inline void vtkBitArray::SetVariantValue(vtkIdType id, vtkVariant value)
{
  this->SetValue(id, value.ToInt());
}

inline void vtkBitArray::InsertVariantValue(vtkIdType id, vtkVariant value)
{
  this->InsertValue(id, value.ToInt());
}

inline vtkIdType vtkBitArray::InsertNextValue(int i)
{
  this->InsertValue(this->MaxId + 1, i);
  return this->MaxId;
}
VTK_ABI_NAMESPACE_END
#endif
