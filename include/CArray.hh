/*=========================================================================

  Program:   Visualization Library
  Module:    CArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCharArray - dynamic, self adjusting character array
// .SECTION Description
// vlCharArray is an array of character values. It provides methods
// for insertion and retrieval of characters, and will automatically 
// resize itself to hold new data.

#ifndef __vlCharArray_h
#define __vlCharArray_h

#include "Object.hh"

class vlCharArray : public vlObject 
{
public:
  vlCharArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vlCharArray(const int sz, const int ext=1000);
  vlCharArray(const vlCharArray& ia);
  ~vlCharArray();
  virtual char *GetClassName() {return "vlCharArray";};
  void PrintSelf(ostream& os, vlIndent indent);

  // access/insertion methods
  char GetValue(const int id);
  char *GetPtr(const int id);
  vlCharArray &InsertValue(const int id, const char i);
  int InsertNextValue(const char i);

  // special operators
  vlCharArray &operator=(const vlCharArray& ia);
  void operator+=(const vlCharArray& ia);
  void operator+=(const char i);
  char& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  char *GetArray();
  void Reset();

private:
  char *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  char *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline char vlCharArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline char *vlCharArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Insert data at a specified position in the array.
inline vlCharArray& vlCharArray::InsertValue(const int id, const char i)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = i;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlCharArray::InsertNextValue(const char i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}
inline void vlCharArray::operator+=(const char i) 
{
  this->InsertNextValue(i);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline char& vlCharArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vlCharArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vlCharArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vlCharArray::GetMaxId() {return this->MaxId;};

// Description:
// Get the pointer to the array. Useful for interfacing to C or 
// FORTRAN routines.
inline char *vlCharArray::GetArray() {return this->Array;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlCharArray::Reset() {this->MaxId = -1;};

#endif
