/*=========================================================================

  Program:   Visualization Library
  Module:    FArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFloatArray - dynamic, self adjusting floating point array
// .SECTION Description
// vlFloatArray is an array of floating point numbers. It provides methods
// for insertion and retrieval of floating point values, and will 
// automatically resize itself to hold new data.

#ifndef __vlFloatArray_h
#define __vlFloatArray_h

#include "Object.hh"

class vlFloatArray : public vlObject 
{
public:
  vlFloatArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vlFloatArray(const int sz, const int ext=1000);
  vlFloatArray(const vlFloatArray& fa);
  ~vlFloatArray();
  virtual char *GetClassName() {return "vlFloatArray";};
  void PrintSelf(ostream& os, vlIndent indent);

  // access/insertion methods
  float GetValue(const int id);
  float *GetPtr(const int id);
  vlFloatArray &InsertValue(const int id, const float f);
  int InsertNextValue(const float f);

  // special operators
  vlFloatArray &operator=(const vlFloatArray& fa);
  void operator+=(const vlFloatArray& fa);
  void operator+=(const float f) {this->InsertNextValue(f);};
  float& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  float *GetArray();
  void Reset();

private:
  float *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  float *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline float vlFloatArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline float *vlFloatArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Insert data at a specified position in the array.
inline vlFloatArray& vlFloatArray::InsertValue(const int id, const float f)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = f;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlFloatArray::InsertNextValue(const float f)
{
  this->InsertValue (++this->MaxId,f); 
  return this->MaxId;
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline float& vlFloatArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vlFloatArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vlFloatArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vlFloatArray::GetMaxId() {return this->MaxId;};

// Description:
// Get the pointer to the array. Useful for interfacing to C or 
// FORTRAN routines.
inline float *vlFloatArray::GetArray() {return this->Array;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlFloatArray::Reset() {this->MaxId = -1;};

#endif
