/*=========================================================================

  Program:   Visualization Library
  Module:    FScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Floating point representation of scalars
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatScalars_h
#define __vlFloatScalars_h

#include "Scalars.hh"
#include "FArray.hh"

class vlFloatScalars : public vlScalars 
{
public:
  vlFloatScalars() {};
  vlScalars *MakeObject(int sze, int ext=1000);
  int Initialize(const int sz, const int ext=1000) 
    {return S.Initialize(sz,ext);};
  vlFloatScalars(const vlFloatScalars& fs) {this->S = fs.S;};
  vlFloatScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlFloatScalars() {};
  char *GetClassName() {return "vlFloatScalars";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Reset() {this->S.Reset();};
  void Squeeze() {this->S.Squeeze();};
  vlFloatScalars &operator=(const vlFloatScalars& fs);
  void operator+=(const vlFloatScalars& fs) {this->S += fs.S;};

  float GetScalar(int i) {return this->S[i];};
  void SetScalar(int i, float s) {this->S[i] = s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,s);};
  int InsertNextScalar(float s) {return S.InsertNextValue(s);};

private:
  vlFloatArray S;
};

#endif
