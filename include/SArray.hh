/*=========================================================================

  Program:   Visualization Library
  Module:    SArray.hh
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
//  Dynamic, self adjusting short array
//
#ifndef __vlShortArray_h
#define __vlShortArray_h

#include "Object.hh"

class vlShortArray : public vlObject 
{
public:
  vlShortArray():Array(0),Size(0),MaxId(-1),Extend(1000) {};
  int Initialize(const int sz, const int ext=1000);
  vlShortArray(const int sz, const int ext=1000);
  vlShortArray(const vlShortArray& sa);
  ~vlShortArray();
  short GetValue(const int id) {return this->Array[id];};
  short *GetPtr(const int id) {return this->Array + id;};
  vlShortArray &InsertValue(const int id, const short i)
    {if ( id >= this->Size ) this->Resize(id);
     this->Array[id] = i;
     if ( id > this->MaxId ) this->MaxId = id;
     return *this;
    }
  int InsertNextValue(const short i)
    {this->InsertValue (++this->MaxId,i); return this->MaxId;};
  vlShortArray &operator=(vlShortArray& sa);
  vlShortArray &operator+=(vlShortArray& sa);
  void operator+=(const short i) {this->InsertNextValue(i);};
  // operator[] can be used on both left and right side of expression;
  // Note: if used on lh side, user's responsibility to do range checking
  short& operator[](const int i) {return this->Array[i];};
  void Squeeze() {this->Resize (this->MaxId+1);};
  int GetSize() {return this->Size;};
  int GetMaxId() {return this->MaxId;};
  short *GetArray() {return this->Array;};
  void Reset() {this->MaxId = -1;};
  virtual char *GetClassName() {return "vlShortArray";};
  void PrintSelf(ostream& os, vlIndent indent);

private:
  short *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  short *Resize(const int sz);  // function to resize data
};

#endif

