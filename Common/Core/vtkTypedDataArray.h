/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTypedDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTypedDataArray - Extend vtkDataArray with abstract type-specific API
//
// .SECTION Description
// This templated class decorates vtkDataArray with additional type-specific
// methods that can be used to interact with the data.
//
// .SECTION Caveats
// This class uses vtkTypeTraits to implement GetDataType(). Since vtkIdType
// is a typedef for either a 32- or 64-bit integer, subclasses that are designed
// to hold vtkIdTypes will, by default, return an incorrect value from
// GetDataType(). To fix this, such subclasses should override GetDataType() to
// return VTK_ID_TYPE.

#ifndef __vtkTypedDataArray_h
#define __vtkTypedDataArray_h

#include "vtkDataArray.h"

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkTypeTemplate.h" // For vtkTypeTemplate
#include "vtkTypeTraits.h"   // For type metadata

template <class Scalar> class vtkTypedDataArrayIterator;

template <class Scalar>
class vtkTypedDataArray :
    public vtkTypeTemplate<vtkTypedDataArray<Scalar>, vtkDataArray>
{
public:
  // Description:
  // Typedef to get the type of value stored in the array.
  typedef Scalar ValueType;

  // Description:
  // Typedef to a suitable iterator class.
  // Rather than using this member directly, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency.
  typedef vtkTypedDataArrayIterator<ValueType> Iterator;

  // Description:
  // Return an iterator initialized to the first element of the data.
  // Rather than using this member directly, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency.
  Iterator Begin();

  // Description:
  // Return an iterator initialized to first element past the end of the data.
  // Rather than using this member directly, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency.
  Iterator End();

  // Description:
  // Compile time access to the VTK type identifier.
  enum { VTK_DATA_TYPE = vtkTypeTraits<ValueType>::VTK_TYPE_ID };

  // Description:
  // Perform a fast, safe cast from a vtkAbstractArray to a vtkTypedDataArray.
  // This method checks if:
  // - source->GetArrayType() is appropriate, and
  // - source->GetDataType() matches the Scalar template argument
  // if these conditions are met, the method performs a static_cast to return
  // source as a vtkTypedDataArray pointer. Otherwise, NULL is returned.
  static vtkTypedDataArray<Scalar>* FastDownCast(vtkAbstractArray *source);

  // Description:
  // Return the VTK data type held by this array.
  int GetDataType();

  // Description:
  // Return the size of the element type in bytes.
  int GetDataTypeSize();

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  virtual void SetNumberOfValues(vtkIdType num);

  // Description:
  // Set the tuple value at the ith location in the array.
  virtual void SetTupleValue(vtkIdType i, const ValueType *t) = 0;

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  virtual void InsertTupleValue(vtkIdType i, const ValueType *t) = 0;

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  virtual vtkIdType InsertNextTupleValue(const ValueType *t) = 0;

  // Description:
  // Return the indices where a specific value appears.
  virtual vtkIdType LookupTypedValue(ValueType value) = 0;
  virtual void LookupTypedValue(ValueType value, vtkIdList *ids) = 0;

  // Description:
  // Get the data at a particular index.
  virtual ValueType GetValue(vtkIdType idx) = 0;

  // Description:
  // Get a reference to the scalar value at a particular index.
  virtual ValueType& GetValueReference(vtkIdType idx) = 0;

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  virtual void SetValue(vtkIdType idx, ValueType value) = 0;

  // Description:
  // Copy the tuple value into a user-provided array.
  virtual void GetTupleValue(vtkIdType idx, ValueType *t) = 0;

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  virtual vtkIdType InsertNextValue(ValueType v) = 0;

  // Description:
  // Insert data at a specified position in the array.
  virtual void InsertValue(vtkIdType idx, ValueType v) = 0;

  // Description:
  // Method for type-checking in FastDownCast implementations.
  virtual int GetArrayType() { return vtkAbstractArray::TypedDataArray; }

protected:
  vtkTypedDataArray();
  ~vtkTypedDataArray();

private:
  vtkTypedDataArray(const vtkTypedDataArray &); // Not implemented.
  void operator=(const vtkTypedDataArray &);   // Not implemented.
};

// Included here to resolve chicken/egg issue with container/iterator:
#include "vtkTypedDataArrayIterator.h" // For iterator

template <class Scalar> inline
typename vtkTypedDataArray<Scalar>::Iterator vtkTypedDataArray<Scalar>::Begin()
{
  return Iterator(this, 0);
}

template <class Scalar> inline
typename vtkTypedDataArray<Scalar>::Iterator vtkTypedDataArray<Scalar>::End()
{
  return Iterator(this, this->MaxId + 1);
}

#include "vtkTypedDataArray.txx"

#endif //__vtkTypedDataArray_h

// VTK-HeaderTest-Exclude: vtkTypedDataArray.h
