/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataArrayTemplate - Implementation template for vtkDataArray.
// .SECTION Description
// There is a vtkDataArray subclass for each native type supported by
// VTK.  This template is used to implement all the subclasses in the
// same way while avoiding code duplication.

#ifndef __vtkDataArrayTemplate_h
#define __vtkDataArrayTemplate_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkTypedDataArray.h"
#include "vtkTypeTemplate.h" // For templated vtkObject API
#include <cassert> // for assert()

template <class T>
class vtkDataArrayTemplateLookup;

template <class T>
class VTKCOMMONCORE_EXPORT vtkDataArrayTemplate:
    public vtkTypeTemplate<vtkDataArrayTemplate<T>, vtkTypedDataArray<T> >
{
public:
  typedef vtkTypedDataArray<T> Superclass;
  typedef typename Superclass::ValueType ValueType;
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Typedef to a suitable iterator class.
  // Rather than using this member directly, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency.
  typedef ValueType* Iterator;

  // Description:
  // Return an iterator initialized to the first element of the data.
  // Rather than using this member directly, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency.
  Iterator Begin() { return Iterator(this->GetVoidPointer(0)); }

  // Description:
  // Return an iterator initialized to first element past the end of the data.
  // Rather than using this member directly, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency.
  Iterator End() { return Iterator(this->GetVoidPointer(this->MaxId + 1)); }

  // Description:
  // Perform a fast, safe cast from a vtkAbstractArray to a
  // vtkDataArrayTemplate.
  // This method checks if:
  // - source->GetArrayType() is appropriate, and
  // - source->GetDataType() matches vtkTypeTraits<ValueType>::VTK_TYPE_ID
  // if these conditions are met, the method performs a static_cast to return
  // source as a vtkTypedDataArray pointer. Otherwise, NULL is returned.
  static vtkDataArrayTemplate<T>* FastDownCast(vtkAbstractArray *src);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  int Allocate(vtkIdType sz, vtkIdType ext=1000);

  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // Description:
  // Return the size of the data type.
  int GetDataTypeSize() { return static_cast<int>(sizeof(T)); }

  // Description:
  // Set the number of n-tuples in the array.
  void SetNumberOfTuples(vtkIdType number);

  // Description:
  // Set the tuple at the ith location using the jth tuple in the source array.
  // This method assumes that the two arrays have the same type
  // and structure. Note that range checking and memory allocation is not
  // performed; use in conjunction with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);

  // Description:
  // Insert the jth tuple in the source array, at ith location in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source);

  // Description:
  // Copy the tuples indexed in srcIds from the source array to the tuple
  // locations indexed by dstIds in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertTuples(vtkIdList *destIds, vtkIdList *srcIds,
                            vtkAbstractArray *source);

  // Description:
  // Insert the jth tuple in the source array, at the end in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  // Returns the location at which the data was inserted.
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source);

  // Description:
  // Get a pointer to a tuple at the ith location. This is a dangerous method
  // (it is not thread safe since a pointer is returned).
  double* GetTuple(vtkIdType i);

  // Description:
  // Copy the tuple value into a user-provided array.
  void GetTuple(vtkIdType i, double* tuple);
  void GetTupleValue(vtkIdType i, T* tuple);

  // Description:
  // Set the tuple value at the ith location in the array.
  void SetTuple(vtkIdType i, const float* tuple);
  void SetTuple(vtkIdType i, const double* tuple);
  void SetTupleValue(vtkIdType i, const T* tuple);

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTuple(vtkIdType i, const float* tuple);
  void InsertTuple(vtkIdType i, const double* tuple);
  void InsertTupleValue(vtkIdType i, const T* tuple);

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  vtkIdType InsertNextTuple(const float* tuple);
  vtkIdType InsertNextTuple(const double* tuple);
  vtkIdType InsertNextTupleValue(const T* tuple);

  // Description:
  // Get the range of array values for the given component in the
  // native data type.
  void GetValueRange(T range[2], int comp)
    {
    double doubleRange[2];
    this->ComputeRange(doubleRange, comp);
    range[0] = static_cast<T>(doubleRange[0]);
    range[1] = static_cast<T>(doubleRange[1]);
    }
  T *GetValueRange(int comp)
    {
    this->GetValueRange(this->ValueRange, comp);
    return this->ValueRange;
    }

  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  void Squeeze() { this->ResizeAndExtend (this->MaxId+1); }

  // Description:
  // Return the capacity in typeof T units of the current array.
  vtkIdType Capacity() { return this->Size; }

  // Description:
  // Resize the array while conserving the data.
  // Caution: No assumption can be made on the resulting size of the DataArray,
  //          meaning that the provided argument won't necessary be equal to
  //          the data array size, but at least the size will be bigger.
  virtual int Resize(vtkIdType numTuples);

  // Description:
  // Get the data at a particular index.
  T GetValue(vtkIdType id)
    { assert(id >= 0 && id < this->Size); return this->Array[id]; }
  T& GetValueReference(vtkIdType id)
    { assert(id >= 0 && id < this->Size); return this->Array[id]; }

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(vtkIdType id, T value)
    { assert(id >= 0 && id < this->Size); this->Array[id] = value;};

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(vtkIdType number);

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType id, T f);

  // Description:
  // Set a value in the array from a vtkVariant.
  void SetVariantValue(vtkIdType id, vtkVariant value);

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  vtkIdType InsertNextValue(T f);

  // Description:
  // These methods remove tuples from the data array. They shift data and
  // resize array, so the data array is still valid after this operation. Note,
  // this operation is fairly slow.
  virtual void RemoveTuple(vtkIdType id);
  virtual void RemoveFirstTuple();
  virtual void RemoveLastTuple();

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i is less then NumberOfTuples and j is less then
  // NumberOfComponents.
  double GetComponent(vtkIdType i, int j);

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less then NumberOfTuples and j is less then
  // NumberOfComponents. Make sure enough memory has been allocated
  // (use SetNumberOfTuples() and SetNumberOfComponents()).
  void SetComponent(vtkIdType i, int j, double c);

  // Description:
  // Insert the data component at ith tuple and jth component location.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertComponent(vtkIdType i, int j, double c);

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  T* WritePointer(vtkIdType id, vtkIdType number);
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number)
    { return this->WritePointer(id, number); }

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  // If the data is simply being iterated over, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency, rather than using this
  // member directly.
  T* GetPointer(vtkIdType id) { return this->Array + id; }
  virtual void* GetVoidPointer(vtkIdType id) { return this->GetPointer(id); }

  // Description:
  // Deep copy of another double array.
  void DeepCopy(vtkDataArray* da);
  void DeepCopy(vtkAbstractArray* aa)
    { this->Superclass::DeepCopy(aa); }

