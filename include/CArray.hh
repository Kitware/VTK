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
// .NAME vlCharArray - dynamic, self adjusting unssigned character array
// .SECTION Description
// vlCharArray is an array of unsigned character values. It provides methods
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
  unsigned char GetValue(const int id);
  unsigned char *GetPtr(const int id);
  vlCharArray &InsertValue(const int id, const unsigned char c);
  int InsertNextValue(const unsigned char c);

  // special operators
  vlCharArray &operator=(const vlCharArray& ia);
  void operator+=(const vlCharArray& ia);
  void operator+=(const unsigned char c);
  unsigned char& operator[](const int i);
  unsigned char *WriteInto(const int i,  const int number);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  unsigned char *GetArray();
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
inline unsigned char vlCharArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline unsigned char *vlCharArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Insert data at a specified position in the array.
inline vlCharArray& vlCharArray::InsertValue(const int id, const unsigned char c)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = c;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlCharArray::InsertNextValue(const unsigned char c)
{
  this->InsertValue (++this->MaxId,c); 
  return this->MaxId;
}
inline void vlCharArray::operator+=(const unsigned char c)
{
  this->InsertNextValue(c);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline unsigned char& vlCharArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = 1;
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
inline unsigned char *vlCharArray::GetArray() {return this->Array;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlCharArray::Reset() {this->MaxId = -1;};

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary).
inline unsigned char *vlCharArray::WriteInto(const int id, const int number)
{
  if ( (id + number) >= this->Size ) this->Resize(id+number);
  this->MaxId = id + number - 1;
  return this->Array + id;
}

#endif
