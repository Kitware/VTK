/*=========================================================================

  Program:   Visualization Library
  Module:    VArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlVoidArray - dynamic, self adjusting array of void* pointers
// .SECTION Description
// vlVoidArray is an array of pointers to void. It provides methods
// for insertion and retrieval of these pointers values, and will 
// automatically resize itself to hold new data.

#ifndef __vlVoidArray_h
#define __vlVoidArray_h

#include "Object.hh"

class vlVoidArray : public vlObject 
{
public:
  vlVoidArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vlVoidArray(const int sz, const int ext=1000);
  vlVoidArray(const vlVoidArray& fa);
  ~vlVoidArray();
  virtual char *GetClassName() {return "vlVoidArray";};
  void PrintSelf(ostream& os, vlIndent indent);

  // access/insertion methods
  void* GetValue(const int id);
  vlVoidArray &InsertValue(const int id, void* p);
  int InsertNextValue(void* v);
  void** GetPtr(const int id);
  void** WritePtr(const int id, const int number);

  // special operators
  vlVoidArray &operator=(const vlVoidArray& fa);
  void operator+=(const vlVoidArray& fa);
  void operator+=(void* p);
  void* &operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void Reset();

private:
  void** Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  void** Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline void* vlVoidArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline void** vlVoidArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline void** vlVoidArray::WritePtr(const int id, const int number) 
{
  if ( (id + number) > this->Size ) this->Resize(id+number);
  this->MaxId = id + number - 1;
  return this->Array + id;
}

// Description:
// Insert data at a specified position in the array.
inline vlVoidArray& vlVoidArray::InsertValue(const int id, void* p)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = p;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlVoidArray::InsertNextValue(void* p)
{
  this->InsertValue (++this->MaxId,p);
  return this->MaxId;
}
inline void vlVoidArray::operator+=(void* p) 
{
  this->InsertNextValue(p);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline void* &vlVoidArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vlVoidArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vlVoidArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vlVoidArray::GetMaxId() {return this->MaxId;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlVoidArray::Reset() {this->MaxId = -1;};

#endif
