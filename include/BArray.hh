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
//
//  Dynamic, self adjusting bit array
//
#ifndef __vlBitArray_h
#define __vlBitArray_h

#include "Object.hh"

class vlBitArray : public vlObject 
{
public:
  vlBitArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  void Initialize();
  int Allocate(const int sz, const int ext);
  vlBitArray(const int sz, const int ext);
  vlBitArray(const vlBitArray& ia);
  ~vlBitArray();
  int GetValue(const int id) 
    {if (this->Array[id/8]&(0x80 >> (id%8))) return 1; return 0;};
  char *GetPtr(const int id) {return this->Array + id/8;};

  vlBitArray &SetValue(const int id, const int i)
  {
  if (i) this->Array[id/8] |= (0x80 >> id%8);
  else this->Array[id/8] &= (~(0x80 >> id%8));
  if ( id > this->MaxId ) this->MaxId = id;
  }

  vlBitArray &InsertValue(const int id, const int i)
  {
  if ( id >= this->Size ) this->Resize(id);
  if (i) this->Array[id/8] |= (0x80 >> id%8);
  else this->Array[id/8] &= (~(0x80 >> id%8));
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
  }

  int InsertNextValue(const int i)
  {this->InsertValue (++this->MaxId,i); return this->MaxId;};
  vlBitArray &operator=(const vlBitArray& ia);
  vlBitArray &operator+=(const vlBitArray& ia);
  void operator+=(const char i) {this->InsertNextValue(i);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on lh side, user's responsibility to do range checking
  // Note: bit arrays don't handle [] very well 
  //  char& operator[](const int i)
  //    {if (i > this->MaxId) this->MaxId = i; 
  //    return (this->Array[i/8]&(0x80 >> (id%8)));};
  void Squeeze() {this->Resize (this->MaxId+1);};
  int GetSize() {return this->Size;};
  int GetMaxId() {return this->MaxId;};
  char *GetArray() {return this->Array;};
  void Reset() {this->MaxId = -1;};
  virtual char *GetClassName() {return "vlBitArray";};
  void PrintSelf(ostream& os, vlIndent indent);

private:
  unsigned char *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus iar
  int Extend;     // grow array by this point
  char *Resize(const int sz);  // function to resize data
};

#endif

