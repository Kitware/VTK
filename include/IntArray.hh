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
//
//  Dynamic, self adjusting integer array
//
#ifndef __vlIntArray_h
#define __vlIntArray_h

#include "Object.hh"

class vlIntArray : public vlObject 
{
public:
  vlIntArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  void Initialize();
  int Allocate(const int sz, const int ext);
  vlIntArray(const int sz, const int ext);
  vlIntArray(const vlIntArray& ia);
  ~vlIntArray();
  int GetValue(const int id) {return this->Array[id];};
  int *GetPtr(const int id) {return this->Array + id;};
  vlIntArray &InsertValue(const int id, const int i)
    {if ( id >= this->Size ) this->Resize(id);
     this->Array[id] = i;
     if ( id > this->MaxId ) this->MaxId = id;
     return *this;
    }
  int InsertNextValue(const int i)
    {this->InsertValue (++this->MaxId,i); return this->MaxId;};
  vlIntArray &operator=(const vlIntArray& ia);
  void operator+=(const vlIntArray& ia);
  void operator+=(const int i) {this->InsertNextValue(i);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on lh side, user's responsibility to do range checking
  int& operator[](const int i) 
    {if (i > this->MaxId) this->MaxId = i; return this->Array[i];};
  void Squeeze() {this->Resize (this->MaxId+1);};
  int GetSize() {return this->Size;};
  int GetMaxId() {return this->MaxId;};
  int *GetArray() {return this->Array;};
  void Reset() {this->MaxId = -1;};
  virtual char *GetClassName() {return "vlIntArray";};
  void PrintSelf(ostream& os, vlIndent indent);

private:
  int *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus iar
  int Extend;     // grow array by this point
  int *Resize(const int sz);  // function to resize data
};

#endif

