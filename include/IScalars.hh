/*=========================================================================

  Program:   Visualization Library
  Module:    IScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlIntScalars - integer representation of scalar data
// .SECTION Description
// vlIntScalars is a concrete implementation of vlScalars. Scalars are
// represented using integer values.

#ifndef __vlIntScalars_h
#define __vlIntScalars_h

#include "Scalars.hh"
#include "SArray.hh"

class vlIntScalars : public vlScalars 
{
public:
  vlIntScalars() {};
  vlIntScalars(const vlIntScalars& ss) {this->S = ss.S;};
  vlIntScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlIntScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlIntScalars";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "int";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, int s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (int)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(int)s);};
  void InsertScalar(int i, int s) {S.InsertValue(i,s);};
  int InsertNextScalar(int s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((int)s);};
  void GetScalars(vlIdList& ptIds, vlFloatScalars& fs);

  // miscellaneous
  int *WritePtr(const int id, const int number);
  vlIntScalars &operator=(const vlIntScalars& is);
  void operator+=(const vlIntScalars& is) {this->S += is.S;};
  void Reset() {this->S.Reset();};

private:
  vlIntArray S;
};

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the locaation you 
// wish to write into; number is the number of scalars to write.
inline int *vlIntScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

#endif
