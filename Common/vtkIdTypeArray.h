/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdTypeArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIdTypeArray - dynamic, self-adjusting integer array
// .SECTION Description
// vtkIdTypeArray is an array of integer numbers. It provides methods
// for insertion and retrieval of integer values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkIdTypeArray_h
#define __vtkIdTypeArray_h

#include "vtkDataArray.h"

class VTK_COMMON_EXPORT vtkIdTypeArray : public vtkDataArray
{
public:
  static vtkIdTypeArray *New();

  vtkTypeRevisionMacro(vtkIdTypeArray, vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  int Allocate(vtkIdType sz, vtkIdType ext=1000);
  
  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // Description:
  // Get the data type.
  int GetDataType() 
    {return VTK_ID_TYPE;}
  
  // Description:
  // Return the size of the data type.
  int GetDataTypeSize() { return sizeof(vtkIdType); }

  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  void Squeeze() 
    {this->ResizeAndExtend (this->MaxId+1);}

  // Description:
  // Resize the array while conserving the data.
  virtual void Resize(vtkIdType numTuples);

  // Description:
  // Set the number of n-tuples in the array.
  void SetNumberOfTuples(vtkIdType number);

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
  // Get the data at a particular index.
  vtkIdType GetValue(vtkIdType id) 
    {return this->Array[id];}

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(vtkIdType id, vtkIdType value) 
    {this->Array[id] = value;}

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(vtkIdType number);

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType id, vtkIdType i);

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  vtkIdType InsertNextValue(vtkIdType i);

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  vtkIdType *GetPointer(vtkIdType id) 
    {return this->Array + id;}
  void *GetVoidPointer(vtkIdType id) 
    {return (void *)this->GetPointer(id);}

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  vtkIdType *WritePointer(vtkIdType id, vtkIdType number);

  // Description:
  // Deep copy of another integer array.
  void DeepCopy(vtkDataArray *ia);

  // Description:
  // This method lets the user specify data to be held by the array.  The 
  // array argument is a pointer to the data.  size is the size of 
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data 
  // from the suppled array.
  void SetArray(vtkIdType* array, vtkIdType size, int save);
  void SetVoidArray(void *array, vtkIdType size, int save) 
    {this->SetArray((vtkIdType*)array, size, save);};

protected:
  vtkIdTypeArray(vtkIdType numComp=1);
  ~vtkIdTypeArray();

  vtkIdType *Array;   // pointer to data
  vtkIdType *ResizeAndExtend(vtkIdType sz);  // function to resize data

  int TupleSize; //used for data conversion
  double *Tuple;

  int SaveUserArray;
private:
  vtkIdTypeArray(const vtkIdTypeArray&);  // Not implemented.
  void operator=(const vtkIdTypeArray&);  // Not implemented.
};


inline void vtkIdTypeArray::SetNumberOfValues(vtkIdType number) 
{
  this->Allocate(number);
  this->MaxId = number - 1;
}

inline vtkIdType *vtkIdTypeArray::WritePointer(vtkIdType id,
                                               vtkIdType number)
{
  vtkIdType newSize=id+number;
  if ( newSize > this->Size )
    {
    this->ResizeAndExtend(newSize);
    }
  if ( (--newSize) > this->MaxId )
    {
    this->MaxId = newSize;
    }
  return this->Array + id;
}

inline void vtkIdTypeArray::InsertValue(vtkIdType id, vtkIdType i)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = i;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
}

inline vtkIdType vtkIdTypeArray::InsertNextValue(vtkIdType i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}


#endif
