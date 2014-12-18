/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitArrayIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBitArrayIterator - Iterator for vtkBitArray.
// This iterator iterates over a vtkBitArray. It uses the double interface
// to get/set bit values.

#ifndef vtkBitArrayIterator_h
#define vtkBitArrayIterator_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkArrayIterator.h"

class vtkBitArray;
class VTKCOMMONCORE_EXPORT vtkBitArrayIterator : public vtkArrayIterator
{
public:
  static vtkBitArrayIterator* New();
  vtkTypeMacro(vtkBitArrayIterator, vtkArrayIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the array this iterator will iterate over.
  // After Initialize() has been called, the iterator is valid
  // so long as the Array has not been modified
  // (except using the iterator itself).
  // If the array is modified, the iterator must be re-intialized.
  virtual void Initialize(vtkAbstractArray* array);

  // Description:
  // Get the array.
  vtkAbstractArray* GetArray();

  // Description:
  // Must be called only after Initialize.
  int* GetTuple(vtkIdType id) ;

  // Description:
  // Must be called only after Initialize.
  int GetValue(vtkIdType id);

  // Description:
  // Must be called only after Initialize.
  vtkIdType GetNumberOfTuples();

  // Description:
  // Must be called only after Initialize.
  vtkIdType GetNumberOfValues();

  // Description:
  // Must be called only after Initialize.
  int GetNumberOfComponents();

  // Description:
  // Get the data type from the underlying array.
  int GetDataType();

  // Description:
  // Get the data type size from the underlying array.
  int GetDataTypeSize();

  // Description:
  // Sets the value at the index. This does not verify if the index is valid.
  // The caller must ensure that id is less than the maximum number of values.
  void SetValue(vtkIdType id, int value);

  //BTX
  // Description:
  // Data type of a value.
  typedef int ValueType;
  //ETX
protected:
  vtkBitArrayIterator();
  ~vtkBitArrayIterator();

  int *Tuple;
  int TupleSize;
  void SetArray(vtkBitArray* b);
  vtkBitArray* Array;
private:
  vtkBitArrayIterator(const vtkBitArrayIterator&); // Not implemented.
  void operator=(const vtkBitArrayIterator&); // Not implemented.
};

#endif

