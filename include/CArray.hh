/*=========================================================================

  Program:   Visualization Library
  Module:    CArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
//  Dynamic, self adjusting char array
//
#ifndef __vlCharArray_h
#define __vlCharArray_h

#include "Object.hh"

class vlCharArray : public vlObject 
{
public:
  vlCharArray():Array(0),Size(0),MaxId(-1),Extend(1000) {};
  int Initialize(const int sz, const int ext);
  vlCharArray(const int sz, const int ext);
  vlCharArray(const vlCharArray& ia);
  ~vlCharArray();
  char GetValue(const int id) {return this->Array[id];};
  char *GetPtr(const int id) {return this->Array + id;};
  vlCharArray &InsertValue(const int id, const int i)
    {if ( id >= this->Size ) this->Resize(id);
     this->Array[id] = i;
     if ( id > this->MaxId ) this->MaxId = id;
     return *this;
    }
  int InsertNextValue(const int i)
    {this->InsertValue (++this->MaxId,i); return this->MaxId;};
  vlCharArray &operator=(vlCharArray& ia);
  vlCharArray &operator+=(vlCharArray& ia);
  void operator+=(const char i) {this->InsertNextValue(i);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on lh side, user's responsibility to do range checking
  char& operator[](const int i)
    {if (i > this->MaxId) this->MaxId = i; return this->Array[i];};
  void Squeeze() {this->Resize (this->MaxId+1);};
  int GetSize() {return this->Size;};
  int GetMaxId() {return this->MaxId;};
  char *GetArray() {return this->Array;};
  void Reset() {this->MaxId = -1;};
  virtual char *GetClassName() {return "vlCharArray";};
  void PrintSelf(ostream& os, vlIndent indent);

private:
  char *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus iar
  int Extend;     // grow array by this point
  char *Resize(const int sz);  // function to resize data
};

#endif

