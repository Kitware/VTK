/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCharArray - dynamic, self adjusting unssigned character array
// .SECTION Description
// vtkCharArray is an array of unsigned character values. It provides methods
// for insertion and retrieval of characters, and will automatically 
// resize itself to hold new data.

#ifndef __vtkCharArray_h
#define __vtkCharArray_h

#include "Object.hh"

class vtkCharArray : public vtkObject 
{
public:
  vtkCharArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vtkCharArray(const int sz, const int ext=1000);
  vtkCharArray(const vtkCharArray& ia);
  ~vtkCharArray();
  virtual char *GetClassName() {return "vtkCharArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // access/insertion methods
  unsigned char GetValue(const int id);
  vtkCharArray &InsertValue(const int id, const unsigned char c);
  int InsertNextValue(const unsigned char c);
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);

  // special operators
  vtkCharArray &operator=(const vtkCharArray& ia);
  void operator+=(const vtkCharArray& ia);
  void operator+=(const unsigned char c);
  unsigned char& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void Reset();

private:
  unsigned char *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  unsigned char *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline unsigned char vtkCharArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline unsigned char *vtkCharArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline unsigned char *vtkCharArray::WritePtr(const int id, const int number) 
{
  if ( (id + number) > this->Size ) this->Resize(id+number);
  this->MaxId = id + number - 1;
  return this->Array + id;
}

// Description:
// Insert data at a specified position in the array.
inline vtkCharArray& vtkCharArray::InsertValue(const int id, const unsigned char c)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = c;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vtkCharArray::InsertNextValue(const unsigned char c)
{
  this->InsertValue (++this->MaxId,c); 
  return this->MaxId;
}
inline void vtkCharArray::operator+=(const unsigned char c)
{
  this->InsertNextValue(c);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline unsigned char& vtkCharArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i;
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vtkCharArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vtkCharArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vtkCharArray::GetMaxId() {return this->MaxId;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vtkCharArray::Reset() {this->MaxId = -1;};

#endif
