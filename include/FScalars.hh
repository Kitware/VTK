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
// .NAME vlFloatScalars - floating point representation of scalar data
// .SECTION Description
// vlFloatScalars is a concrete implementation of vlScalars. Scalars are
// represented using float values.

#ifndef __vlFloatScalars_h
#define __vlFloatScalars_h

#include "Scalars.hh"
#include "FArray.hh"

class vlFloatScalars : public vlScalars 
{
public:
  vlFloatScalars() {};
  vlFloatScalars(const vlFloatScalars& fs) {this->S = fs.S;};
  vlFloatScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlFloatScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlFloatScalars";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return this->S.GetValue(i);};
  void SetScalar(int i, float s) {this->S[i] = s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,s);};
  int InsertNextScalar(float s) {return S.InsertNextValue(s);};
  void GetScalars(vlIdList& ptIds, vlFloatScalars& fs);

  // miscellaneous
  vlFloatScalars &operator=(const vlFloatScalars& fs);
  void operator+=(const vlFloatScalars& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};

protected:
  vlFloatArray S;
};

#endif
