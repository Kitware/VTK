/*=========================================================================

  Program:   Visualization Library
  Module:    BArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlBitArray - dynamic, self adjusting array of bits
// .SECTION Description
// vlBitArray is an array of bits (0/1 data value). The array is packed 
// so that each  byte stores eight bits. vlBitArray provides methods
// for insertion and retrieval of bits, and will automatically resize 
// itself to hold new data.

#ifndef __vlBitArray_h
#define __vlBitArray_h

#include "Object.hh"

class vlBitArray : public vlObject 
{
public:
  vlBitArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext);
  void Initialize();
  vlBitArray(const int sz, const int ext=1000);
  vlBitArray(const vlBitArray& ia);
  ~vlBitArray();
  virtual char *GetClassName() {return "vlBitArray";};
  void PrintSelf(ostream& os, vlIndent indent);

  // access/insertion methods
  int GetValue(const int id);
  char *GetPtr(const int id);
  vlBitArray &InsertValue(const int id, const int i);
  int InsertNextValue(const int i);

  // special operators
  vlBitArray &operator=(const vlBitArray& ia);
  vlBitArray &operator+=(const vlBitArray& ia);
  void operator+=(const char i) {this->InsertNextValue(i);};
  vlBitArray &SetValue(const int id, const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  char *GetArray();
  void Reset();

private:
  unsigned char *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus iar
  int Extend;     // grow array by this point
  char *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline int vlBitArray::GetValue(const int id) 
{
  if (this->Array[id/8]&(0x80 >> (id%8))) return 1; return 0;
};

// Description:
// Get the address of a particular data index.
inline char *vlBitArray::GetPtr(const int id) 
{
  return this->Array + id/8;
};

// Description:
// Insert data at a specified position in the array. Does not perform
// range checking.
inline vlBitArray& vlBitArray::SetValue(const int id, const int i)
{
  if (i) this->Array[id/8] |= (0x80 >> id%8);
  else this->Array[id/8] &= (~(0x80 >> id%8));
  if ( id > this->MaxId ) this->MaxId = id;
 
  return *this;
}

// Description:
// Insert data at a specified position in the array.
inline vlBitArray& vlBitArray::InsertValue(const int id, const int i)
{
  if ( id >= this->Size ) this->Resize(id);
  if (i) this->Array[id/8] |= (0x80 >> id%8);
  else this->Array[id/8] &= (~(0x80 >> id%8));
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vlBitArray::InsertNextValue(const int i)
{
  this->InsertValue (++this->MaxId,i); return this->MaxId;
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vlBitArray::Squeeze() {this->Resize (this->MaxId+1);}

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vlBitArray::GetSize() {return this->Size;}

// Description:
// Returning the maximum index of data inserted so far.
inline int vlBitArray::GetMaxId() {return this->MaxId;}

// Description:
// Get the pointer to the array. Useful for interfacing to C or 
// FORTRAN routines.
inline char *vlBitArray::GetArray() {return this->Array;}

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vlBitArray::Reset() {this->MaxId = -1;}

#endif

