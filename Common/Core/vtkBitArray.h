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

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"

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

  static vtkBitArray* New();
  vtkTypeMacro(vtkBitArray, vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  int GetDataType() const override { return VTK_BIT; }
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

  /**
   * Set the data component at the ith tuple and jth component location.
   * Note that i is less then NumberOfTuples and j is less then
   * NumberOfComponents. Make sure enough memory has been allocated (use
   * SetNumberOfTuples() and  SetNumberOfComponents()).
   *
   * NOT THREAD-SAFE
   */
  void SetComponent(vtkIdType i, int j, double c) override;

  /**
   * Free any unneeded memory.
   */
  void Squeeze() override;

  /**
   * Resize the array while conserving the data.
   */
  vtkTypeBool Resize(vtkIdType numTuples) override;

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
  unsigned char* GetPointer(vtkIdType id) { return this->Array + id / 8; }

  /**
   * Get the address of a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  unsigned char* WritePointer(vtkIdType id, vtkIdType number);

  void* WriteVoidPointer(vtkIdType id, vtkIdType number) override
  {
    return this->WritePointer(id, number);
  }

  void* GetVoidPointer(vtkIdType id) override { return static_cast<void*>(this->GetPointer(id)); }

  /**
   * Deep copy of another bit array.
   */
  void DeepCopy(vtkDataArray* da) override;
  void DeepCopy(vtkAbstractArray* aa) override { this->Superclass::DeepCopy(aa); }

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
    unsigned char* array, vtkIdType size, int save, int deleteMethod = VTK_DATA_ARRAY_DELETE);
#endif
  void SetVoidArray(void* array, vtkIdType size, int save) override
  {
    this->SetArray(static_cast<unsigned char*>(array), size, save);
  }
  void SetVoidArray(void* array, vtkIdType size, int save, int deleteMethod) override
  {
    this->SetArray(static_cast<unsigned char*>(array), size, save, deleteMethod);
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

  unsigned char* Array; // pointer to data
  unsigned char* ResizeAndExtend(vtkIdType sz);
  // function to resize data

  int TupleSize; // used for data conversion
  double* Tuple;

  void (*DeleteFunction)(void*);

private:
  // hide superclass' DeepCopy() from the user and the compiler
  void DeepCopy(vtkDataArray& da) { this->vtkDataArray::DeepCopy(&da); }

  vtkBitArray(const vtkBitArray&) = delete;
  void operator=(const vtkBitArray&) = delete;

  vtkBitArrayLookup* Lookup;
  void UpdateLookup();
};

inline void vtkBitArray::SetValue(vtkIdType id, int value)
{
  this->Array[id / 8] =
    static_cast<unsigned char>((value != 0) ? (this->Array[id / 8] | (0x80 >> id % 8))
                                            : (this->Array[id / 8] & (~(0x80 >> id % 8))));
  this->DataChanged();
}

inline void vtkBitArray::InsertValue(vtkIdType id, int i)
{
  if (id >= this->Size)
  {
    if (!this->ResizeAndExtend(id + 1))
    {
      return;
    }
  }
  this->Array[id / 8] =
    static_cast<unsigned char>((i != 0) ? (this->Array[id / 8] | (0x80 >> id % 8))
                                        : (this->Array[id / 8] & (~(0x80 >> id % 8))));
  if (id > this->MaxId)
  {
    this->MaxId = id;
    this->InitializeUnusedBitsInLastByte();
  }
  this->DataChanged();
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
  this->DataChanged();
  return this->MaxId;
}

inline void vtkBitArray::Squeeze()
{
  this->ResizeAndExtend(this->MaxId + 1);
}
VTK_ABI_NAMESPACE_END
#endif
