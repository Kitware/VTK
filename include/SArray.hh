/*=========================================================================

  Program:   Visualization Library
  Module:    SArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlShortArray - dynamic, self adjusting short integer array
// .SECTION Description
// vlShortArray is an array of short integer numbers. It provides methods
// for insertion and retrieval of integer values, and will 
// automatically resize itself to hold new data.

#ifndef __vlShortArray_h
#define __vlShortArray_h

#include "Object.hh"

class vlShortArray : public vlObject 
{
public:
  vlShortArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vlShortArray(const int sz, const int ext=1000);
  vlShortArray(const vlShortArray& ia);
  ~vlShortArray();
  virtual char *GetClassName() {return "vlShortArray";};
  void PrintSelf(ostream& os, vlIndent indent);

  // access/insertion methods
  short GetValue(const int id);
  short *GetPtr(const int id);
  vlShortArray &InsertValue(const int id, const short i);
  int InsertNextValue(const int short);

  // special operators
  vlShortArray &operator=(const vlShortArray& ia);
  void operator+=(const vlShortArray& ia);
  void operator+=(const short i) {this->InsertNextValue(i);};
  short& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  short *GetArray();
  void Reset();

private:
  short *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  short *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline short vlShortArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline short *vlShortArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Insert data at a specified position in the array.
inline vlShortArray& vlShortArray::InsertValue(const int id, const short i)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = i;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlShortArray::InsertNextValue(const short i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline short& vlShortArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vlShortArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vlShortArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vlShortArray::GetMaxId() {return this->MaxId;};

// Description:
// Get the pointer to the array. Useful for interfacing to C or 
// FORTRAN routines.
inline short *vlShortArray::GetArray() {return this->Array;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlShortArray::Reset() {this->MaxId = -1;};

#endif
