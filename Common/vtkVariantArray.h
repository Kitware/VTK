/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkVariantArray - An array holding vtkVariants.
//
// .SECTION Description
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class.


#ifndef __vtkVariantArray_h
#define __vtkVariantArray_h

#include "vtkAbstractArray.h"
#include "vtkVariant.h" // For variant type

class vtkVariantArrayLookup;

//BTX
/// Forward declaration required for Boost serialization
namespace boost { namespace serialization { class access; } }
//ETX

class VTK_COMMON_EXPORT vtkVariantArray : public vtkAbstractArray
{
//BTX
  /// Friendship required for Boost serialization
  friend class boost::serialization::access;
//ETX

public:
  static vtkVariantArray* New();
  vtkTypeMacro(vtkVariantArray,vtkAbstractArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // 
  // Functions required by vtkAbstractArray
  //

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(vtkIdType sz, vtkIdType ext=1000);

  // Description:
  // Release storage and reset array to initial state.
  virtual void Initialize();

  // Description:
  // Return the underlying data type. An integer indicating data type is 
  // returned as specified in vtkSetGet.h.
  virtual int GetDataType();

  // Description:
  // Return the size of the underlying data type.  For a bit, 1 is
  // returned.  For string 0 is returned. Arrays with variable length
  // components return 0.
  virtual int GetDataTypeSize();

  // Description:
  // Return the size, in bytes, of the lowest-level element of an
  // array.  For vtkDataArray and subclasses this is the size of the
  // data type.  For vtkStringArray, this is
  // sizeof(vtkStdString::value_type), which winds up being
  // sizeof(char).  
  virtual int GetElementComponentSize();

  // Description:
  // Set the number of tuples (a component group) in the array. Note that 
  // this may allocate space depending on the number of components.
  virtual void SetNumberOfTuples(vtkIdType number);

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
  // Insert the jth tuple in the source array, at the end in this array. 
  // Note that memory allocation is performed as necessary to hold the data.
  // Returns the location at which the data was inserted.
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source);
  
  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  virtual void *GetVoidPointer(vtkIdType id);

  // Description:
  // Deep copy of data. Implementation left to subclasses, which
  // should support as many type conversions as possible given the
  // data type.
  virtual void DeepCopy(vtkAbstractArray *da);

  // Description:
  // Set the ith tuple in this array as the interpolated tuple value,
  // given the ptIndices in the source array and associated 
  // interpolation weights.
  // This method assumes that the two arrays are of the same type
  // and strcuture.
  virtual void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
    vtkAbstractArray* source,  double* weights);

  // Description
  // Insert the ith tuple in this array as interpolated from the two values, 
  // p1 and p2, and an interpolation factor, t. 
  // The interpolation factor ranges from (0,1), 
  // with t=0 located at p1. This method assumes that the three arrays are of 
  // the same type. p1 is value at index id1 in source1, while, p2 is
  // value at index id2 in source2.
  virtual void InterpolateTuple(vtkIdType i, 
    vtkIdType id1, vtkAbstractArray* source1, 
    vtkIdType id2, vtkAbstractArray* source2, double t);
    
  // Description:
  // Free any unnecessary memory.
  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  virtual void Squeeze(); 

  // Description:
  // Resize the array while conserving the data.  Returns 1 if
  // resizing succeeded and 0 otherwise.
  virtual int Resize(vtkIdType numTuples);

  // Description:
  // This method lets the user specify data to be held by the array.  The 
  // array argument is a pointer to the data.  size is the size of 
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data 
  // from the supplied array.
  virtual void SetVoidArray(void *arr,
                            vtkIdType size,
                            int save);

  // Description:
  // Return the memory in kilobytes consumed by this data array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The 
  // information returned is valid only after the pipeline has 
  // been updated.
  virtual unsigned long GetActualMemorySize();
  
  // Description:
  // Since each item can be of a different type, we say that a variant array is not numeric.
  virtual int IsNumeric();

  // Description:
  // Subclasses must override this method and provide the right 
  // kind of templated vtkArrayIteratorTemplate.
  virtual vtkArrayIterator* NewIterator();

  //
  // Additional functions
  //

  //BTX
  // Description:
  // Get the data at a particular index.
  vtkVariant & GetValue(vtkIdType id) const;

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(vtkIdType id, vtkVariant value);

  // Description:
  // If id < GetNumberOfValues(), overwrite the array at that index.
  // If id >= GetNumberOfValues(), expand the array size to id+1
  // and set the final value to the specified value.
  void InsertValue(vtkIdType id, vtkVariant value);

  // Description:
  // Insert a value into the array from a variant.
  void InsertVariantValue(vtkIdType idx, vtkVariant value);

  // Description:
  // Expand the array by one and set the value at that location.
  // Return the array index of the inserted value.
  vtkIdType InsertNextValue(vtkVariant value);

  // Description:
  // Return a pointer to the location in the internal array at the specified index.
  vtkVariant* GetPointer(vtkIdType id);

  // Description:
  // Set the internal array used by this object.
  void SetArray(vtkVariant* arr, vtkIdType size, int save);
  //ETX

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(vtkIdType number);

  // Description:
  // Return the number of values in the array.
  vtkIdType GetNumberOfValues() { return this->MaxId + 1; }

  //BTX
  // Description:
  // Return the indices where a specific value appears.
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual void LookupValue(vtkVariant value, vtkIdList* ids);
  //ETX
  
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
  // This destructor is public to work around a bug in version 1.36.0 of
  // the Boost.Serialization library.
  ~vtkVariantArray();
  
protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkVariantArray(vtkIdType numComp=1);

  // Pointer to data
  //BTX
  vtkVariant* Array;

  // Function to resize data
  vtkVariant* ResizeAndExtend(vtkIdType sz);
  //ETX

  int SaveUserArray;

private:
  vtkVariantArray(const vtkVariantArray&);  // Not implemented.
  void operator=(const vtkVariantArray&);  // Not implemented.

  //BTX
  vtkVariantArrayLookup* Lookup;
  void UpdateLookup();
  //ETX
};

#endif