//BTX
  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE
  };
//ETX

  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of the
  // array supplied by the user.  Set save to 1 to keep the class from
  // deleting the array when it cleans up or reallocates memory.  The class
  // uses the actual array provided; it does not copy the data from the
  // suppled array. If specified, the delete method determines how the data
  // array will be deallocated. If the delete method is
  // VTK_DATA_ARRAY_FREE, free() will be used. If the delete method is
  // DELETE, delete[] will be used. The default is FREE.
  void SetArray(T* array, vtkIdType size, int save, int deleteMethod);
  void SetArray(T* array, vtkIdType size, int save)
    { this->SetArray(array, size, save, VTK_DATA_ARRAY_FREE); }
  virtual void SetVoidArray(void* array, vtkIdType size, int save)
    { this->SetArray(static_cast<T*>(array), size, save); }
  virtual void SetVoidArray(void* array,
                            vtkIdType size,
                            int save,
                            int deleteMethod)
    {
      this->SetArray(static_cast<T*>(array), size, save, deleteMethod);
    }

  // Description:
  // This method copies the array data to the void pointer specified
  // by the user.  It is up to the user to allocate enough memory for
  // the void pointer.
  virtual void ExportToVoidPointer(void *out_ptr);

  // Description:
  // Returns a vtkArrayIteratorTemplate<T>.
  virtual vtkArrayIterator* NewIterator();

  // Description:
  // Return the indices where a specific value appears.
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual void LookupValue(vtkVariant value, vtkIdList* ids);
  vtkIdType LookupValue(T value);
  void LookupValue(T value, vtkIdList* ids);
  vtkIdType LookupTypedValue(T value)
    { return this->LookupValue(value); }
  void LookupTypedValue(T value, vtkIdList* ids)
    { this->LookupValue(value, ids); }

  // Description:
  // Tell the array explicitly that the data has changed.
  // This is only necessary to call when you modify the array contents
  // without using the array's API (i.e. you retrieve a pointer to the
  // data and modify the array contents).  You need to call this so that
  // the fast lookup will know to rebuild itself.  Otherwise, the lookup
  // functions will give incorrect results.
  virtual void DataChanged();

  // Description:
  // Tell the array explicitly that a single data element has
  // changed. Like DataChanged(), then is only necessary when you
  // modify the array contents without using the array's API.
  virtual void DataElementChanged(vtkIdType id);

  // Description:
  // Delete the associated fast lookup data structure on this array,
  // if it exists.  The lookup will be rebuilt on the next call to a lookup
  // function.
  virtual void ClearLookup();

  // Description:
  // Method for type-checking in FastDownCast implementations.
  virtual int GetArrayType() { return vtkAbstractArray::DataArrayTemplate; }

