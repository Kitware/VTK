/*=========================================================================

  Program:   Visualization Library
  Module:    SScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlShortScalars - short integer representation of scalar data
// .SECTION Description
// vlShortScalars is a concrete implementation of vlScalars. Scalars are
// represented using short integer values.

#ifndef __vlShortScalars_h
#define __vlShortScalars_h

#include "Scalars.hh"
#include "SArray.hh"

class vlShortScalars : public vlScalars 
{
public:
  vlShortScalars() {};
  vlShortScalars(const vlShortScalars& ss) {this->S = ss.S;};
  vlShortScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlShortScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlShortScalars";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, short s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (short)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(short)s);};
  void InsertScalar(int i, short s) {S.InsertValue(i,s);};
  int InsertNextScalar(short s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((short)s);};

  // miscellaneous
  vlShortScalars &operator=(const vlShortScalars& ss);
  void operator+=(const vlShortScalars& ss) {this->S += ss.S;};
  void Reset() {this->S.Reset();};

private:
  vlShortArray S;
};

#endif
