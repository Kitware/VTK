/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBitArray - dynamic, self-adjusting array of bits
// .SECTION Description
// vtkBitArray is an array of bits (0/1 data value). The array is packed
// so that each byte stores eight bits. vtkBitArray provides methods
// for insertion and retrieval of bits, and will automatically resize
// itself to hold new data.

#ifndef __vtkBitArray_h
#define __vtkBitArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"

class vtkBitArrayLookup;

class VTKCOMMONCORE_EXPORT vtkBitArray : public vtkDataArray
{
public:
  static vtkBitArray *New();
  vtkTypeMacro(vtkBitArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  int Allocate(vtkIdType sz, vtkIdType ext=1000);

  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // satisfy vtkDataArray API
  int GetDataType() {return VTK_BIT;};
  int GetDataTypeSize() { return 0; }

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
  virtual void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                            vtkAbstractArray *source);

  // Description:
  // Insert the jth tuple in the source array, at the end in this array.
  // Note that memory allocation is performed as necessary to hold the data.
  // Returns the location at which the data was inserted.
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source);

  // Description:
  // Get a pointer to a tuple at the ith location. This is a dangerous method
  // (it is not thread safe since a pointer is returned).
  double *GetTuple(vtkIdType i);

  // Description:
  // Copy the tuple value into a user-provided array.
  void GetTuple(vtkIdType i, double * tuple);

  // Description:
  // Set the tuple value at the ith location in the array.
  void SetTuple(vtkIdType i, const float * tuple);
  void SetTuple(vtkIdType i, const double * tuple);

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTuple(vtkIdType i, const float * tuple);
  void InsertTuple(vtkIdType i, const double * tuple);

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  vtkIdType InsertNextTuple(const float * tuple);
  vtkIdType InsertNextTuple(const double * tuple);

  // Description:
  // These methods remove tuples from the data array. They shift data and
  // resize array, so the data array is still valid after this operation. Note,
  // this operation is fairly slow.
  virtual void RemoveTuple(vtkIdType id);
  virtual void RemoveFirstTuple();
  virtual void RemoveLastTuple();

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less then NumberOfTuples and j is less then
  // NumberOfComponents. Make sure enough memory has been allocated (use
  // SetNumberOfTuples() and  SetNumberOfComponents()).
  void SetComponent(vtkIdType i, int j, double c);

  // Description:
  // Free any unneeded memory.
  void Squeeze();

  // Description:
  // Resize the array while conserving the data.
  virtual int Resize(vtkIdType numTuples);

  // Description:
  // Get the data at a particular index.
  int GetValue(vtkIdType id);

  // Description:
  // Fast method based setting of values without memory checks. First
  // use SetNumberOfValues then use SetValue to actually set them.
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(vtkIdType number);

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(vtkIdType id, int value);

  // Description:
  // Insets values and checks to make sure there is enough memory
  void InsertValue(vtkIdType id, int i);

  // Description:
  // Set a value in the array from a variant.
  void SetVariantValue(vtkIdType idx, vtkVariant value);

  vtkIdType InsertNextValue(int i);

  // Description:
  // Insert the data component at ith tuple and jth component location.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertComponent(vtkIdType i, int j, double c);

  // Description:
  // Direct manipulation of the underlying data.
  unsigned char *GetPointer(vtkIdType id) {return this->Array + id/8;}

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  unsigned char *WritePointer(vtkIdType id, vtkIdType number);
  void* WriteVoidPointer(vtkIdType id, vtkIdType number)
    { return this->WritePointer(id, number); }
  void *GetVoidPointer(vtkIdType id)
    {
      return static_cast<void *>(this->GetPointer(id));
    }

  // Description:
  // Deep copy of another bit array.
  void DeepCopy(vtkDataArray *da);
  void DeepCopy(vtkAbstractArray* aa)
    { this->Superclass::DeepCopy(aa); }

  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data
  // from the supplied array. If save 0, the array must have been allocated
  // with new[] not malloc.
#ifndef __WRAP__
  void SetArray(unsigned char* array, vtkIdType size, int save);
#endif
  void SetVoidArray(void *array, vtkIdType size, int save)
    {
      this->SetArray(static_cast<unsigned char *>(array), size, save);
    }

  // Description:
  // Returns a new vtkBitArrayIterator instance.
  vtkArrayIterator* NewIterator();

  // Description:
  // Return the indices where a specific value appears.
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual void LookupValue(vtkVariant value, vtkIdList* ids);
  vtkIdType LookupValue(int value);
  void LookupValue(int value, vtkIdList* ids);

  // Description:
  // Tell the array explicitly that the data has changed.
  // This is only necessary to call when you modify the array contents
  // without using the array's API (i.e. you retrieve a pointer to the
  // data and modify the array contents).  You need to call this so that
  // the fast lookup will know to rebuild itself.  Otherwise, the lookup
  // functions will give incorrect results.
  virtual void DataChanged();

  // Description:
  // Delete the associated fast lookup data structure on this array,
  // if it exists.  The lookup will be rebuilt on the next call to a lookup
  // function.
  virtual void ClearLookup();

protected:
  vtkBitArray();
  ~vtkBitArray();

  unsigned char *Array;   // pointer to data
  unsigned char *ResizeAndExtend(vtkIdType sz);
    // function to resize data

  int TupleSize; //used for data conversion
  double *Tuple;

  int SaveUserArray;

private:
  // hide superclass' DeepCopy() from the user and the compiler
  void DeepCopy(vtkDataArray &da) {this->vtkDataArray::DeepCopy(&da);}

private:
  vtkBitArray(const vtkBitArray&);  // Not implemented.
  void operator=(const vtkBitArray&);  // Not implemented.

  //BTX
  vtkBitArrayLookup* Lookup;
  void UpdateLookup();
  //ETX
};

inline void vtkBitArray::SetNumberOfValues(vtkIdType number)
{
  this->Allocate(number);
  this->MaxId = number - 1;
  this->DataChanged();
}

inline void vtkBitArray::SetValue(vtkIdType id, int value)
{
  if (value)
    {
    this->Array[id/8] = static_cast<unsigned char>(
      this->Array[id/8] | (0x80 >> id%8));
    }
  else
    {
    this->Array[id/8] = static_cast<unsigned char>(
      this->Array[id/8] & (~(0x80 >> id%8)));
    }
  this->DataChanged();
}

inline void vtkBitArray::InsertValue(vtkIdType id, int i)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  if (i)
    {
    this->Array[id/8] = static_cast<unsigned char>(
      this->Array[id/8] | (0x80 >> id%8));
    }
  else
    {
    this->Array[id/8] = static_cast<unsigned char>(
      this->Array[id/8] & (~(0x80 >> id%8)));
    }
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
  this->DataChanged();
}

inline void vtkBitArray::SetVariantValue(vtkIdType id, vtkVariant value)
{
  this->SetValue(id, value.ToInt());
}

inline vtkIdType vtkBitArray::InsertNextValue(int i)
{
  this->InsertValue (++this->MaxId,i);
  this->DataChanged();
  return this->MaxId;
}

inline void vtkBitArray::Squeeze() {this->ResizeAndExtend (this->MaxId+1);}

#endif

