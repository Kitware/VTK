// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkStringArray
 * @brief   a vtkAbstractArray subclass for strings
 *
 * Points and cells may sometimes have associated data that are stored
 * as strings, e.g. labels for information visualization projects.
 * This class provides a clean way to store and access those strings.
 * @par Thanks:
 * Andy Wilson (atwilso@sandia.gov) wrote this class.
 */

#ifndef vtkStringArray_h
#define vtkStringArray_h

#include "vtkAbstractArray.h"
#include "vtkBuffer.h"           // For vtkBuffer
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkStdString.h"        // needed for vtkStdString definition
#include "vtkWrappingHints.h"    // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArrayLookup;

class VTKCOMMONCORE_EXPORT VTK_MARSHALMANUAL vtkStringArray : public vtkAbstractArray
{
public:
  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE = vtkAbstractArray::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE = vtkAbstractArray::VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE = vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE,
    VTK_DATA_ARRAY_USER_DEFINED = vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED
  };
  using ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_STRING_ARRAY>;
  using DataTypeTag = std::integral_constant<int, VTK_STRING>;
  using ValueType = vtkStdString;

  static vtkStringArray* New();
  static vtkStringArray* ExtendedNew();
  vtkTypeMacro(vtkStringArray, vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkStringArray.
   * This method checks if source->GetArrayType() returns StringArray,
   * and performs a static_cast to return source as a vtkStringArray pointer.
   * Otherwise, nullptr is returned.
   */
  static vtkStringArray* FastDownCast(vtkAbstractArray* source);

  /**
   * Return the array type.
   */
  int GetArrayType() const override { return vtkStringArray::ArrayTypeTag::value; }

  /**
   * Get the data type.
   */
  int GetDataType() const override { return vtkStringArray::DataTypeTag::value; }

  int IsNumeric() const override { return 0; }

  /**
   * Release storage and reset array to initial state.
   */
  void Initialize() override;

  /**
   * Copy one component from \a src into a (potentially different) component
   * of this array. Note that \a src must be a vtkStringArray.
   */
  bool CopyComponent(int dstComponent, vtkAbstractArray* src, int srcComponent) override;

  /**
   * Return the size of the data type.  WARNING: This may not mean
   * what you expect with strings.  It will return
   * sizeof(std::string) and not take into account the data
   * included in any particular string.
   */
  int GetDataTypeSize() const override;

  /**
   * Free any unnecessary memory.
   * Resize object to just fit data requirement. Reclaims extra memory.
   */
  void Squeeze() override { this->Resize(this->GetNumberOfTuples()); }

  /**
   * Resize the array while conserving the data.
   */
  vtkTypeBool Resize(vtkIdType numTuples) override;

  /**
   * Set the tuple at the ith location using the jth tuple in the source array.
   * This method assumes that the two arrays have the same type
   * and structure. Note that range checking and memory allocation is not
   * performed; use in conjunction with SetNumberOfTuples() to allocate space.
   */
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;

  /**
   * Insert the jth tuple in the source array, at ith location in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;

  /**
   * Copy the tuples indexed in srcIds from the source array to the tuple
   * locations indexed by dstIds in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override;

  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override;

  /**
   * Copy n consecutive tuples starting at srcStart from the source array to
   * this array, starting at the dstStart location.
   * Note that memory allocation is performed as necessary to hold the data.
   */
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;

  /**
   * Insert the jth tuple in the source array, at the end in this array.
   * Note that memory allocation is performed as necessary to hold the data.
   * Returns the location at which the data was inserted.
   */
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) override;

  /**
   * Set the ith tuple in this array as the interpolated tuple value,
   * given the ptIndices in the source array and associated
   * interpolation weights.
   * This method assumes that the two arrays are of the same type
   * and structure.
   */
  void InterpolateTuple(
    vtkIdType i, vtkIdList* ptIndices, vtkAbstractArray* source, double* weights) override;

  /**
   * Insert the ith tuple in this array as interpolated from the two values,
   * p1 and p2, and an interpolation factor, t.
   * The interpolation factor ranges from (0,1),
   * with t=0 located at p1. This method assumes that the three arrays are of
   * the same type. p1 is value at index id1 in source1, while, p2 is
   * value at index id2 in source2.
   */
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray* source1, vtkIdType id2,
    vtkAbstractArray* source2, double t) override;

  /**
   * Given a list of indices, return an array of values.  You must
   * ensure that the output array has been previously allocated with
   * enough space to hold the data and that the types match
   * sufficiently to allow conversion (if necessary).
   */
  void GetTuples(vtkIdList* ptIds, vtkAbstractArray* output) override;

  /**
   * Get the values for the range of indices specified (i.e.,
   * p1->p2 inclusive). You must ensure that the output array has been
   * previously allocated with enough space to hold the data and that
   * the type of the output array is compatible with the type of this
   * array.
   */
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output) override;

  /**
   * Allocate memory for this array. Delete old storage only if necessary.
   * Note that ext is no longer used.
   */
  vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext = 1000) override;

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
   * Read-access of string at a particular index.
   */
  const ValueType& GetValue(vtkIdType id) const
    VTK_EXPECTS(0 <= id && id < this->GetNumberOfValues());

  /**
   * Get the string at a particular index.
   */
  ValueType& GetValue(vtkIdType id) VTK_EXPECTS(0 <= id && id < this->GetNumberOfValues());

  /**
   * Set the data at a particular index. Does not do range checking. Make sure
   * you use the method SetNumberOfValues() before inserting data.
   */
  void SetValue(vtkIdType id, ValueType value)
    VTK_EXPECTS(0 <= id && id < this->GetNumberOfValues())
  {
    this->Buffer->GetBuffer()[id] = value;
    this->DataChanged();
  }

  void SetValue(vtkIdType id, const char* value)
    VTK_EXPECTS(0 <= id && id < this->GetNumberOfValues()) VTK_EXPECTS(value != nullptr);

  /**
   * Set the number of tuples (a component group) in the array. Note that
   * this may allocate space depending on the number of components.
   */
  void SetNumberOfTuples(vtkIdType number) override
  {
    this->SetNumberOfValues(this->NumberOfComponents * number);
  }

  VTK_DEPRECATED_IN_9_7_0("No longer needed")
  int GetNumberOfElementComponents() { return 0; }
  int GetElementComponentSize() const override
  {
    return static_cast<int>(sizeof(ValueType::value_type));
  }

  /**
   * Insert data at a specified position in the array.
   */
  void InsertValue(vtkIdType id, ValueType f) VTK_EXPECTS(0 <= id);
  void InsertValue(vtkIdType id, const char* val) VTK_EXPECTS(0 <= id) VTK_EXPECTS(val != nullptr);

  /**
   * Get a value in the array as a variant.
   */
  vtkVariant GetVariantValue(vtkIdType idx) override;

  /**
   * Set a value in the array from a variant.
   */
  void SetVariantValue(vtkIdType idx, vtkVariant value) override;

  /**
   * Safely set a value in the array from a variant.
   * Safely insert a value into the array from a variant.
   */
  void InsertVariantValue(vtkIdType idx, vtkVariant value) override;

  /**
   * Insert data at the end of the array. Return its location in the array.
   */
  vtkIdType InsertNextValue(ValueType f);
  vtkIdType InsertNextValue(const char* f) VTK_EXPECTS(f != nullptr);

  /**
   * Get the address of a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  ValueType* WritePointer(vtkIdType id, vtkIdType number);

  /**
   * Get the address of a particular data index. Performs no checks
   * to verify that the memory has been allocated etc.
   */
  ValueType* GetPointer(vtkIdType id) { return this->Buffer->GetBuffer() + id; }
  void* GetVoidPointer(vtkIdType id) override { return this->GetPointer(id); }

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
   * Deep copy of another string array.  Will complain and change nothing
   * if the array passed in is not a vtkStringArray.
   */
  void DeepCopy(vtkAbstractArray* aa) override;

  /**
   * This method will copy the data from the source array to this array.
   * This is possible only if the data types are the same data type and array type.
   * Otherwise, it will complain and change nothing if the array passed in is not the same.
   */
  void ShallowCopy(vtkAbstractArray* src) override;

  /**
   * This method lets the user specify data to be held by the array.  The
   * array argument is a pointer to the data.  size is the size of
   * the array supplied by the user.  Set save to 1 to keep the class
   * from deleting the array when it cleans up or reallocates memory.
   * The class uses the actual array provided; it does not copy the data
   * from the supplied array. If save is 0, then this class is free to delete
   * the array when it cleans up or reallocates using the provided free function
   * If the delete method is VTK_DATA_ARRAY_USER_DEFINED
   * a custom free function can be assigned to be called using SetArrayFreeFunction,
   * if no custom function is assigned we will default to delete[].
   */
  void SetArray(
    ValueType* array, vtkIdType size, int save, int deleteMethod = VTK_DATA_ARRAY_DELETE);
  void SetVoidArray(void* array, vtkIdType size, int save) override
  {
    this->SetArray(static_cast<ValueType*>(array), size, save);
  }
  void SetVoidArray(void* array, vtkIdType size, int save, int deleteMethod) override
  {
    this->SetArray(static_cast<ValueType*>(array), size, save, deleteMethod);
  }

  /**
   * This method allows the user to specify a custom free function to be
   * called when the array is deallocated. Calling this method will implicitly
   * mean that the given free function will be called when the class
   * cleans up or reallocates memory.
   **/
  void SetArrayFreeFunction(void (*callback)(void*)) override;

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this data array. Used to
   * support streaming and reading/writing data. The value returned is
   * guaranteed to be greater than or equal to the memory required to
   * actually represent the data represented by this object. The
   * information returned is valid only after the pipeline has
   * been updated.

   * This function takes into account the size of the contents of the
   * strings as well as the string containers themselves.
   */
  unsigned long GetActualMemorySize() const override;

  /**
   * Returns a vtkArrayIteratorTemplate<vtkStdString>.
   */
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;

  /**
   * Returns the size of the data in DataTypeSize units. Thus, the number of bytes
   * for the data can be computed by GetDataSize() * GetDataTypeSize().
   * The size computation includes the string termination character for each string.
   */
  vtkIdType GetDataSize() const override;

  ///@{
  /**
   * Return the indices where a specific value appears.
   */
  vtkIdType LookupValue(vtkVariant value) override;
  void LookupValue(vtkVariant value, vtkIdList* ids) override;
  ///@}

  vtkIdType LookupValue(const ValueType& value);
  void LookupValue(const ValueType& value, vtkIdList* ids);

  vtkIdType LookupValue(const char* value);
  void LookupValue(const char* value, vtkIdList* ids);

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
   * Tell the array explicitly that a single data element has
   * changed. Like DataChanged(), then is only necessary when you
   * modify the array contents without using the array's API.
   */
  VTK_DEPRECATED_IN_9_7_0("Use DataChanged() instead")
  virtual void DataElementChanged(vtkIdType id);

  /**
   * Delete the associated fast lookup data structure on this array,
   * if it exists.  The lookup will be rebuilt on the next call to a lookup
   * function.
   */
  void ClearLookup() override;

protected:
  vtkStringArray();
  ~vtkStringArray() override;

  /**
   * Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
   * all data is freed.
   */
  bool AllocateTuples(vtkIdType numTuples);

  /**
   * Allocate space for numTuples. Old data is preserved. If numTuples == 0, all data is freed.
   */
  bool ReallocateTuples(vtkIdType numTuples);

  /**
   * This method resizes the array if needed so that the given tuple index is valid/accessible.
   */
  bool EnsureAccessToTuple(vtkIdType tupleIdx);

  vtkBuffer<ValueType>* Buffer;

  /**
   * Function to resize data
   */
  VTK_DEPRECATED_IN_9_7_0("Use Resize")
  ValueType* ResizeAndExtend(vtkIdType sz)
  {
    this->Resize(sz);
    return this->Buffer->GetBuffer();
  }

private:
  vtkStringArray(const vtkStringArray&) = delete;
  void operator=(const vtkStringArray&) = delete;

  vtkStringArrayLookup* Lookup;
  void UpdateLookup();
};

// Declare vtkArrayDownCast implementations for vtkStringArray:
vtkArrayDownCast_FastCastMacro(vtkStringArray);

VTK_ABI_NAMESPACE_END
#endif
