/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkBitArray - dynamic, self adjusting array of bits
// .SECTION Description
// vtkBitArray is an array of bits (0/1 data value). The array is packed 
// so that each  byte stores eight bits. vtkBitArray provides methods
// for insertion and retrieval of bits, and will automatically resize 
// itself to hold new data.

#ifndef __vtkBitArray_h
#define __vtkBitArray_h

#include "Object.hh"

class vtkBitArray : public vtkObject 
{
public:
  vtkBitArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext);
  void Initialize();
  vtkBitArray(const int sz, const int ext=1000);
  vtkBitArray(const vtkBitArray& ia);
  ~vtkBitArray();
  virtual char *GetClassName() {return "vtkBitArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // access/insertion methods
  int GetValue(const int id);
  vtkBitArray &InsertValue(const int id, const int i);
  int InsertNextValue(const int i);
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);

  // special operators
  vtkBitArray &operator=(const vtkBitArray& ia);
  vtkBitArray &operator+=(const vtkBitArray& ia);
  void operator+=(const char i);
  vtkBitArray &SetValue(const int id, const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void Reset();

private:
  unsigned char *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus iar
  int Extend;     // grow array by this point
  unsigned char *Resize(const int sz);  // function to resize data
};

// Description:
// Get the address of a particular data index.
inline unsigned char *vtkBitArray::GetPtr(const int id)
{
  return this->Array + id/8;
};

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline unsigned char *vtkBitArray::WritePtr(const int id, const int number)
{
  if ( (id + number) > this->Size ) this->Resize(id+number);
  this->MaxId = id + number - 1;
  return this->Array + id/8;
}

// Description:
// Insert data at a specified position in the array. Does not perform
// range checking.
inline vtkBitArray& vtkBitArray::SetValue(const int id, const int i)
{
  if (i) this->Array[id/8] |= (0x80 >> id%8);
  else this->Array[id/8] &= (~(0x80 >> id%8));
  if ( id > this->MaxId ) this->MaxId = id;
 
  return *this;
}

// Description:
// Insert data at a specified position in the array.
inline vtkBitArray& vtkBitArray::InsertValue(const int id, const int i)
{
  if ( id >= this->Size ) this->Resize(id);
  if (i) this->Array[id/8] |= (0x80 >> id%8);
  else this->Array[id/8] &= (~(0x80 >> id%8));
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vtkBitArray::InsertNextValue(const int i)
{
  this->InsertValue (++this->MaxId,i); return this->MaxId;
}
inline void vtkBitArray::operator+=(const char i) 
{
  this->InsertNextValue(i);
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vtkBitArray::Squeeze() {this->Resize (this->MaxId+1);}

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vtkBitArray::GetSize() {return this->Size;}

// Description:
// Returning the maximum index of data inserted so far.
inline int vtkBitArray::GetMaxId() {return this->MaxId;}

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vtkBitArray::Reset() {this->MaxId = -1;}

#endif

