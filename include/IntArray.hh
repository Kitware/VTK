/*=========================================================================

  Program:   Visualization Library
  Module:    IntArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlIntArray - dynamic, self adjusting integer array
// .SECTION Description
// vlIntArray is an array of integer numbers. It provides methods
// for insertion and retrieval of integer values, and will 
// automatically resize itself to hold new data.

#ifndef __vlIntArray_h
#define __vlIntArray_h

#include "Object.hh"

class vlIntArray : public vlObject 
{
public:
  vlIntArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vlIntArray(const int sz, const int ext=1000);
  vlIntArray(const vlIntArray& ia);
  ~vlIntArray();
  virtual char *GetClassName() {return "vlIntArray";};
  void PrintSelf(ostream& os, vlIndent indent);

  // access/insertion methods
  int GetValue(const int id);
  int *GetPtr(const int id);
  vlIntArray &InsertValue(const int id, const int i);
  int InsertNextValue(const int i);

  // special operators
  vlIntArray &operator=(const vlIntArray& ia);
  void operator+=(const vlIntArray& ia);
  void operator+=(const int i);
  int& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  int *GetArray();
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
inline int vlIntArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline int *vlIntArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Insert data at a specified position in the array.
inline vlIntArray& vlIntArray::InsertValue(const int id, const int i)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = i;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlIntArray::InsertNextValue(const int i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}
inline void vlIntArray::operator+=(const int i) 
{
  this->InsertNextValue(i);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline int& vlIntArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vlIntArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vlIntArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vlIntArray::GetMaxId() {return this->MaxId;};

// Description:
// Get the pointer to the array. Useful for interfacing to C or 
// FORTRAN routines.
inline int *vlIntArray::GetArray() {return this->Array;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlIntArray::Reset() {this->MaxId = -1;};

#endif