protected:
  vtkDataArrayTemplate();
  ~vtkDataArrayTemplate();

  T* Array;   // pointer to data
  T ValueRange[2]; // range of the data
  T* ResizeAndExtend(vtkIdType sz);  // function to resize data
  T* Realloc(vtkIdType sz);

  int TupleSize; //used for data conversion
  double* Tuple;

  int SaveUserArray;
  int DeleteMethod;

  virtual void ComputeScalarRange(double range[2], int comp);
  virtual void ComputeVectorRange(double range[2]);
private:
  vtkDataArrayTemplate(const vtkDataArrayTemplate&);  // Not implemented.
  void operator=(const vtkDataArrayTemplate&);  // Not implemented.

  vtkDataArrayTemplateLookup<T>* Lookup;
  bool RebuildLookup;
  void UpdateLookup();

  void DeleteArray();
};

#if !defined(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
# define VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(T) \
   template class VTKCOMMONCORE_EXPORT vtkDataArrayTemplate< T >
#else
# include "vtkDataArrayTemplateImplicit.txx"
# define VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(T)
#endif

#endif // !defined(__vtkDataArrayTemplate_h)

// This portion must be OUTSIDE the include blockers.  Each
// vtkDataArray subclass uses this to give its instantiation of this
// template a DLL interface.
#if defined(VTK_DATA_ARRAY_TEMPLATE_TYPE)
# if defined(VTK_BUILD_SHARED_LIBS) && defined(_MSC_VER)
#  pragma warning (push)
#  pragma warning (disable: 4091) // warning C4091: 'extern ' :
   // ignored on left of 'int' when no variable is declared
#  pragma warning (disable: 4231) // Compiler-specific extension warning.

   // We need to disable warning 4910 and do an extern dllexport
   // anyway.  When deriving vtkCharArray and other types from an
   // instantiation of this template the compiler does an explicit
   // instantiation of the base class.  From outside the vtkCommon
   // library we block this using an extern dllimport instantiation.
   // For classes inside vtkCommon we should be able to just do an
   // extern instantiation, but VS 2008 complains about missing
   // definitions.  We cannot do an extern dllimport inside vtkCommon
   // since the symbols are local to the dll.  An extern dllexport
   // seems to be the only way to convince VS 2008 to do the right
   // thing, so we just disable the warning.
#  pragma warning (disable: 4910) // extern and dllexport incompatible

   // Use an "extern explicit instantiation" to give the class a DLL
   // interface.  This is a compiler-specific extension.
   extern VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(VTK_DATA_ARRAY_TEMPLATE_TYPE);
#  pragma warning (pop)
# endif
# undef VTK_DATA_ARRAY_TEMPLATE_TYPE
#endif
// VTK-HeaderTest-Exclude: vtkDataArrayTemplate.h
