/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IntArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkIntArray - dynamic, self adjusting integer array
// .SECTION Description
// vtkIntArray is an array of integer numbers. It provides methods
// for insertion and retrieval of integer values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkIntArray_h
#define __vtkIntArray_h

#include "Object.hh"

class vtkIntArray : public vtkObject 
{
public:
  vtkIntArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vtkIntArray(const int sz, const int ext=1000);
  vtkIntArray(const vtkIntArray& ia);
  ~vtkIntArray();
  virtual char *GetClassName() {return "vtkIntArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // access/insertion methods
  int GetValue(const int id);
  vtkIntArray &InsertValue(const int id, const int i);
  int InsertNextValue(const int i);
  int *GetPtr(const int id);
  int *WritePtr(const int id, const int number);

  // special operators
  vtkIntArray &operator=(const vtkIntArray& ia);
  void operator+=(const vtkIntArray& ia);
  void operator+=(const int i);
  int& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void Reset();

private:
  int *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  int *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline int vtkIntArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline int *vtkIntArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline int *vtkIntArray::WritePtr(const int id, const int number)
{
  if ( (id + number) > this->Size ) this->Resize(id+number);
  this->MaxId = id + number - 1;
  return this->Array + id;
}

// Description:
// Insert data at a specified position in the array.
inline vtkIntArray& vtkIntArray::InsertValue(const int id, const int i)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = i;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vtkIntArray::InsertNextValue(const int i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}
inline void vtkIntArray::operator+=(const int i) 
{
  this->InsertNextValue(i);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline int& vtkIntArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vtkIntArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vtkIntArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vtkIntArray::GetMaxId() {return this->MaxId;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vtkIntArray::Reset() {this->MaxId = -1;};

#endif
