/*=========================================================================

  Program:   Visualization Library
  Module:    SScalars.hh
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
// Short representation of scalars
//
//  use internal short array to represent data
//
#ifndef __vlShortScalars_h
#define __vlShortScalars_h

#include "Scalars.hh"
#include "SArray.hh"

class vlShortScalars : public vlScalars 
{
public:
  vlShortScalars() {};
  vlScalars *MakeObject(int sze, int ext=1000);
  int Initialize(const int sz, const int ext=1000) 
    {return S.Initialize(sz,ext);};
  vlShortScalars(const vlShortScalars& ss) {this->S = ss.S;};
  vlShortScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlShortScalars() {};
  char *GetClassName() {return "vlShortScalars";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Reset() {this->S.Reset();};
  void Squeeze() {this->S.Squeeze();};
  vlShortScalars &operator=(const vlShortScalars& ss);
  void operator+=(const vlShortScalars& ss) {this->S += ss.S;};

  // float conversion for abstract computation
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, short s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (short)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(short)s);};
  void InsertScalar(int i, short s) {S.InsertValue(i,s);};
  int InsertNextScalar(short s) {return S.InsertNextValue(s);};

private:
  vlShortArray S;
};

#endif
