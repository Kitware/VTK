/*=========================================================================

  Program:   Visualization Library
  Module:    BScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
//
// Char representation of scalars
//
//  use internal char array to represent data
//
#ifndef __vlBitScalars_h
#define __vlBitScalars_h

#include "Scalars.hh"
#include "BArray.hh"

class vlBitScalars : public vlScalars 
{
public:
  vlBitScalars() {};
  vlScalars *MakeObject(int sze, int ext=1000);
  int Allocate(const int sz, const int ext=1000) 
    {return this->S.Allocate(sz,ext);};
  void Initialize() {return this->S.Initialize();};
  vlBitScalars(const vlBitScalars& cs) {this->S = cs.S;};
  vlBitScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlBitScalars() {};
  char *GetClassName() {return "vlBitScalars";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Reset() {this->S.Reset();};
  void Squeeze() {this->S.Squeeze();};
  vlBitScalars &operator=(const vlBitScalars& cs);
  void operator+=(const vlBitScalars& cs) {this->S += cs.S;};

  // float conversion for abstract computation
  float GetScalar(int i) {return (float)this->S.GetValue(i);};
  void SetScalar(int i, int s) {this->S.SetValue(i,s);};
  void SetScalar(int i, float s) {this->S.SetValue(i,(int)s);};
  void InsertScalar(int i, float s) {S.InsertValue(i,(int)s);};
  void InsertScalar(int i, int s) {S.InsertValue(i,s);};
  int InsertNextScalar(int s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((int)s);};

private:
  vlBitArray S;
};

#endif
