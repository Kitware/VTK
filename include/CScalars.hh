/*=========================================================================

  Program:   Visualization Library
  Module:    CScalars.hh
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
// Char representation of scalars
//
//  use internal char array to represent data
//
#ifndef __vlCharScalars_h
#define __vlCharScalars_h

#include "Scalars.hh"
#include "CArray.hh"

class vlCharScalars : public vlScalars 
{
public:
  vlCharScalars() {};
  vlScalars *MakeObject(int sze, int ext=1000);
  int Initialize(const int sz, const int ext=1000) 
    {return S.Initialize(sz,ext);};
  vlCharScalars(const vlCharScalars& cs) {this->S = cs.S;};
  vlCharScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlCharScalars() {};
  char *GetClassName() {return "vlCharScalars";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Reset() {this->S.Reset();};
  void Squeeze() {this->S.Squeeze();};
  vlCharScalars &operator=(const vlCharScalars& cs);
  void operator+=(const vlCharScalars& cs) {this->S += cs.S;};

  // float conversion for abstract computation
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, char s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (char)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(char)s);};
  void InsertScalar(int i, char s) {S.InsertValue(i,s);};
  int InsertNextScalar(char s) {return S.InsertNextValue(s);};

private:
  vlCharArray S;
};

#endif
